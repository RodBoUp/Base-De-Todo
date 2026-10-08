/*
 * almacen_paginas.c - Guarda los nodos de la Skip List en las páginas del
 * índice, usando el administrador de buffers de PostgreSQL.
 *
 * Organización del archivo del índice:
 *   - Página 0 (metapágina): nivel actual, enlaces del HEAD, última página usada.
 *   - Páginas 1, 2, ...: los nodos, uno detrás de otro.
 *
 * Referencia de un nodo = (número de página << 16) | posición en la página.
 * El HEAD vive en la metapágina y su referencia es SL_HEAD (= 1).
 *
 * Cada nodo ocupa 16 bytes + 8 bytes por nivel (solo se guardan los enlaces
 * que el nodo realmente usa).
 */
#include "postgres.h"

#include <stddef.h>

#include "storage/bufmgr.h"
#include "storage/bufpage.h"
#include "utils/rel.h"

#include "almacen_paginas.h"
#include "skiplist_meta.h"

#define REF_PAGINA(r)      ((BlockNumber) ((r) >> 16))
#define REF_POSICION(r)    ((OffsetNumber) ((r) & 0xFFFF))
#define HACER_REF(b, o)    ((((sl_ref) (b)) << 16) | (sl_ref) (o))

/* Bytes que ocupa en disco un nodo de "nivel" niveles. */
#define TAMANO_NODO(nivel) (offsetof(SLNodo, sig) + (nivel) * sizeof(sl_ref))

/* Crea la metapágina vacía (se llama al empezar CREATE INDEX). */
void
slpg_iniciar_meta(Relation indice)
{
    Buffer      buf = ExtendBufferedRel(BMR_REL(indice), MAIN_FORKNUM, NULL, EB_LOCK_FIRST);
    Page        pagina = BufferGetPage(buf);
    SLMeta     *meta;

    Assert(BufferGetBlockNumber(buf) == SL_PAGINA_META);
    PageInit(pagina, BLCKSZ, 0);
    meta = SL_META(pagina);
    memset(meta, 0, sizeof(SLMeta));
    meta->magico = SL_MAGICO;
    meta->nivelActual = 1;
    meta->ultimaPagina = SL_PAGINA_META;   /* aún no hay páginas de nodos */
    ((PageHeader) pagina)->pd_lower += sizeof(SLMeta);

    MarkBufferDirty(buf);
    UnlockReleaseBuffer(buf);
}

/* Agrega una página nueva y vacía al final del índice (debe tener la meta bloqueada). */
static Buffer
nueva_pagina(Relation indice)
{
    Buffer      buf = ExtendBufferedRel(BMR_REL(indice), MAIN_FORKNUM, NULL, EB_LOCK_FIRST);

    PageInit(BufferGetPage(buf), BLCKSZ, 0);
    return buf;
}

sl_ref
slpg_crear(void *ptr, const SLNodo *nodo)
{
    Relation    indice = (Relation) ptr;
    Size        tam = TAMANO_NODO(nodo->nivel);
    Buffer      bufMeta, buf;
    SLMeta     *meta;
    Page        pagina;
    OffsetNumber pos;
    sl_ref      ref;

    bufMeta = ReadBuffer(indice, SL_PAGINA_META);
    LockBuffer(bufMeta, BUFFER_LOCK_EXCLUSIVE);
    meta = SL_META(BufferGetPage(bufMeta));

    /* Intentar en la última página usada; si no hay espacio, crear otra. */
    if (meta->ultimaPagina == SL_PAGINA_META)
        buf = nueva_pagina(indice);
    else
    {
        buf = ReadBuffer(indice, meta->ultimaPagina);
        LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
        if (PageGetFreeSpace(BufferGetPage(buf)) < MAXALIGN(tam))
        {
            UnlockReleaseBuffer(buf);
            buf = nueva_pagina(indice);
        }
    }

    pagina = BufferGetPage(buf);
    pos = PageAddItem(pagina, (Item) nodo, tam, InvalidOffsetNumber, false, false);
    if (pos == InvalidOffsetNumber)
        elog(ERROR, "skiplist: no se pudo agregar el nodo a la página");
    ref = HACER_REF(BufferGetBlockNumber(buf), pos);

    meta->ultimaPagina = BufferGetBlockNumber(buf);
    meta->cantidadNodos++;

    MarkBufferDirty(buf);
    UnlockReleaseBuffer(buf);
    MarkBufferDirty(bufMeta);
    UnlockReleaseBuffer(bufMeta);
    return ref;
}

void
slpg_leer(void *ptr, sl_ref ref, SLNodo *nodo)
{
    Relation    indice = (Relation) ptr;
    BlockNumber pag = REF_PAGINA(ref);
    Buffer      buf = ReadBuffer(indice, pag);
    Page        pagina;

    LockBuffer(buf, BUFFER_LOCK_SHARE);
    pagina = BufferGetPage(buf);

    if (ref == SL_HEAD)
    {
        SLMeta     *meta = SL_META(pagina);

        memset(nodo, 0, sizeof(SLNodo));
        nodo->clave = PG_INT32_MIN;
        nodo->nivel = SL_MAX_NIVEL;
        memcpy(nodo->sig, meta->headSig, sizeof(meta->headSig));
    }
    else
    {
        SLNodo     *enPagina = (SLNodo *) PageGetItem(pagina, PageGetItemId(pagina, REF_POSICION(ref)));

        memcpy(nodo, enPagina, TAMANO_NODO(enPagina->nivel));
        for (int i = enPagina->nivel; i < SL_MAX_NIVEL; i++)
            nodo->sig[i] = SL_NULO;
    }
    UnlockReleaseBuffer(buf);
}

void
slpg_enlazar(void *ptr, sl_ref ref, int nivel, sl_ref sig)
{
    Relation    indice = (Relation) ptr;
    Buffer      buf = ReadBuffer(indice, REF_PAGINA(ref));
    Page        pagina;

    LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
    pagina = BufferGetPage(buf);
    if (ref == SL_HEAD)
        SL_META(pagina)->headSig[nivel] = sig;
    else
    {
        SLNodo     *enPagina = (SLNodo *) PageGetItem(pagina, PageGetItemId(pagina, REF_POSICION(ref)));

        Assert(nivel < enPagina->nivel);
        enPagina->sig[nivel] = sig;
    }
    MarkBufferDirty(buf);
    UnlockReleaseBuffer(buf);
}

int
slpg_nivel_actual(void *ptr)
{
    Relation    indice = (Relation) ptr;
    Buffer      buf = ReadBuffer(indice, SL_PAGINA_META);
    int         nivel;

    LockBuffer(buf, BUFFER_LOCK_SHARE);
    nivel = SL_META(BufferGetPage(buf))->nivelActual;
    UnlockReleaseBuffer(buf);
    return nivel;
}

void
slpg_fijar_nivel_actual(void *ptr, int nivel)
{
    Relation    indice = (Relation) ptr;
    Buffer      buf = ReadBuffer(indice, SL_PAGINA_META);

    LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
    SL_META(BufferGetPage(buf))->nivelActual = nivel;
    MarkBufferDirty(buf);
    UnlockReleaseBuffer(buf);
}
