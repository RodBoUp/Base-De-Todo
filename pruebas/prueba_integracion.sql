-- prueba_integracion.sql - Verifica que el índice Skip List devuelve EXACTAMENTE
-- lo mismo que una búsqueda sin índice (recorrido completo de la tabla).
-- Incluye claves repetidas, NULL, inserciones, DELETE + VACUUM.
-- Ejecutar:  psql -U postgres -f pruebas/prueba_integracion.sql
-- (Después de reiniciar PostgreSQL se puede volver a correr solo el
--  final con:  SELECT sl_verificar('después de reiniciar');)
\set ON_ERROR_STOP on
\pset pager off
SET client_min_messages = warning;

CREATE EXTENSION IF NOT EXISTS skiplist;
DROP TABLE IF EXISTS prueba_sl;
CREATE TABLE prueba_sl (k INTEGER, relleno TEXT);

-- 50 000 claves aleatorias entre 1 y 20 000 (muchas repetidas) + 100 NULL
SELECT setseed(0.1);
INSERT INTO prueba_sl SELECT (random() * 19999)::int + 1, 'x' FROM generate_series(1, 50000);
INSERT INTO prueba_sl SELECT NULL, 'nulo' FROM generate_series(1, 100);
CREATE INDEX idx_prueba_sl ON prueba_sl USING skiplist (k);
ANALYZE prueba_sl;

-- 200 consultas reproducibles
DROP TABLE IF EXISTS consultas_sl;
CREATE TABLE consultas_sl AS
SELECT g AS id, lo, lo + (random() * 300)::int AS hi
FROM (SELECT g, (random() * 21000)::int - 500 AS lo FROM generate_series(1, 200) g) s;

-- Cuenta resultados de =, BETWEEN, < y > para cada consulta, primero
-- obligando a usar el índice y luego sin índice, y compara.
CREATE OR REPLACE FUNCTION sl_verificar(etiqueta TEXT) RETURNS TEXT AS $$
DECLARE
    malos INT;
    usa_indice BOOLEAN;
    plan TEXT;
BEGIN
    -- 1) Con el índice Skip List
    PERFORM set_config('enable_seqscan', 'off', true);
    PERFORM set_config('enable_bitmapscan', 'off', true);
    usa_indice := false;
    FOR plan IN EXECUTE 'EXPLAIN SELECT count(*) FROM prueba_sl WHERE k = 5' LOOP
        IF plan LIKE '%idx_prueba_sl%' THEN usa_indice := true; END IF;
    END LOOP;
    CREATE TEMP TABLE r_indice ON COMMIT DROP AS
    SELECT id,
           (SELECT count(*) FROM prueba_sl WHERE k = lo)               AS igual,
           (SELECT count(*) FROM prueba_sl WHERE k BETWEEN lo AND hi)  AS rango,
           (SELECT count(*) FROM prueba_sl WHERE k < lo)               AS menor,
           (SELECT count(*) FROM prueba_sl WHERE k > hi)               AS mayor
    FROM consultas_sl;

    -- 2) Sin índice (recorriendo toda la tabla)
    PERFORM set_config('enable_seqscan', 'on', true);
    PERFORM set_config('enable_indexscan', 'off', true);
    CREATE TEMP TABLE r_tabla ON COMMIT DROP AS
    SELECT id,
           (SELECT count(*) FROM prueba_sl WHERE k = lo)               AS igual,
           (SELECT count(*) FROM prueba_sl WHERE k BETWEEN lo AND hi)  AS rango,
           (SELECT count(*) FROM prueba_sl WHERE k < lo)               AS menor,
           (SELECT count(*) FROM prueba_sl WHERE k > hi)               AS mayor
    FROM consultas_sl;
    PERFORM set_config('enable_indexscan', 'on', true);
    PERFORM set_config('enable_bitmapscan', 'on', true);

    SELECT count(*) INTO malos
    FROM r_indice a JOIN r_tabla b USING (id)
    WHERE (a.igual, a.rango, a.menor, a.mayor) IS DISTINCT FROM (b.igual, b.rango, b.menor, b.mayor);
    DROP TABLE r_indice;
    DROP TABLE r_tabla;

    RETURN rpad(etiqueta, 40) ||
           CASE WHEN NOT usa_indice THEN 'FALLA (no usó el índice)'
                WHEN malos = 0 THEN 'PASA'
                ELSE 'FALLA (' || malos || ' consultas distintas)' END;
END;
$$ LANGUAGE plpgsql;

\echo '--- Resultados ---'
SELECT sl_verificar('1. después de construir') AS resultado;

INSERT INTO prueba_sl SELECT (random() * 25000)::int - 1000, 'nuevo' FROM generate_series(1, 20000);
SELECT sl_verificar('2. después de 20 000 inserciones') AS resultado;

DELETE FROM prueba_sl WHERE k % 7 = 0;
VACUUM prueba_sl;
SELECT sl_verificar('3. después de DELETE + VACUUM') AS resultado;

INSERT INTO prueba_sl SELECT (random() * 20000)::int, 'reuso' FROM generate_series(1, 10000);
SELECT sl_verificar('4. insertar después de VACUUM') AS resultado;

UPDATE prueba_sl SET k = k + 1 WHERE k % 11 = 0;
SELECT sl_verificar('5. después de UPDATE') AS resultado;

INSERT INTO prueba_sl VALUES (2147483647, 'max'), (-2147483648, 'min');
SELECT sl_verificar('6. claves extremas (min y max int)') AS resultado;
SELECT count(*) = 1 AS max_ok FROM prueba_sl WHERE k = 2147483647;
SELECT count(*) = 1 AS min_ok FROM prueba_sl WHERE k = -2147483648;
