# Compila e instala la extensión con la infraestructura de PostgreSQL (PGXS).
#   make            compila
#   make install    copia la extensión a la instalación de PostgreSQL

MODULE_big = skiplist
OBJS = skiplist_am.o almacen_paginas.o puente.o

EXTENSION = skiplist
DATA = skiplist--1.0.sql

# El código de la Skip List (C++) está en ../src
PG_CPPFLAGS = -I$(srcdir)/../src
PG_CXXFLAGS = -std=c++17 -fno-exceptions -fno-rtti
SHLIB_LINK += -lstdc++

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# No generar código para el JIT (evita necesitar clang)
override with_llvm = no

puente.o: ../src/skiplist.hpp ../src/nodo.h puente.h almacen_paginas.h
