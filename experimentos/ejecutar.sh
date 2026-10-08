#!/usr/bin/env bash
# ejecutar.sh - Comparación sin índice vs. B-tree vs. Skip List.
#
# Uso:   bash experimentos/ejecutar.sh [DATASET] [N] [REPETICIONES] [LIMITE]
# Ej.:   bash experimentos/ejecutar.sh D2 100000 5 1000
#
#   DATASET       D1, D2 o D3                         (por defecto D2)
#   N             cantidad de claves                  (por defecto 100000)
#   REPETICIONES  veces que se repite cada escenario  (por defecto 5, mínimo de las pautas)
#   LIMITE        consultas de W1/W2/W4 por repetición (por defecto 1000;
#                 usar 10000 para la carga completa — sin índice tarda bastante)
#
# Resultado: experimentos/resultados/<DATASET>_<N>.csv y un resumen en pantalla.
set -euo pipefail

DATASET=${1:-D2}
N=${2:-100000}
REPS=${3:-5}
LIMITE=${4:-1000}
SEMILLA=42

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
DIR_DATOS="$RAIZ/datos/generados/${DATASET}_${N}"
DIR_RES="$RAIZ/experimentos/resultados"
export PGOPTIONS="-c client_min_messages=warning"
PSQL="psql -X -q -v ON_ERROR_STOP=1 ${PSQL_OPCIONES:-}"
mkdir -p "$DIR_RES"

echo "== 1. Generando datos $DATASET (N=$N, semilla=$SEMILLA)"
python3 "$RAIZ/datos/generar_datos.py" --dataset "$DATASET" --n "$N" --semilla "$SEMILLA" --salida "$DIR_DATOS"

echo "== 2. Cargando datos en PostgreSQL"
$PSQL -f "$RAIZ/experimentos/benchmark.sql"
$PSQL <<SQL
DROP TABLE IF EXISTS base, w1, w2, w3, w4;
CREATE TABLE base (k INTEGER, info TEXT DEFAULT 'dato');
CREATE TABLE w1 (id SERIAL, k INTEGER);
CREATE TABLE w2 (id SERIAL, k INTEGER);
CREATE TABLE w3 (id SERIAL, k INTEGER);
CREATE TABLE w4 (id SERIAL, desde INTEGER, hasta INTEGER);
\copy base(k) FROM '$DIR_DATOS/claves.csv' CSV
\copy w1(k) FROM '$DIR_DATOS/w1.csv' CSV
\copy w2(k) FROM '$DIR_DATOS/w2.csv' CSV
\copy w3(k) FROM '$DIR_DATOS/w3.csv' CSV
\copy w4(desde, hasta) FROM '$DIR_DATOS/w4.csv' CSV
DELETE FROM resultados WHERE dataset = '$DATASET' AND n = $N;
SQL

echo "== 3. Ejecutando $REPS repeticiones por escenario"
for ESC in sin_indice btree skiplist; do
    # Sin índice cada consulta recorre toda la tabla: se limita a 200 consultas
    LIM=$LIMITE
    if [ "$ESC" = "sin_indice" ] && [ "$LIM" -gt 200 ]; then LIM=200; fi
    for REP in $(seq 1 "$REPS"); do
        echo "   $ESC - repetición $REP"
        $PSQL -c "SELECT bench_ronda('$DATASET', $N, '$ESC', $REP, $LIM);" >/dev/null
    done
done

echo "== 4. Resultados (mediana de $REPS repeticiones)"
$PSQL -c "SELECT metrica, unidad, sin_indice, btree, skiplist FROM resumen WHERE dataset='$DATASET' AND n=$N;"
$PSQL -c "\copy (SELECT * FROM resultados WHERE dataset='$DATASET' AND n=$N ORDER BY escenario, repeticion, metrica) TO '$DIR_RES/${DATASET}_${N}.csv' CSV HEADER"
$PSQL -c "\copy (SELECT version()) TO '$DIR_RES/version_postgresql.txt'"
echo "Detalle guardado en $DIR_RES/${DATASET}_${N}.csv"
