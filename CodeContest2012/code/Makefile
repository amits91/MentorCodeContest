FLAGS := 
SPEEDUPFLAGS := -O3 -msse4.2 -ffast-math
GCCOPENMP:=  gcc

all: compile_omp big_picture small_picture run

noomp: compile run

compile: main.c
	@echo "-- Compiling"
	$(GCCOPENMP) $(FLAGS) main.c -o run

compile_omp:
	@echo "-- Compiling"
	$(GCCOPENMP) -fopenmp $(FLAGS) $(SPEEDUPFLAGS) main.c -o run

big_picture:
	bunzip2 big_picture.bz2
small_picture:
	bunzip2 small_picture.bz2
run:
	@echo "-- Running program"
	./run

actual_run:
	./run
	./run
	./run
	./run
	./run
	./run
	./run
	./run
	./run
	./run

.PHONY: compile run
