# Índice Skip List para PostgreSQL

Proyecto del curso Base de Datos II (UCSP, 2026-2): implementación propia de una Skip List en C++, integrada a PostgreSQL 18.6 como un nuevo tipo de índice.

**Equipo:** Camila Mansilla · Nayalem Arune · Marccelo Tito · Rodrigo Fuentes · Pedro Flores

## Estructura del repositorio

| Carpeta | Contenido |
|---|---|
| `docs/` | Propuesta, diseño e informes semanales |
| `src/` | Skip List en C++ |
| `extension/` | Integración con PostgreSQL (capa en C) |
| `pruebas/` | Pruebas |
| `sql/` | Scripts SQL y demo |
| `datos/` | Generadores de datos con semilla fija |
| `experimentos/` | Comparación de rendimiento |
| `docker/` | Dockerfile (PostgreSQL 18.6) |

El diseño y el pseudocódigo están en `docs/diseno.md`.

## Avance

- [x] Semana 1: comprensión y diseño
- [ ] Semana 2: construcción y búsqueda con pruebas
- [ ] Semana 3: inserción, rango y esqueleto de la extensión
- [ ] Semana 4: integración con PostgreSQL
- [ ] Semana 5: demo reproducible y comparación preliminar con B-tree

## Recursos externos

- W. Pugh, *Skip Lists: A Probabilistic Alternative to Balanced Trees*, CACM 33(6), 1990.
- Documentación oficial de PostgreSQL 18.
