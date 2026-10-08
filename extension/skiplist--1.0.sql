-- skiplist--1.0.sql: lo que se ejecuta con CREATE EXTENSION skiplist;
\echo Use "CREATE EXTENSION skiplist" para cargar este archivo. \quit

-- 1) La función que le describe el índice a PostgreSQL (está en skiplist_am.c)
CREATE FUNCTION skiplist_handler(internal)
RETURNS index_am_handler
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- 2) El nuevo tipo de índice
CREATE ACCESS METHOD skiplist TYPE INDEX HANDLER skiplist_handler;
COMMENT ON ACCESS METHOD skiplist IS 'índice Skip List (proyecto BD II)';

-- 3) Qué operadores puede resolver el índice para columnas INTEGER.
--    Los números 1..5 son el orden estándar: <, <=, =, >=, >
CREATE OPERATOR CLASS int4_skiplist_ops
DEFAULT FOR TYPE int4 USING skiplist AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 btint4cmp(int4, int4);
