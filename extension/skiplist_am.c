/*
 * skiplist_am.c - Registra la Skip List como un nuevo tipo de índice
 * ("método de acceso") de PostgreSQL.
 *
 * PostgreSQL no conoce la Skip List: solo sabe llamar a ciertas funciones
 * cuando necesita algo del índice. Este archivo define esas funciones:
 *
 *   CREATE INDEX ... USING skiplist  -> skiplist_construir()
 *   INSERT INTO tabla ...            -> skiplist_insertar()
 *   SELECT ... WHERE col = / < / >   -> skiplist_iniciar_busqueda(),
 *                                       skiplist_siguiente_fila(), ...
 *   VACUUM                           -> skiplist_limpiar()
 *
 * El trabajo de la estructura lo hace el código C++ (src/skiplist.hpp),
 * al que se llega a través de puente.cpp.
 */
#include "postgres.h"

#include <math.h>

#include "access/amapi.h"
#include "access/genam.h"
#include "access/relscan.h"
#include "access/stratnum.h"
#include "access/tableam.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "nodes/tidbitmap.h"
#include "optimizer/optimizer.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/selfuncs.h"

#include "puente.h"
#include "skiplist_meta.h"

PG_MODULE_MAGIC;

/* ----------------------------------------------------------------------
 * Conversión entre el TID de PostgreSQL y nuestra SLUbicacion
 * ---------------------------------------------------------------------- */
static SLUbicacion
a_ubicacion(ItemPointer tid)
{
    SLUbicacion u;

    u.bloque = ItemPointerGetBlockNumber(tid);
    u.posicion = ItemPointerGetOffsetNumber(tid);
    return u;
}

static void
a_tid(SLUbicacion u, ItemPointer tid)
{
    ItemPointerSet(tid, u.bloque, u.posicion);
}

/* ======================================================================
 * CONSTRUCCIÓN: CREATE INDEX
 * 1) se crea la metapágina,
 * 2) PostgreSQL recorre la tabla y nos entrega cada (clave, ubicación),
 * 3) la Skip List (C++) ordena todo y arma la estructura.
 * ====================================================================== */
typedef struct EstadoConstruccion
{
    SLEntrada  *entradas;
    size_t      cantidad;
    size_t      capacidad;
} EstadoConstruccion;

static void
recibir_fila(Relation indice, ItemPointer tid, Datum *valores, bool *esNulo,
             bool filaViva, void *estadoPtr)
{
    EstadoConstruccion *estado = (EstadoConstruccion *) estadoPtr;

    if (esNulo[0])
        return;                 /* los NULL no se indexan */
    if (estado->cantidad == estado->capacidad)
    {
        estado->capacidad *= 2;
        estado->entradas = repalloc_huge(estado->entradas, estado->capacidad * sizeof(SLEntrada));
    }
    estado->entradas[estado->cantidad].clave = DatumGetInt32(valores[0]);
    estado->entradas[estado->cantidad].ubicacion = a_ubicacion(tid);
    estado->cantidad++;
}

static IndexBuildResult *
skiplist_construir(Relation tabla, Relation indice, IndexInfo *indexInfo)
{
    EstadoConstruccion estado;
    IndexBuildResult *resultado;
    double      filas;

    if (RelationGetNumberOfBlocks(indice) != 0)
        elog(ERROR, "el índice \"%s\" ya tiene datos", RelationGetRelationName(indice));

    slpg_iniciar_meta(indice);

    estado.capacidad = 1024;
    estado.cantidad = 0;
    estado.entradas = palloc_extended(estado.capacidad * sizeof(SLEntrada), MCXT_ALLOC_HUGE);

    filas = table_index_build_scan(tabla, indice, indexInfo, true, true,
                                   recibir_fila, &estado, NULL);

    sl_construir(indice, estado.entradas, estado.cantidad);
    pfree(estado.entradas);

    resultado = palloc(sizeof(IndexBuildResult));
    resultado->heap_tuples = filas;
    resultado->index_tuples = (double) estado.cantidad;
    return resultado;
}

static void
skiplist_construir_vacio(Relation indice)
{
    ereport(ERROR,
            (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
             errmsg("el índice skiplist no soporta tablas UNLOGGED")));
}

/* ======================================================================
 * INSERCIÓN: INSERT / UPDATE
 * Un INSERT a la vez modifica la Skip List (LockPage); las búsquedas
 * pueden seguir leyendo mientras tanto.
 * ====================================================================== */
static bool
skiplist_insertar(Relation indice, Datum *valores, bool *esNulo,
                  ItemPointer tid, Relation tabla, IndexUniqueCheck checkUnique,
                  bool indexUnchanged, IndexInfo *indexInfo)
{
    if (esNulo[0])
        return false;

    LockPage(indice, SL_PAGINA_META, ExclusiveLock);
    sl_insertar(indice, DatumGetInt32(valores[0]), a_ubicacion(tid));
    UnlockPage(indice, SL_PAGINA_META, ExclusiveLock);
    return false;
}

/* ======================================================================
 * BÚSQUEDA: SELECT ... WHERE clave (=, <, <=, >, >=) valor
 * Todas las condiciones se convierten en un rango [desde, hasta].
 * Ejemplo: "= 50" es [50, 50];  "BETWEEN 10 AND 20" es [10, 20].
 * ====================================================================== */
typedef struct EstadoBusqueda
{
    int64       desde;
    int64       hasta;
    bool        vacio;          /* las condiciones no pueden cumplirse */
    bool        iniciado;
    sl_ref      actual;         /* próximo nodo a revisar */
} EstadoBusqueda;

static IndexScanDesc
skiplist_iniciar_busqueda(Relation indice, int nclaves, int norderbys)
{
    IndexScanDesc scan = RelationGetIndexScan(indice, nclaves, norderbys);

    scan->opaque = palloc0(sizeof(EstadoBusqueda));
    return scan;
}

static void
skiplist_reiniciar_busqueda(IndexScanDesc scan, ScanKey claves, int nclaves,
                            ScanKey orderbys, int norderbys)
{
    EstadoBusqueda *e = (EstadoBusqueda *) scan->opaque;

    if (claves && scan->numberOfKeys > 0)
        memcpy(scan->keyData, claves, scan->numberOfKeys * sizeof(ScanKeyData));

    e->desde = PG_INT32_MIN;
    e->hasta = PG_INT32_MAX;
    e->vacio = false;
    e->iniciado = false;

    for (int i = 0; i < scan->numberOfKeys; i++)
    {
        ScanKey     k = &scan->keyData[i];
        int64       v;

        if (k->sk_flags & SK_ISNULL)
        {
            e->vacio = true;    /* "col = NULL" nunca es verdadero */
            continue;
        }
        v = DatumGetInt32(k->sk_argument);
        switch (k->sk_strategy)
        {
            case BTLessStrategyNumber:          /* <  */
                e->hasta = Min(e->hasta, v - 1);
                break;
            case BTLessEqualStrategyNumber:     /* <= */
                e->hasta = Min(e->hasta, v);
                break;
            case BTEqualStrategyNumber:         /* =  */
                e->desde = Max(e->desde, v);
                e->hasta = Min(e->hasta, v);
                break;
            case BTGreaterEqualStrategyNumber:  /* >= */
                e->desde = Max(e->desde, v);
                break;
            case BTGreaterStrategyNumber:       /* >  */
                e->desde = Max(e->desde, v + 1);
                break;
            default:
                elog(ERROR, "skiplist: estrategia %d no soportada", k->sk_strategy);
        }
    }
    if (e->desde > e->hasta)
        e->vacio = true;
}

/* Devuelve en *tid la siguiente fila que cumple el rango. false = no hay más. */
static bool
siguiente_coincidencia(IndexScanDesc scan, ItemPointer tid)
{
    EstadoBusqueda *e = (EstadoBusqueda *) scan->opaque;
    SLNodo      nodo;

    if (e->vacio)
        return false;
    if (!e->iniciado)
    {
        /* bajar por los niveles hasta el primer nodo >= desde */
        e->actual = sl_primero_mayor_igual(scan->indexRelation, (int32) e->desde);
        e->iniciado = true;
        /* estadísticas: cuenta una búsqueda en el índice (se ve en EXPLAIN) */
        pgstat_count_index_scan(scan->indexRelation);
        if (scan->instrument)
            scan->instrument->nsearches++;
    }
    while (e->actual != SL_NULO)
    {
        CHECK_FOR_INTERRUPTS();
        /* leer el nodo y avanzar por el nivel 0 */
        e->actual = sl_siguiente(scan->indexRelation, e->actual, &nodo);
        if (nodo.clave > e->hasta)
            break;              /* ya pasamos el rango: terminar */
        if (nodo.borrado)
            continue;
        a_tid(nodo.ubicacion, tid);
        return true;
    }
    e->actual = SL_NULO;
    return false;
}

/* Index Scan: PostgreSQL pide las filas de una en una. */
static bool
skiplist_siguiente_fila(IndexScanDesc scan, ScanDirection dir)
{
    scan->xs_recheck = false;   /* la Skip List es exacta: no hace falta revisar de nuevo */
    return siguiente_coincidencia(scan, &scan->xs_heaptid);
}

/* Bitmap Index Scan: PostgreSQL pide todas las filas de una vez. */
static int64
skiplist_todas_las_filas(IndexScanDesc scan, TIDBitmap *tbm)
{
    ItemPointerData tid;
    int64       total = 0;

    while (siguiente_coincidencia(scan, &tid))
    {
        tbm_add_tuples(tbm, &tid, 1, false);
        total++;
    }
    return total;
}

static void
skiplist_terminar_busqueda(IndexScanDesc scan)
{
    pfree(scan->opaque);
}

/* ======================================================================
 * VACUUM: marca como borrados los nodos cuyas filas ya no existen
 * (borrado lógico; el espacio no se reutiliza).
 * ====================================================================== */
static IndexBulkDeleteResult *
skiplist_limpiar(IndexVacuumInfo *info, IndexBulkDeleteResult *stats,
                 IndexBulkDeleteCallback filaEliminada, void *estado)
{
    Relation    indice = info->index;
    BlockNumber npaginas = RelationGetNumberOfBlocks(indice);

    if (stats == NULL)
        stats = palloc0(sizeof(IndexBulkDeleteResult));

    for (BlockNumber p = SL_PAGINA_META + 1; p < npaginas; p++)
    {
        Buffer      buf;
        Page        pagina;
        bool        cambio = false;

        vacuum_delay_point(false);
        buf = ReadBufferExtended(indice, MAIN_FORKNUM, p, RBM_NORMAL, info->strategy);
        LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
        pagina = BufferGetPage(buf);

        for (OffsetNumber pos = FirstOffsetNumber; pos <= PageGetMaxOffsetNumber(pagina); pos++)
        {
            SLNodo     *nodo = (SLNodo *) PageGetItem(pagina, PageGetItemId(pagina, pos));
            ItemPointerData tid;

            if (nodo->borrado)
                continue;
            a_tid(nodo->ubicacion, &tid);
            if (filaEliminada(&tid, estado))
            {
                nodo->borrado = 1;
                cambio = true;
                stats->tuples_removed += 1;
            }
            else
                stats->num_index_tuples += 1;
        }
        if (cambio)
            MarkBufferDirty(buf);
        UnlockReleaseBuffer(buf);
    }
    stats->num_pages = npaginas;
    return stats;
}

static IndexBulkDeleteResult *
skiplist_limpiar_final(IndexVacuumInfo *info, IndexBulkDeleteResult *stats)
{
    if (stats == NULL)
    {
        stats = palloc0(sizeof(IndexBulkDeleteResult));
        stats->num_index_tuples = info->num_heap_tuples;
        stats->estimated_count = true;
    }
    stats->num_pages = RelationGetNumberOfBlocks(info->index);
    return stats;
}

/* ======================================================================
 * ESTIMACIÓN DE COSTO: el planificador la usa para decidir si conviene
 * usar el índice. Se usa la estimación genérica de PostgreSQL más el costo
 * de bajar por los niveles (~log2 n comparaciones).
 * ====================================================================== */
static void
skiplist_costo(PlannerInfo *root, IndexPath *path, double loop_count,
               Cost *costoInicial, Cost *costoTotal, Selectivity *selectividad,
               double *correlacion, double *paginas)
{
    GenericCosts costs = {0};
    double      tuplas = Max(path->indexinfo->tuples, 2.0);
    Cost        descenso = ceil(log2(tuplas)) * 2 * cpu_operator_cost;

    genericcostestimate(root, path, loop_count, &costs);

    *costoInicial = costs.indexStartupCost + descenso;
    *costoTotal = costs.indexTotalCost + descenso;
    *selectividad = costs.indexSelectivity;
    *correlacion = costs.indexCorrelation;
    *paginas = costs.numIndexPages;
}

static bytea *
skiplist_opciones(Datum reloptions, bool validate)
{
    return NULL;                /* el índice no tiene opciones configurables */
}

static bool
skiplist_validar(Oid opclassoid)
{
    return true;
}

/* ======================================================================
 * HANDLER: le dice a PostgreSQL qué sabe hacer este índice y qué función
 * llamar en cada caso. Es lo que se registra con CREATE ACCESS METHOD.
 * ====================================================================== */
PG_FUNCTION_INFO_V1(skiplist_handler);

Datum
skiplist_handler(PG_FUNCTION_ARGS)
{
    IndexAmRoutine *am = makeNode(IndexAmRoutine);

    am->amstrategies = 5;           /* <, <=, =, >=, > */
    am->amsupport = 1;              /* función de comparación */
    am->amoptsprocnum = 0;
    am->amcanorder = false;
    am->amcanorderbyop = false;
    am->amcanhash = false;
    am->amconsistentequality = false;
    am->amconsistentordering = false;
    am->amcanbackward = false;
    am->amcanunique = false;
    am->amcanmulticol = false;      /* una sola columna */
    am->amoptionalkey = false;      /* solo se usa si hay condición sobre la columna */
    am->amsearcharray = false;
    am->amsearchnulls = false;
    am->amstorage = false;
    am->amclusterable = false;
    am->ampredlocks = false;
    am->amcanparallel = false;
    am->amcanbuildparallel = false;
    am->amcaninclude = false;
    am->amusemaintenanceworkmem = false;
    am->amsummarizing = false;
    am->amparallelvacuumoptions = 0;
    am->amkeytype = InvalidOid;

    am->ambuild = skiplist_construir;
    am->ambuildempty = skiplist_construir_vacio;
    am->aminsert = skiplist_insertar;
    am->aminsertcleanup = NULL;
    am->ambulkdelete = skiplist_limpiar;
    am->amvacuumcleanup = skiplist_limpiar_final;
    am->amcanreturn = NULL;
    am->amcostestimate = skiplist_costo;
    am->amgettreeheight = NULL;
    am->amoptions = skiplist_opciones;
    am->amproperty = NULL;
    am->ambuildphasename = NULL;
    am->amvalidate = skiplist_validar;
    am->amadjustmembers = NULL;
    am->ambeginscan = skiplist_iniciar_busqueda;
    am->amrescan = skiplist_reiniciar_busqueda;
    am->amgettuple = skiplist_siguiente_fila;
    am->amgetbitmap = skiplist_todas_las_filas;
    am->amendscan = skiplist_terminar_busqueda;
    am->ammarkpos = NULL;
    am->amrestrpos = NULL;
    am->amestimateparallelscan = NULL;
    am->aminitparallelscan = NULL;
    am->amparallelrescan = NULL;
    am->amtranslatestrategy = NULL;
    am->amtranslatecmptype = NULL;

    PG_RETURN_POINTER(am);
}
