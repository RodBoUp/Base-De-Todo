# Índice Skip List para PostgreSQL

Proyecto del curso Base de Datos II (UCSP, 2026-2). Es una implementación propia de una Skip List en C++, integrada a PostgreSQL 18.6 como un nuevo tipo de índice:

```sql
CREATE EXTENSION skiplist;
CREATE INDEX idx_recurso ON recurso USING skiplist (recurso_id);
```

**Equipo:** Camila Mansilla · Nayalem Arune · Marccelo Tito · Rodrigo Fuentes · Pedro Flores

---

## Cómo ejecutar (desde cero)

Requisitos: Docker y Docker Compose.

```bash
# 1. Construir la imagen y levantar PostgreSQL 18.6 con la extensión instalada
docker compose up -d --build

# 2. Pruebas de la Skip List en memoria (sin PostgreSQL)
docker compose exec db make -C pruebas

# 3. Demostración dentro de PostgreSQL (búsqueda, inserción y rango)
docker compose exec db psql -U postgres -f sql/demo.sql

# 4. Prueba de integración: el índice debe devolver lo mismo que sin índice
docker compose exec db psql -U postgres -f pruebas/prueba_integracion.sql

# 5. Comparación sin índice vs. B-tree vs. Skip List (D2, 100 000 claves, 5 repeticiones)
docker compose exec db bash experimentos/ejecutar.sh D2 100000 5
```

Para entrar a PostgreSQL y probar a mano: `docker compose exec db psql -U postgres`

Para reiniciar todo desde cero (se borra la base de datos): `docker compose down -v && docker compose up -d --build`

---

## Estructura del repositorio

| Carpeta | Contenido |
|---|---|
| `src/` | **Skip List en C++**: `skiplist.hpp` (construcción, búsqueda, inserción, rango), `nodo.h`, `almacen_memoria.hpp` |
| `extension/` | **Integración con PostgreSQL**: `skiplist_am.c` (funciones que llama PostgreSQL), `almacen_paginas.c` (nodos en páginas del índice), `puente.cpp` (une C con C++), archivos de la extensión |
| `pruebas/` | Pruebas en memoria (`test_skiplist.cpp`) y prueba de integración (`prueba_integracion.sql`) |
| `sql/` | `demo.sql`: demostración para el parcial |
| `datos/` | `generar_datos.py`: datasets D1, D2, D3 y cargas W1–W4 con semilla fija |
| `experimentos/` | Comparación de rendimiento (`ejecutar.sh`, `benchmark.sql`) y resultados |
| `docker/` | `Dockerfile` (PostgreSQL 18.6 + compilación de la extensión) |
| `docs/` | Propuesta, guía de construcción e informes semanales |

## Cómo funciona (resumen)

```
SELECT ... WHERE recurso_id = 50
        │
        ▼
PostgreSQL (planificador elige el índice skiplist)
        │   llama a las funciones de skiplist_am.c
        ▼
skiplist_am.c (C)  ──►  puente.cpp  ──►  SkipList en C++ (src/skiplist.hpp)
                                               │ lee y escribe nodos
                                               ▼
                                   almacen_paginas.c: páginas del índice
```

- El algoritmo de la Skip List está escrito una sola vez (`src/skiplist.hpp`). Se prueba con nodos en memoria y PostgreSQL lo usa con nodos guardados en las páginas del índice.
- La página 0 del índice guarda el HEAD y el nivel actual. Las demás páginas guardan los nodos.
- **Construcción:** ordena todas las claves y enlaza los nodos de izquierda a derecha.
- **Búsqueda:** baja por los niveles. **Inserción:** guarda el último nodo de cada nivel y enlaza el nuevo empezando por el nivel 0. **Rango:** recorre el nivel 0.
- **VACUUM:** marca como borrados los nodos de filas eliminadas (borrado lógico).

## Alcance y limitaciones

- La clave es `INTEGER` y se soportan los operadores `=`, `<`, `<=`, `>`, `>=` y `BETWEEN`.
- No se implementa WAL propio. Si PostgreSQL se cae de forma abrupta, se debe ejecutar `REINDEX`. Un reinicio normal no tiene problema.
- Solo un `INSERT` modifica el índice a la vez (bloqueo simple). Las búsquedas siguen funcionando mientras tanto.
- No soporta tablas `UNLOGGED` ni índices de varias columnas. El borrado es lógico: el espacio no se reutiliza.

## Recursos externos y uso de IA

- W. Pugh, *Skip Lists: A Probabilistic Alternative to Balanced Trees*, Communications of the ACM 33(6), 1990.
- Documentación oficial de PostgreSQL 18: *Index Access Method Interface Definition*, *Extending SQL*.
- El módulo `contrib/bloom` de PostgreSQL se consultó como ejemplo de la estructura de un índice (no se copió código).
- *Completar:* indicar aquí si se usó alguna herramienta de IA y en qué partes.

## Avance por semana

Cada semana tiene una etiqueta en Git (`git checkout semana-2`, etc.). Ver `docs/GUIA_CONSTRUCCION.md`.

- [x] Semana 1: comprensión y diseño
- [x] Semana 2: construcción y búsqueda con pruebas
- [x] Semana 3: inserción, rango y esqueleto de la extensión
- [x] Semana 4: integración con PostgreSQL
- [x] Semana 5: demo reproducible y comparación preliminar con B-tree
