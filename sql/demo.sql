-- demo.sql - Demostración del índice Skip List dentro de PostgreSQL.
-- Ejecutar:  psql -U postgres -f sql/demo.sql
\set ON_ERROR_STOP on
\pset pager off

\echo '=== 1. Cargar la extensión ==='
DROP TABLE IF EXISTS recurso;
DROP EXTENSION IF EXISTS skiplist CASCADE;
CREATE EXTENSION skiplist;

\echo '=== 2. Crear la tabla y llenarla con 100 000 recursos ==='
CREATE TABLE recurso (
    recurso_id  INTEGER,
    nombre      VARCHAR(100),
    tipo        VARCHAR(50),
    cantidad    INTEGER
);
SELECT setseed(0.42);
INSERT INTO recurso
SELECT i, 'Recurso ' || i, (ARRAY['Equipo','Material','Mueble'])[1 + i % 3], (random() * 100)::int
FROM generate_series(1, 100000) AS i;

\echo '=== 3. Construir el índice Skip List ==='
\timing on
CREATE INDEX idx_recurso_skip ON recurso USING skiplist (recurso_id);
\timing off
ANALYZE recurso;
SELECT pg_size_pretty(pg_relation_size('idx_recurso_skip')) AS tamano_indice;

\echo '=== 4. Búsqueda de una clave EXISTENTE (debe usar el índice) ==='
EXPLAIN (ANALYZE, COSTS OFF) SELECT * FROM recurso WHERE recurso_id = 50;
SELECT * FROM recurso WHERE recurso_id = 50;

\echo '=== 5. Búsqueda de una clave que NO existe ==='
SELECT count(*) AS encontrados FROM recurso WHERE recurso_id = -1;

\echo '=== 6. Inserción: la fila nueva se encuentra con el índice ==='
INSERT INTO recurso VALUES (1000001, 'Laptop', 'Equipo', 3);
SELECT * FROM recurso WHERE recurso_id = 1000001;

\echo '=== 7. Búsqueda por rango ==='
EXPLAIN (ANALYZE, COSTS OFF) SELECT * FROM recurso WHERE recurso_id BETWEEN 100 AND 110;
SELECT recurso_id, nombre FROM recurso WHERE recurso_id BETWEEN 100 AND 110;
