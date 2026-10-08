-- benchmark.sql - Funciones para medir los tres escenarios:
--   sin_indice, btree y skiplist, con exactamente los mismos datos y operaciones.
-- Lo usa experimentos/ejecutar.sh (no hace falta ejecutarlo a mano).

CREATE EXTENSION IF NOT EXISTS skiplist;

CREATE TABLE IF NOT EXISTS resultados (
    dataset    TEXT,
    n          INTEGER,
    escenario  TEXT,
    repeticion INTEGER,
    metrica    TEXT,
    valor      DOUBLE PRECISION,
    unidad     TEXT
);

-- Una repetición completa de un escenario:
-- 1) copia los datos, 2) construye el índice, 3) ejecuta W1, W2, W4 y W3.
-- "limite" = cantidad máxima de consultas de W1/W2/W4 que se ejecutan
-- (el escenario sin índice es muy lento con 10 000 consultas).
CREATE OR REPLACE FUNCTION bench_ronda(p_dataset TEXT, p_n INT, p_escenario TEXT, p_rep INT, p_limite INT)
RETURNS VOID AS $$
DECLARE
    t0 TIMESTAMPTZ;
    ms DOUBLE PRECISION;
    cnt BIGINT;
    total INT;
    r RECORD;
BEGIN
    DROP TABLE IF EXISTS datos;
    CREATE TABLE datos AS SELECT k, info FROM base;

    -- Construcción del índice
    t0 := clock_timestamp();
    IF p_escenario = 'btree' THEN
        CREATE INDEX idx_datos ON datos USING btree (k);
    ELSIF p_escenario = 'skiplist' THEN
        CREATE INDEX idx_datos ON datos USING skiplist (k);
    END IF;
    ms := extract(epoch FROM clock_timestamp() - t0) * 1000;
    INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'construccion', ms, 'ms');
    ANALYZE datos;

    -- En los escenarios con índice, obligar a usarlo
    PERFORM set_config('enable_seqscan', CASE WHEN p_escenario = 'sin_indice' THEN 'on' ELSE 'off' END, true);

    -- W1: búsquedas exitosas
    total := 0; t0 := clock_timestamp();
    FOR r IN SELECT k FROM w1 ORDER BY id LIMIT p_limite LOOP
        SELECT count(*) INTO cnt FROM datos WHERE k = r.k;
        total := total + 1;
    END LOOP;
    ms := extract(epoch FROM clock_timestamp() - t0) * 1000 / total;
    INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'W1_busqueda_exitosa', ms, 'ms/consulta');

    -- W2: búsquedas no exitosas
    total := 0; t0 := clock_timestamp();
    FOR r IN SELECT k FROM w2 ORDER BY id LIMIT p_limite LOOP
        SELECT count(*) INTO cnt FROM datos WHERE k = r.k;
        total := total + 1;
    END LOOP;
    ms := extract(epoch FROM clock_timestamp() - t0) * 1000 / total;
    INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'W2_busqueda_no_exitosa', ms, 'ms/consulta');

    -- W4: búsquedas por rango
    total := 0; t0 := clock_timestamp();
    FOR r IN SELECT desde, hasta FROM w4 ORDER BY id LIMIT p_limite LOOP
        SELECT count(*) INTO cnt FROM datos WHERE k BETWEEN r.desde AND r.hasta;
        total := total + 1;
    END LOOP;
    ms := extract(epoch FROM clock_timestamp() - t0) * 1000 / total;
    INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'W4_rango', ms, 'ms/consulta');

    -- W3: inserciones (10 % adicional)
    t0 := clock_timestamp();
    INSERT INTO datos SELECT k, 'nuevo' FROM w3;
    ms := extract(epoch FROM clock_timestamp() - t0) * 1000;
    INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'W3_insercion_total', ms, 'ms');

    -- Tamaño del índice
    IF p_escenario <> 'sin_indice' THEN
        INSERT INTO resultados VALUES (p_dataset, p_n, p_escenario, p_rep, 'tamano_indice',
            pg_relation_size('idx_datos') / 1024.0 / 1024.0, 'MB');
    END IF;
END;
$$ LANGUAGE plpgsql;

-- Resumen: mediana de las repeticiones (más robusta que el promedio
-- ante una ejecución que salió lenta por casualidad).
CREATE OR REPLACE VIEW resumen AS
SELECT dataset, n, metrica, unidad,
       round(percentile_cont(0.5) WITHIN GROUP (ORDER BY valor) FILTER (WHERE escenario = 'sin_indice')::numeric, 4) AS sin_indice,
       round(percentile_cont(0.5) WITHIN GROUP (ORDER BY valor) FILTER (WHERE escenario = 'btree')::numeric, 4)      AS btree,
       round(percentile_cont(0.5) WITHIN GROUP (ORDER BY valor) FILTER (WHERE escenario = 'skiplist')::numeric, 4)   AS skiplist,
       count(DISTINCT repeticion) AS repeticiones
FROM resultados
GROUP BY dataset, n, metrica, unidad
ORDER BY dataset, n, metrica;
