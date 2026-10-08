/*
 * test_skiplist.cpp - Pruebas de la Skip List en memoria (sin PostgreSQL).
 *
 * Compilar y ejecutar:   make -C pruebas
 */
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <vector>
#include "skiplist.hpp"
#include "almacen_memoria.hpp"

static int fallas = 0;
#define VERIFICAR(cond, msg) \
    do { if (!(cond)) { std::printf("  [FALLA] %s\n", msg); fallas++; } } while (0)

typedef SkipList<AlmacenMemoria> SL;

static SLUbicacion ubic(uint32_t i) { return SLUbicacion{i / 100, (uint16_t)(i % 100 + 1)}; }

/* Revisa que en cada nivel las claves estén ordenadas y que cada nivel
 * sea un subconjunto del nivel de abajo. */
static bool estructuraCorrecta(AlmacenMemoria &a)
{
    SLNodo head, n;
    a.leer(SL_HEAD, head);
    size_t anterior = (size_t) -1;
    for (int i = 0; i < a.nivelActual(); i++)
    {
        size_t cuenta = 0;
        int32_t ultima = INT32_MIN;
        for (sl_ref r = head.sig[i]; r != SL_NULO; r = n.sig[i])
        {
            a.leer(r, n);
            if (n.clave < ultima || n.nivel <= i)
                return false;
            ultima = n.clave;
            cuenta++;
        }
        if (cuenta > anterior)
            return false;
        anterior = cuenta;
    }
    return true;
}

/* Muestra la Skip List por niveles (solo para listas pequeñas). */
static void imprimir(AlmacenMemoria &a)
{
    SLNodo head, n;
    a.leer(SL_HEAD, head);
    for (int i = a.nivelActual() - 1; i >= 0; i--)
    {
        std::printf("  nivel %2d: HEAD", i);
        for (sl_ref r = head.sig[i]; r != SL_NULO; r = n.sig[i])
        {
            a.leer(r, n);
            std::printf(" -> %d", n.clave);
        }
        std::printf(" -> NULO\n");
    }
}

static void pruebaEjemplo()
{
    std::printf("Prueba 1: ejemplo pequeño\n");
    AlmacenMemoria a;
    SL sl(a);
    std::vector<SLEntrada> e;
    int claves[] = {25, 3, 40, 12, 7, 31, 19};
    for (int i = 0; i < 7; i++)
        e.push_back(SLEntrada{claves[i], ubic(i)});
    sl.construir(e.data(), e.size());
    imprimir(a);
    VERIFICAR(estructuraCorrecta(a), "estructura ordenada");
    VERIFICAR(sl.buscar(25, [](const SLNodo &) {}) == 1, "encuentra 25");
    VERIFICAR(sl.buscar(26, [](const SLNodo &) {}) == 0, "no encuentra 26");
}

static void pruebaConstruccionYBusqueda(size_t n)
{
    std::printf("Prueba 2: construcción y búsqueda con %zu claves aleatorias\n", n);
    std::mt19937 rng(42);                                  /* semilla fija */
    std::uniform_int_distribution<int32_t> dist(1, (int32_t)(n * 10));
    std::multimap<int32_t, uint32_t> esperado;             /* referencia para comparar */
    std::vector<SLEntrada> e;
    for (uint32_t i = 0; i < n; i++)
    {
        int32_t c = dist(rng);
        e.push_back(SLEntrada{c, ubic(i)});
        esperado.insert({c, i});
    }
    AlmacenMemoria a;
    SL sl(a);
    sl.construir(e.data(), e.size());
    VERIFICAR(a.cantidad() == n, "cantidad de nodos");
    VERIFICAR(estructuraCorrecta(a), "estructura ordenada");
    std::printf("  niveles usados: %d\n", a.nivelActual());

    int malas = 0;
    for (int k = 0; k < 2000; k++)
    {
        int32_t c = dist(rng);
        if (sl.buscar(c, [](const SLNodo &) {}) != esperado.count(c))
            malas++;
    }
    VERIFICAR(malas == 0, "búsquedas existentes y no existentes coinciden con la referencia");
    VERIFICAR(sl.buscar(-5, [](const SLNodo &) {}) == 0, "clave menor que todas");
    VERIFICAR(sl.buscar((int32_t)(n * 10 + 1), [](const SLNodo &) {}) == 0, "clave mayor que todas");
}


int main()
{
    pruebaEjemplo();
    pruebaConstruccionYBusqueda(100000);
    if (fallas == 0)
        std::printf("\nTODAS LAS PRUEBAS PASARON\n");
    else
        std::printf("\n%d PRUEBA(S) FALLARON\n", fallas);
    return fallas == 0 ? 0 : 1;
}
