#!/usr/bin/env python3
"""
generar_datos.py - Genera los datasets y cargas de trabajo de las pautas,
siempre con semilla fija para que sean reproducibles.

Datasets (enteros):
  D1  claves 1..N en orden (secuenciales)
  D2  N claves aleatorias únicas entre 1 y 10*N
  D3  mismas claves que D2, pero las búsquedas W1 siguen una distribución
      Zipf (unas pocas claves se consultan muchísimo más que el resto)

Cargas de trabajo:
  W1  10 000 búsquedas de claves que SÍ existen
  W2  10 000 búsquedas de claves que NO existen
  W3  inserciones: 10 % adicional del tamaño inicial (claves nuevas)
  W4  1 000 búsquedas por rango (~100 claves cada una)

Uso:
  python3 datos/generar_datos.py --dataset D2 --n 100000 --semilla 42
Crea la carpeta datos/generados/D2_100000/ con:
  claves.csv  w1.csv  w2.csv  w3.csv  w4.csv  parametros.txt
"""
import argparse
import bisect
import os
import random

ZIPF_S = 1.1            # parámetro de la distribución Zipf (D3)
N_W1 = 10_000
N_W2 = 10_000
N_W4 = 1_000
CLAVES_POR_RANGO = 100


def escribir(ruta, filas):
    with open(ruta, "w") as f:
        for fila in filas:
            f.write(",".join(str(x) for x in fila) + "\n" if isinstance(fila, tuple) else f"{fila}\n")


def muestra_zipf(rng, claves, cantidad, s):
    """Elige 'cantidad' claves: la clave en la posición r se elige con
    probabilidad proporcional a 1 / r^s (posiciones en orden aleatorio)."""
    orden = claves[:]
    rng.shuffle(orden)
    acumulado, total = [], 0.0
    for r in range(1, len(orden) + 1):
        total += 1.0 / (r ** s)
        acumulado.append(total)
    return [orden[bisect.bisect_left(acumulado, rng.random() * total)] for _ in range(cantidad)]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dataset", choices=["D1", "D2", "D3"], default="D2")
    p.add_argument("--n", type=int, default=100_000, help="cantidad de claves (100000, 500000, 1000000)")
    p.add_argument("--semilla", type=int, default=42)
    p.add_argument("--salida", default=None)
    a = p.parse_args()

    rng = random.Random(a.semilla)
    n = a.n
    salida = a.salida or os.path.join(os.path.dirname(os.path.abspath(__file__)), "generados", f"{a.dataset}_{n}")
    os.makedirs(salida, exist_ok=True)

    # --- Claves del dataset ---
    if a.dataset == "D1":
        claves = list(range(1, n + 1))
        maximo = n
    else:  # D2 y D3 usan las mismas claves aleatorias únicas
        maximo = 10 * n
        claves = rng.sample(range(1, maximo + 1), n)
    existentes = set(claves)

    # --- W1: búsquedas exitosas ---
    if a.dataset == "D3":
        w1 = muestra_zipf(rng, claves, N_W1, ZIPF_S)
    else:
        w1 = [rng.choice(claves) for _ in range(N_W1)]

    # --- W2: búsquedas no exitosas (claves que no están) ---
    w2 = []
    while len(w2) < N_W2:
        c = rng.randint(1, 2 * maximo)
        if c not in existentes:
            w2.append(c)

    # --- W3: inserciones (10 % de N, claves nuevas) ---
    n_ins = n // 10
    if a.dataset == "D1":
        w3 = list(range(n + 1, n + n_ins + 1))            # siguen en orden
    else:
        w3, usadas = [], set(existentes)
        while len(w3) < n_ins:
            c = rng.randint(1, 2 * maximo)
            if c not in usadas:
                usadas.add(c)
                w3.append(c)

    # --- W4: rangos que devuelven ~100 claves ---
    ancho = CLAVES_POR_RANGO * maximo // n
    w4 = []
    for _ in range(N_W4):
        desde = rng.randint(1, maximo - ancho)
        w4.append((desde, desde + ancho - 1))

    escribir(os.path.join(salida, "claves.csv"), claves)
    escribir(os.path.join(salida, "w1.csv"), w1)
    escribir(os.path.join(salida, "w2.csv"), w2)
    escribir(os.path.join(salida, "w3.csv"), w3)
    escribir(os.path.join(salida, "w4.csv"), w4)
    with open(os.path.join(salida, "parametros.txt"), "w") as f:
        f.write(f"dataset={a.dataset}\nn={n}\nsemilla={a.semilla}\nclave_maxima={maximo}\n"
                f"zipf_s={ZIPF_S if a.dataset == 'D3' else '-'}\nw1={N_W1}\nw2={N_W2}\nw3={n_ins}\n"
                f"w4={N_W4} (ancho {ancho})\n")
    print(f"Datos generados en {salida}")


if __name__ == "__main__":
    main()
