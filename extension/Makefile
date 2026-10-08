# Compila e instala la extensión con la infraestructura de PostgreSQL (PGXS).
#   make            compila
#   make install    copia la extensión a la instalación de PostgreSQL

MODULE_big = skiplist
OBJS = skiplist_am.o

EXTENSION = skiplist
DATA = skiplist--1.0.sql

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# No generar código para el JIT (evita necesitar clang)
override with_llvm = no
