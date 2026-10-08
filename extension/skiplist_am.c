/*
 * skiplist_am.c - Registra la Skip List como un nuevo tipo de índice
 * ("método de acceso") de PostgreSQL.
 *
 * SEMANA 3: esqueleto. PostgreSQL ya reconoce el tipo de índice "skiplist",
 * pero las funciones todavía no hacen nada (se completan en la semana 4).
 */
#include "postgres.h"

#include "access/amapi.h"
#include "access/genam.h"
#include "access/relscan.h"
#include "nodes/execnodes.h"
#include "nodes/pathnodes.h"
#include "utils/rel.h"

PG_MODULE_MAGIC;

#define PENDIENTE() \
    ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
                    errmsg("skiplist: pendiente (se implementa en la semana 4)")))

static IndexBuildResult *
skiplist_construir(Relation tabla, Relation indice, IndexInfo *indexInfo)
{
    PENDIENTE();
    return NULL;
}

static void
skiplist_construir_vacio(Relation indice)
{
    PENDIENTE();
}

static bool
skiplist_insertar(Relation indice, Datum *valores, bool *esNulo,
                  ItemPointer tid, Relation tabla, IndexUniqueCheck checkUnique,
                  bool indexUnchanged, IndexInfo *indexInfo)
{
    PENDIENTE();
    return false;
}

static IndexScanDesc
skiplist_iniciar_busqueda(Relation indice, int nclaves, int norderbys)
{
    PENDIENTE();
    return NULL;
}

static void
skiplist_reiniciar_busqueda(IndexScanDesc scan, ScanKey claves, int nclaves,
                            ScanKey orderbys, int norderbys)
{
    PENDIENTE();
}

static bool
skiplist_siguiente_fila(IndexScanDesc scan, ScanDirection dir)
{
    PENDIENTE();
    return false;
}

static void
skiplist_terminar_busqueda(IndexScanDesc scan)
{
}

static IndexBulkDeleteResult *
skiplist_limpiar(IndexVacuumInfo *info, IndexBulkDeleteResult *stats,
                 IndexBulkDeleteCallback callback, void *estado)
{
    return stats;
}

static IndexBulkDeleteResult *
skiplist_limpiar_final(IndexVacuumInfo *info, IndexBulkDeleteResult *stats)
{
    return stats;
}

static void
skiplist_costo(PlannerInfo *root, IndexPath *path, double loop_count,
               Cost *costoInicial, Cost *costoTotal, Selectivity *selectividad,
               double *correlacion, double *paginas)
{
    /* costo muy alto: el planificador nunca elegirá este índice todavía */
    *costoInicial = 1.0e10;
    *costoTotal = 1.0e10;
    *selectividad = 1.0;
    *correlacion = 0.0;
    *paginas = 1.0;
}

static bytea *
skiplist_opciones(Datum reloptions, bool validate)
{
    return NULL;
}

static bool
skiplist_validar(Oid opclassoid)
{
    return true;
}

/* HANDLER: le dice a PostgreSQL qué sabe hacer este índice y qué función
 * llamar en cada caso. Es lo que se registra con CREATE ACCESS METHOD. */
PG_FUNCTION_INFO_V1(skiplist_handler);

Datum
skiplist_handler(PG_FUNCTION_ARGS)
{
    IndexAmRoutine *am = makeNode(IndexAmRoutine);   /* todo en 0 / false / NULL */

    am->amstrategies = 5;           /* <, <=, =, >=, > */
    am->amsupport = 1;              /* función de comparación */
    am->amkeytype = InvalidOid;

    am->ambuild = skiplist_construir;
    am->ambuildempty = skiplist_construir_vacio;
    am->aminsert = skiplist_insertar;
    am->ambulkdelete = skiplist_limpiar;
    am->amvacuumcleanup = skiplist_limpiar_final;
    am->amcostestimate = skiplist_costo;
    am->amoptions = skiplist_opciones;
    am->amvalidate = skiplist_validar;
    am->ambeginscan = skiplist_iniciar_busqueda;
    am->amrescan = skiplist_reiniciar_busqueda;
    am->amgettuple = skiplist_siguiente_fila;
    am->amendscan = skiplist_terminar_busqueda;

    PG_RETURN_POINTER(am);
}
