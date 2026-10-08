# Imagen de PostgreSQL 18.6 con la extensión Skip List ya compilada e instalada.
FROM postgres:18.6

# Herramientas para compilar la extensión (C y C++) y Python para generar datos
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential postgresql-server-dev-18 python3 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /proyecto
COPY . /proyecto

# Compilar las pruebas de la Skip List en memoria
RUN make -C pruebas test_skiplist
