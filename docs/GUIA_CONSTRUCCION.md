# Guía de construcción – Etapa 1 (semanas 1 a 5)

Esta guía explica, semana por semana, cómo se construye el proyecto. Cada semana termina con algo que se puede ejecutar y mostrar, y queda guardada en Git con una etiqueta: `semana-1`, `semana-2`, etc.

Para ver cómo estaba el proyecto al final de una semana:

```bash
git checkout semana-3      # ver el proyecto al final de la semana 3
git checkout main          # volver a la versión final
```

**La idea general en una frase:** primero se programa y prueba la Skip List sola, en C++ y en memoria (semanas 2 y 3). Después se conecta a PostgreSQL para que la use como índice (semana 4). Al final se demuestra y se compara con B-tree (semana 5).

```
Semana 1        Semana 2             Semana 3               Semana 4                Semana 5
Diseño   ──►  Construir + buscar ──► Insertar + rango  ──►  PostgreSQL usa el  ──►  Demo + comparación
(papel)       (C++, en memoria)      + esqueleto extensión  índice (CREATE INDEX)   con B-tree
```

---

## Semana 1 – Comprensión y diseño

**Lo que pide el curso:** propuesta aprobada, referencias, representación de la estructura, pseudocódigo y estrategia de integración.

| Quién | Qué hace |
|---|---|
| Marccelo (1.1) | Lee la guía del curso y el artículo de Pugh (1990). Resume los requisitos. |
| Rodrigo (1.2) | Dibuja la Skip List: nodos, niveles, punteros y HEAD. Escribe el pseudocódigo. |
| Camila (1.3) | Define la clave (`recurso_id`, INTEGER), los datos y las consultas de prueba. |
| Pedro (1.4) | Investiga cómo se agrega un índice nuevo a PostgreSQL (extensión). |
| Nayalem (1.5) | Crea el repositorio, las carpetas, el README y el cronograma. |

**Pasos**

1. Crear el repositorio en GitHub e invitar a los 5 integrantes.
2. Crear las carpetas: `src/`, `extension/`, `pruebas/`, `sql/`, `datos/`, `experimentos/`, `docker/` y `docs/`.
3. Subir la propuesta a `docs/` y escribir el README inicial.
4. Hacer en papel el dibujo de una Skip List pequeña (por ejemplo, con 3, 7, 12, 19, 25, 31 y 40) y buscar a mano la clave 25.
5. Escribir el pseudocódigo de construir, buscar e insertar (está en el informe de la semana 1).

**Cómo se comprueba:** la propuesta está entregada, el repositorio existe y todos entienden el dibujo de la Skip List.

---

## Semana 2 – La Skip List funciona sola (construcción y búsqueda)

**Lo que pide el curso:** construcción y búsqueda funcionando, con pruebas.

| Quién | Qué hace |
|---|---|
| Rodrigo (2.1) | `src/nodo.h`: cómo es un nodo. `src/almacen_memoria.hpp`: dónde se guardan los nodos para las pruebas. |
| Nayalem (2.2) | Función `construir()` en `src/skiplist.hpp`. |
| Camila (2.3, adelantada) | Función `primeroMayorOIgual()` / `buscar()` y sus pruebas. |
| Pedro (2.4) | Empieza a estudiar la inserción. |
| Todos | Preparan Docker. |

**Pasos**

1. **El nodo** (`src/nodo.h`). Guarda la clave, su nivel, la ubicación de la fila en la tabla y un arreglo `sig[]` con el siguiente nodo en cada nivel. Usa como máximo 16 niveles.
2. **El almacén en memoria** (`src/almacen_memoria.hpp`). Es un arreglo de nodos: la posición 0 es el HEAD. Ofrece 5 operaciones: `crear`, `leer`, `enlazar`, `nivelActual` y `fijarNivelActual`.
3. **El nivel aleatorio** (`nivelAleatorio()` en `src/skiplist.hpp`). Es como lanzar una moneda: mientras salga "cara" se sube un nivel. Se usa una función hash para que el resultado sea siempre el mismo con los mismos datos (reproducible).
4. **La construcción** (`construir()`). Ordena las claves y las recorre de menor a mayor. Cada nodo nuevo se engancha al último nodo de cada nivel que tenga.
5. **La búsqueda** (`primeroMayorOIgual()` y `buscar()`). Empieza arriba en el HEAD, avanza mientras la siguiente clave sea menor y, cuando ya no puede, baja un nivel.
6. **Las pruebas** (`pruebas/test_skiplist.cpp`). Construyen 100 000 claves aleatorias con semilla fija. Comparan cada búsqueda con una estructura de referencia (`std::multimap`) y verifican que cada nivel esté ordenado.
7. **Docker** (`docker/Dockerfile` y `docker-compose.yml`). Usan PostgreSQL 18.6 con las herramientas para compilar.

**Cómo se comprueba**

```bash
docker compose up -d --build
docker compose exec db make -C pruebas
```

Debe mostrar la Skip List de ejemplo por niveles y terminar con `TODAS LAS PRUEBAS PASARON`.

**Para explicar en clase:** ¿por qué la búsqueda es O(log n)? Porque cada nivel tiene más o menos la mitad de nodos que el de abajo, así que en cada nivel se avanzan pocos pasos.

---

## Semana 3 – Inserción, rango y esqueleto de la extensión

**Lo que pide el curso:** construcción + búsqueda + inserción, y una demo breve del código.

| Quién | Qué hace |
|---|---|
| Pedro (2.4) | Función `insertar()`. |
| Marccelo (2.5) | Función `buscarRango()` y sus pruebas. |
| Camila (2.3) | Pruebas de búsqueda con claves que existen y que no. |
| Rodrigo y Nayalem (3.1) | Esqueleto de la extensión de PostgreSQL. |

**Pasos**

1. **Inserción** (`insertar()`). Se baja por los niveles como en la búsqueda, guardando en `previo[i]` el último nodo visitado en cada nivel. Luego se sortea el nivel del nodo nuevo y se enlaza después de `previo[i]`, empezando por el nivel 0.
2. **Rango** (`buscarRango()`). Se ubica el primer nodo mayor o igual a `desde` y se avanza por el nivel 0 hasta pasar `hasta`.
3. **Más pruebas.** Se insertan 100 000 claves en desorden, se verifica que todo siga ordenado y se prueban 500 rangos.
4. **Esqueleto de la extensión** (`extension/`):
   - `skiplist.control`: nombre y versión de la extensión.
   - `skiplist--1.0.sql`: crea el tipo de índice (`CREATE ACCESS METHOD`) y dice qué operadores entiende (`<`, `<=`, `=`, `>=`, `>`).
   - `skiplist_am.c`: la función `skiplist_handler`, que le dice a PostgreSQL qué funciones llamar. En esta semana esas funciones solo responden "pendiente".
   - `Makefile`: compila con la herramienta de PostgreSQL (PGXS).

**Cómo se comprueba**

```bash
docker compose exec db make -C pruebas                 # TODAS LAS PRUEBAS PASARON
docker compose exec db make -C extension install
docker compose exec db psql -U postgres -c "CREATE EXTENSION skiplist;"
```

`CREATE EXTENSION` funciona. Un `CREATE INDEX ... USING skiplist` todavía responde "pendiente (semana 4)".

---

## Semana 4 – PostgreSQL usa la Skip List como índice

**Lo que pide el curso:** primera demostración funcional desde PostgreSQL y corrección de errores.

| Quién | Qué hace |
|---|---|
| Rodrigo y Nayalem (3.1) | `almacen_paginas.c`, `puente.cpp` y las funciones de `skiplist_am.c`. |
| Pedro (3.2) | Tabla `recurso`, datos y `sql/demo.sql`. |
| Marccelo (2.5) | Borrado lógico con VACUUM. |
| Camila | `pruebas/prueba_integracion.sql`. |

**La idea clave:** el código de la Skip List **no cambia**. Solo se cambia el almacén: en vez de un arreglo en memoria, los nodos se guardan en las **páginas del índice** de PostgreSQL. Así el índice sigue existiendo aunque se reinicie el servidor.

**Pasos**

1. **Almacén en páginas** (`extension/almacen_paginas.c`). Tiene las mismas 5 operaciones que el almacén en memoria, pero usando páginas:
   - La página 0 (metapágina) guarda el HEAD, el nivel actual y la última página usada.
   - Las páginas 1, 2, … guardan los nodos. La referencia de un nodo es (número de página, posición).
2. **Puente C ↔ C++** (`extension/puente.cpp`). Crea `SkipList<AlmacenPG>` y ofrece funciones simples en C: `sl_construir`, `sl_insertar`, `sl_primero_mayor_igual` y `sl_siguiente`.
3. **Las funciones que llama PostgreSQL** (`extension/skiplist_am.c`):

   | Cuando el usuario hace… | PostgreSQL llama a… | Que usa… |
   |---|---|---|
   | `CREATE INDEX` | `skiplist_construir` | `sl_construir` (ordena y enlaza) |
   | `INSERT` | `skiplist_insertar` | `sl_insertar` |
   | `SELECT … WHERE` | `skiplist_reiniciar_busqueda` + `skiplist_siguiente_fila` | `sl_primero_mayor_igual` + `sl_siguiente` |
   | `VACUUM` | `skiplist_limpiar` | marca nodos como borrados |
   | (planificar) | `skiplist_costo` | estima si conviene usar el índice |

   Todas las condiciones (`=`, `<`, `>`, `BETWEEN`) se convierten en un rango `[desde, hasta]`.
4. **Demo** (`sql/demo.sql`). Crea la tabla `recurso` con 100 000 filas y el índice. Luego busca una clave que existe y una que no, inserta y busca por rango.
5. **Prueba de integración** (`pruebas/prueba_integracion.sql`). Ejecuta 200 consultas con el índice y sin él, y verifica que den exactamente lo mismo. Lo repite después de insertar, de borrar + VACUUM y de actualizar.

**Cómo se comprueba**

```bash
docker compose exec db make -C extension install
docker compose exec db psql -U postgres -f sql/demo.sql
docker compose exec db psql -U postgres -f pruebas/prueba_integracion.sql
```

- En la demo, `EXPLAIN` debe mostrar `Index Scan using idx_recurso_skip`. **Esa es la prueba de que PostgreSQL usa nuestro índice.**
- La prueba de integración debe mostrar `PASA` en las 6 líneas.
- Reiniciar (`docker compose restart`) y ejecutar `SELECT sl_verificar('después de reiniciar');` también debe dar `PASA`.

---

## Semana 5 – Parcial: demo reproducible y comparación con B-tree

**Lo que pide el curso:** demo reproducible, código, defensa individual y comparación preliminar con B-tree.

| Quién | Qué hace |
|---|---|
| Camila y Marccelo (3.3) | Verifican todo desde cero siguiendo solo el README. |
| Rodrigo (4.1) | Ejecuta las pruebas sin índice. |
| Nayalem (4.2) | Ejecuta las pruebas con B-tree. |
| Camila (4.3) | Ejecuta las pruebas con Skip List. |
| Pedro (4.4) | Repite los experimentos 5 veces y junta los resultados. |

**Pasos**

1. **Generador de datos** (`datos/generar_datos.py`). Crea D1, D2 y D3 y las cargas W1 a W4 con semilla fija (42).
2. **Comparación** (`experimentos/ejecutar.sh` + `benchmark.sql`). Usa los mismos datos y las mismas consultas en los 3 escenarios, con 5 repeticiones, y reporta la **mediana**.
   - Se usa la mediana porque una repetición que sale lenta por casualidad no la afecta.
   - Sin índice se ejecutan solo 200 consultas por carga, porque cada una recorre toda la tabla. Por eso se reporta el tiempo **por consulta**.
3. **README final**: los pasos exactos para reconstruir todo desde cero.
4. **Ensayo de defensa:** cada integrante debe poder hacer en vivo las tareas de la lista de abajo.

**Cómo se comprueba**

```bash
docker compose down -v && docker compose up -d --build     # empezar desde cero
docker compose exec db bash experimentos/ejecutar.sh D2 100000 5
```

Resultado preliminar obtenido (D2, 100 000 claves, mediana de 5 repeticiones):

| Métrica | Sin índice | B-tree | Skip List |
|---|---|---|---|
| Búsqueda exitosa (ms/consulta) | 7.36 | 0.016 | 0.019 |
| Búsqueda no exitosa (ms/consulta) | 7.29 | 0.010 | 0.014 |
| Rango (~100 claves, ms/consulta) | 8.15 | 0.124 | 0.135 |
| Construcción (ms) | – | 85 | 119 |
| Inserción de 10 000 filas (ms) | 8 | 35 | 92 |
| Tamaño del índice (MB) | – | 2.3 | 3.8 |

*Medido en un PostgreSQL 18.6 compilado con verificaciones de depuración; en Docker los tiempos absolutos serán distintos, pero la comparación es la que importa.*

**Lectura rápida:** la Skip List busca casi tan rápido como B-tree y unas 400 veces más rápido que sin índice. Ocupa más espacio y es más lenta al insertar. La razón es que cada nodo guarda una sola clave y se leen más páginas, mientras que B-tree guarda cientos de claves por página.

### Preguntas típicas del parcial (todos deben poder hacerlo)

| Te piden… | Cómo |
|---|---|
| Buscar una clave existente / inexistente | `SELECT * FROM recurso WHERE recurso_id = 50;` y `… = -1;` |
| Insertar y volver a buscar | `INSERT INTO recurso VALUES (777777,'X','Equipo',1);` y luego buscarla |
| Demostrar que se usa el índice | `EXPLAIN ANALYZE SELECT … WHERE recurso_id = 50;` y mostrar `Index Scan using idx_recurso_skip` |
| Reiniciar el entorno | `docker compose down -v && docker compose up -d --build` |
| Mostrar dónde se busca | `src/skiplist.hpp`, función `primeroMayorOIgual()` |
| Mostrar dónde se inserta | `src/skiplist.hpp`, función `insertar()` |
| Mostrar dónde se construye | `src/skiplist.hpp`, función `construir()` |
| Explicar el flujo PostgreSQL → Skip List | `skiplist_am.c` → `puente.cpp` → `skiplist.hpp` → `almacen_paginas.c` |
| ¿Qué pasa con los NULL? | No se indexan (como se documenta) |
| ¿Qué no se implementó? | WAL propio, concurrencia completa y unicidad (las pautas no los exigen) |
