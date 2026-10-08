/*
 * puente.cpp - Conecta la Skip List (C++) con PostgreSQL (C).
 *
 * AlmacenPG cumple el papel de "almacén" que espera SkipList<Almacen>,
 * pero en vez de usar un arreglo en memoria llama a las funciones de
 * almacen_paginas.c, que guardan los nodos en las páginas del índice.
 *
 * Las funciones marcadas extern "C" son las que llama skiplist_am.c.
 * Nota: este archivo no usa excepciones ni objetos con destructores, porque
 * PostgreSQL maneja sus errores saltando directamente fuera de la función.
 */
#include "skiplist.hpp"
#include "almacen_paginas.h"
#include "puente.h"

class AlmacenPG
{
public:
    explicit AlmacenPG(void *indice) : indice(indice) {}

    sl_ref crear(const SLNodo &n) { return slpg_crear(indice, &n); }
    void leer(sl_ref r, SLNodo &n) { slpg_leer(indice, r, &n); }
    void enlazar(sl_ref r, int nivel, sl_ref sig) { slpg_enlazar(indice, r, nivel, sig); }
    int nivelActual() { return slpg_nivel_actual(indice); }
    void fijarNivelActual(int nivel) { slpg_fijar_nivel_actual(indice, nivel); }

private:
    void *indice;
};

extern "C" void
sl_construir(void *indice, SLEntrada *entradas, size_t n)
{
    AlmacenPG almacen(indice);
    SkipList<AlmacenPG> sl(almacen);
    sl.construir(entradas, n);
}

extern "C" void
sl_insertar(void *indice, int32_t clave, SLUbicacion ubicacion)
{
    AlmacenPG almacen(indice);
    SkipList<AlmacenPG> sl(almacen);
    sl.insertar(clave, ubicacion);
}

extern "C" sl_ref
sl_primero_mayor_igual(void *indice, int32_t clave)
{
    AlmacenPG almacen(indice);
    SkipList<AlmacenPG> sl(almacen);
    return sl.primeroMayorOIgual(clave);
}

extern "C" sl_ref
sl_siguiente(void *indice, sl_ref ref, SLNodo *nodo)
{
    AlmacenPG almacen(indice);
    SkipList<AlmacenPG> sl(almacen);
    return sl.siguiente(ref, *nodo);
}
