# Diseño de la Skip List (semana 1)

## Representación

| Elemento | Qué guarda |
|---|---|
| Nodo | clave (`recurso_id`), ubicación de la fila en la tabla y `sig[i]`, el siguiente nodo en cada nivel |
| HEAD | nodo inicial sin clave, con enlaces en todos los niveles |
| Nivel máximo | 16 |
| Probabilidad p | 0,5 (cada nodo tiene 50 % de probabilidad de subir un nivel) |
| Claves repetidas | se permiten |

## Ejemplo

```
nivel 2: HEAD ───────────► 7 ──────────────────────────► 31 ─────────► NULO
nivel 1: HEAD ───────────► 7 ──────────► 19 ───────────► 31 ─────────► NULO
nivel 0: HEAD ─► 3 ──────► 7 ─► 12 ────► 19 ─► 25 ─────► 31 ─► 40 ───► NULO
```

Para buscar 25: HEAD → 7 (nivel 2), se baja → 19 (nivel 1), se baja → 25 (nivel 0).

## Pseudocódigo

```
nivelAleatorio():
    nivel ← 1
    mientras "sale cara" y nivel < MAX_NIVEL:  nivel ← nivel + 1
    retornar nivel

buscar(clave):
    x ← HEAD
    para i desde nivelActual-1 hasta 0:
        mientras x.sig[i] ≠ NULO y x.sig[i].clave < clave:
            x ← x.sig[i]
    x ← x.sig[0]
    mientras x ≠ NULO y x.clave = clave:   agregar x a resultados;  x ← x.sig[0]

insertar(clave, ubicacion):
    x ← HEAD
    para i desde nivelActual-1 hasta 0:
        mientras x.sig[i] ≠ NULO y x.sig[i].clave < clave:  x ← x.sig[i]
        previo[i] ← x
    n ← nivelAleatorio()
    si n > nivelActual:  previo[nivelActual..n-1] ← HEAD;  nivelActual ← n
    nuevo ← crearNodo(clave, ubicacion, n)
    para i desde 0 hasta n-1:
        nuevo.sig[i] ← previo[i].sig[i]
        previo[i].sig[i] ← nuevo

construir(tabla):
    ordenar las claves
    ultimo[0..MAX-1] ← HEAD
    para cada clave en orden:
        nuevo ← crearNodo(clave, ubicacion, nivelAleatorio())
        para i desde 0 hasta nuevo.nivel-1:  ultimo[i].sig[i] ← nuevo;  ultimo[i] ← nuevo
```

## Complejidad esperada

| Operación | Promedio | Peor caso |
|---|---|---|
| Búsqueda | O(log n) | O(n) (muy improbable) |
| Inserción | O(log n) | O(n) |
| Construcción | O(n log n) (por el ordenamiento) | O(n log n) |
| Rango | O(log n + k) | O(n) |
| Espacio | O(n) (≈ 2 enlaces por nodo) | O(n · MAX_NIVEL) |

## Integración con PostgreSQL

La Skip List se instala como una **extensión** que agrega un tipo de índice nuevo (`CREATE ACCESS METHOD`). PostgreSQL está escrito en C, así que una capa pequeña en C recibe sus pedidos y llama al código C++.
