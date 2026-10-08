/*
 * skiplist.hpp - Implementación propia de la Skip List (C++).
 *
 * El algoritmo no sabe dónde se guardan los nodos: eso lo decide el
 * "almacén" (Almacen) que recibe como parámetro.
 *   - AlmacenMemoria (src/almacen_memoria.hpp): nodos en un arreglo,
 *     se usa para probar el algoritmo sin PostgreSQL.
 *   - AlmacenPG (extension/puente.cpp): nodos guardados en las páginas
 *     del índice de PostgreSQL.
 * Así, el MISMO código de construcción, búsqueda e inserción se prueba en
 * memoria y luego se usa dentro de PostgreSQL.
 *
 * Lo que debe ofrecer un almacén:
 *   sl_ref crear(const SLNodo &n)              guarda un nodo nuevo y devuelve su referencia
 *   void   leer(sl_ref r, SLNodo &n)           copia el nodo r en n (SL_HEAD = nodo inicial)
 *   void   enlazar(sl_ref r, int i, sl_ref s)  hace r.sig[i] = s
 *   int    nivelActual()                       nivel más alto en uso
 *   void   fijarNivelActual(int nivel)
 */
#ifndef SKIPLIST_HPP
#define SKIPLIST_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "nodo.h"

template <class Almacen>
class SkipList
{
public:
    explicit SkipList(Almacen &almacen) : a(almacen) {}

    /*
     * Nivel "aleatorio" de un nodo nuevo (probabilidad p = 0,5 de subir).
     * Se calcula con una función hash de la clave y la ubicación, así el
     * resultado es pseudoaleatorio pero REPRODUCIBLE: los mismos datos
     * siempre generan la misma Skip List.
     */
    static int nivelAleatorio(int32_t clave, SLUbicacion u)
    {
        uint64_t x = ((uint64_t)(uint32_t) clave << 32) ^ ((uint64_t) u.bloque << 16) ^ u.posicion;
        /* splitmix64: mezcla los bits */
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        x = x ^ (x >> 31);
        /* cada bit en 1 equivale a "salió cara": se sube un nivel */
        int nivel = 1;
        while ((x & 1) && nivel < SL_MAX_NIVEL)
        {
            nivel++;
            x >>= 1;
        }
        return nivel;
    }

    /* ------------------------------------------------------------------
     * CONSTRUCCIÓN (semana 2)
     * Recibe todas las claves de la tabla, las ordena y arma la lista de
     * izquierda a derecha. Como las claves ya están ordenadas, cada nodo
     * nuevo se engancha al último nodo de cada uno de sus niveles: O(n log n)
     * por el ordenamiento y O(n) para enlazar.
     * ------------------------------------------------------------------ */
    void construir(SLEntrada *entradas, size_t n)
    {
        std::sort(entradas, entradas + n, [](const SLEntrada &x, const SLEntrada &y) {
            return x.clave < y.clave;
        });

        sl_ref ultimo[SL_MAX_NIVEL];        /* último nodo visto en cada nivel */
        for (int i = 0; i < SL_MAX_NIVEL; i++)
            ultimo[i] = SL_HEAD;

        int nivelMaximo = 1;
        for (size_t k = 0; k < n; k++)
        {
            SLNodo nodo = nuevoNodo(entradas[k].clave, entradas[k].ubicacion);
            sl_ref ref = a.crear(nodo);
            for (int i = 0; i < nodo.nivel; i++)
            {
                a.enlazar(ultimo[i], i, ref);
                ultimo[i] = ref;
            }
            nivelMaximo = std::max(nivelMaximo, (int) nodo.nivel);
        }
        a.fijarNivelActual(nivelMaximo);
    }

    /* ------------------------------------------------------------------
     * BÚSQUEDA (semana 2)
     * Empieza en el nivel más alto del HEAD, avanza mientras la siguiente
     * clave sea menor y baja un nivel cuando ya no puede avanzar.
     * Devuelve el primer nodo con clave >= buscada (o SL_NULO).
     * ------------------------------------------------------------------ */
    sl_ref primeroMayorOIgual(int32_t clave)
    {
        SLNodo x, sig;
        a.leer(SL_HEAD, x);
        for (int i = a.nivelActual() - 1; i >= 0; i--)
        {
            while (x.sig[i] != SL_NULO)
            {
                a.leer(x.sig[i], sig);
                if (sig.clave >= clave)
                    break;
                x = sig;                    /* avanzar en el mismo nivel */
            }
            /* no se puede avanzar más: se baja al nivel i-1 */
        }
        return x.sig[0];
    }

    /*
     * Busca una clave y llama a alEncontrar(nodo) por cada coincidencia
     * (puede haber claves repetidas). Devuelve cuántas encontró; 0 = no existe.
     */
    template <class F>
    size_t buscar(int32_t clave, F alEncontrar)
    {
        return recorrerRango(clave, clave, alEncontrar);
    }


    /* Avanza al siguiente nodo del nivel 0 (lo usa el escaneo de PostgreSQL). */
    sl_ref siguiente(sl_ref ref, SLNodo &nodo)
    {
        a.leer(ref, nodo);
        return nodo.sig[0];
    }

private:
    Almacen &a;

    static SLNodo nuevoNodo(int32_t clave, SLUbicacion ubicacion)
    {
        SLNodo n;
        n.clave = clave;
        n.nivel = (uint16_t) nivelAleatorio(clave, ubicacion);
        n.borrado = 0;
        n.ubicacion = ubicacion;
        for (int i = 0; i < SL_MAX_NIVEL; i++)
            n.sig[i] = SL_NULO;
        return n;
    }

    template <class F>
    size_t recorrerRango(int32_t desde, int32_t hasta, F alEncontrar)
    {
        size_t encontrados = 0;
        SLNodo nodo;
        for (sl_ref r = primeroMayorOIgual(desde); r != SL_NULO; r = nodo.sig[0])
        {
            a.leer(r, nodo);
            if (nodo.clave > hasta)
                break;
            if (!nodo.borrado)
            {
                alEncontrar(nodo);
                encontrados++;
            }
        }
        return encontrados;
    }
};

#endif
