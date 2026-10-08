/*
 * skiplist_meta.h - Formato de la metapágina (página 0) del índice.
 * Solo lo usan los archivos en C.
 */
#ifndef SKIPLIST_META_H
#define SKIPLIST_META_H

#include "postgres.h"
#include "storage/bufpage.h"
#include "utils/rel.h"
#include "nodo.h"

#define SL_PAGINA_META  0
#define SL_MAGICO       0x534B4950      /* "SKIP": para reconocer el índice */

typedef struct SLMeta
{
    uint32      magico;
    uint32      nivelActual;            /* nivel más alto en uso */
    BlockNumber ultimaPagina;           /* página donde se agregan nodos nuevos */
    uint32      relleno;
    uint64      cantidadNodos;
    sl_ref      headSig[SL_MAX_NIVEL];  /* enlaces del HEAD en cada nivel */
} SLMeta;

#define SL_META(pagina) ((SLMeta *) PageGetContents(pagina))

void slpg_iniciar_meta(Relation indice);

#endif
