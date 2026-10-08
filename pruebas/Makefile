# Pruebas de la Skip List en memoria (no necesitan PostgreSQL).
#   make -C pruebas        compila y ejecuta las pruebas

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

test: test_skiplist
	./test_skiplist

test_skiplist: test_skiplist.cpp ../src/skiplist.hpp ../src/almacen_memoria.hpp ../src/nodo.h
	$(CXX) $(CXXFLAGS) -I../src -o $@ test_skiplist.cpp

clean:
	rm -f test_skiplist

.PHONY: test clean
