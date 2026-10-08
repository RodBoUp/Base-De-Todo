# Índice Skip List para PostgreSQL

Proyecto del curso Base de Datos II (UCSP, 2026-2): implementación propia de una Skip List en C++, integrada a PostgreSQL 18.6 como un nuevo tipo de índice.

**Equipo:** Camila Mansilla · Nayalem Arune · Marccelo Tito · Rodrigo Fuentes · Pedro Flores

## Cómo ejecutar

```bash
docker compose up -d --build
docker compose exec db make -C pruebas      # pruebas de la Skip List en memoria
```

## Avance

- [x] Semana 1: comprensión y diseño
- [x] Semana 2: construcción y búsqueda con pruebas
- [ ] Semana 3: inserción, rango y esqueleto de la extensión
- [ ] Semana 4: integración con PostgreSQL
- [ ] Semana 5: demo reproducible y comparación preliminar con B-tree

## Recursos externos

- W. Pugh, *Skip Lists: A Probabilistic Alternative to Balanced Trees*, CACM 33(6), 1990.
- Documentación oficial de PostgreSQL 18.
