/*
 * almacen_paginas.h - Funciones (en C) que guardan los nodos de la Skip List
 * en las páginas del índice de PostgreSQL. Las usa puente.cpp.
 *
 * "indice" es la Relation del índice (se pasa como void* para que el código
 * C++ no necesite incluir los encabezados de PostgreSQL).
 */
#ifndef ALMACEN_PAGINAS_H
#define ALMACEN_PAGINAS_H

#include "nodo.h"

#ifdef __cplusplus
extern "C" {
#endif

sl_ref slpg_crear(void *indice, const SLNodo *nodo);
void   slpg_leer(void *indice, sl_ref ref, SLNodo *nodo);
void   slpg_enlazar(void *indice, sl_ref ref, int nivel, sl_ref sig);
int    slpg_nivel_actual(void *indice);
void   slpg_fijar_nivel_actual(void *indice, int nivel);

#ifdef __cplusplus
}
#endif

#endif
