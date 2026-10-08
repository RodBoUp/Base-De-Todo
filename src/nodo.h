/*
 * nodo.h - Definición del nodo de la Skip List.
 *
 * Este archivo se comparte entre C++ (la Skip List) y C (la parte que habla
 * con PostgreSQL), por eso solo usa tipos simples de C.
 */
#ifndef SKIPLIST_NODO_H
#define SKIPLIST_NODO_H

#include <stdint.h>

/* Cantidad máxima de niveles. Con p = 0,5 alcanza para millones de claves. */
#define SL_MAX_NIVEL 16

/*
 * Una "referencia" identifica a un nodo dentro del almacenamiento.
 *   - En memoria: es la posición del nodo dentro de un arreglo.
 *   - En PostgreSQL: es (número de página, posición dentro de la página).
 * El valor 0 significa "no hay nodo" (NULO).
 */
typedef uint64_t sl_ref;
#define SL_NULO ((sl_ref) 0)
#define SL_HEAD ((sl_ref) 1)   /* el nodo inicial siempre tiene la referencia 1 */

/* Ubicación de una fila en la tabla (equivale al TID de PostgreSQL). */
typedef struct SLUbicacion
{
    uint32_t bloque;    /* página de la tabla */
    uint16_t posicion;  /* posición de la fila dentro de esa página */
} SLUbicacion;

/* Un nodo de la Skip List. */
typedef struct SLNodo
{
    int32_t     clave;                /* valor indexado (recurso_id) */
    uint16_t    nivel;                /* cuántos niveles tiene este nodo (1..SL_MAX_NIVEL) */
    uint16_t    borrado;              /* 1 si la fila ya no existe (lo marca VACUUM) */
    SLUbicacion ubicacion;            /* dónde está la fila en la tabla */
    sl_ref      sig[SL_MAX_NIVEL];    /* enlace al siguiente nodo en cada nivel */
} SLNodo;

/* Par (clave, ubicación) que se usa para construir la Skip List. */
typedef struct SLEntrada
{
    int32_t     clave;
    SLUbicacion ubicacion;
} SLEntrada;

#endif
