/*
 * puente.h - Funciones de la Skip List (C++) que se pueden llamar desde C.
 */
#ifndef PUENTE_H
#define PUENTE_H

#include <stddef.h>
#include "nodo.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Construye la Skip List con todas las entradas (las ordena). */
void   sl_construir(void *indice, SLEntrada *entradas, size_t n);

/* Inserta una clave nueva. */
void   sl_insertar(void *indice, int32_t clave, SLUbicacion ubicacion);

/* Primer nodo con clave >= clave (punto de partida de una búsqueda). */
sl_ref sl_primero_mayor_igual(void *indice, int32_t clave);

/* Lee un nodo y devuelve la referencia del siguiente en el nivel 0. */
sl_ref sl_siguiente(void *indice, sl_ref ref, SLNodo *nodo);

#ifdef __cplusplus
}
#endif

#endif
