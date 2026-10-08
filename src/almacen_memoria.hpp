/*
 * almacen_memoria.hpp - Guarda los nodos de la Skip List en un arreglo.
 *
 * Se usa para probar el algoritmo sin PostgreSQL (semanas 2 y 3).
 * La referencia de un nodo es su posición en el arreglo + 1
 * (la posición 0 es el HEAD, por eso su referencia es SL_HEAD = 1).
 */
#ifndef ALMACEN_MEMORIA_HPP
#define ALMACEN_MEMORIA_HPP

#include <vector>
#include "nodo.h"

class AlmacenMemoria
{
public:
    AlmacenMemoria()
    {
        SLNodo head = {};
        head.clave = INT32_MIN;
        head.nivel = SL_MAX_NIVEL;
        nodos.push_back(head);
    }

    sl_ref crear(const SLNodo &n)
    {
        nodos.push_back(n);
        return (sl_ref) nodos.size();       /* posición + 1 */
    }

    void leer(sl_ref r, SLNodo &n) const { n = nodos[r - 1]; }

    void enlazar(sl_ref r, int nivel, sl_ref sig) { nodos[r - 1].sig[nivel] = sig; }

    int nivelActual() const { return nivel; }
    void fijarNivelActual(int n) { nivel = n; }

    size_t cantidad() const { return nodos.size() - 1; }   /* sin contar el HEAD */

private:
    std::vector<SLNodo> nodos;
    int nivel = 1;
};

#endif
