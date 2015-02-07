#FLAGS := -g
#SPEEDUPFLAGS := -O3 -msse4.2 -ffast-math
#SPEEDUPFLAGS := -O3 -msse4.2 -ftree-vectorize -fno-rtti -fno-exceptions -fomit-frame-pointer -funroll-loops  -ftree-vectorizer-verbose=10 -ffast-math
SPEEDUPFLAGS := -O3 -msse4.2 -ftree-vectorize -fno-rtti -fno-exceptions -fomit-frame-pointer -funroll-loops  -ffast-math -minline-all-stringops -minline-stringops-dynamically -march=native
#SPEEDUPFLAGS := 
#GCCOPENMP:=  /u/prod/gnu/gcc/20100526/gcc-4.5.0-linux_x86_64/bin/gcc
GCCOPENMP:=  g++

all: clean compile run check1

noomp: compile run

compile: main.cpp
	@echo "-- Compiling"
	$(GCCOPENMP) $(FLAGS) $(SPEEDUPFLAGS) main.cpp -o run

compile_dbg: main.cpp
	@echo "-- Compiling DBG"
	$(GCCOPENMP) $(FLAGS) -g main.cpp -o run
compile_omp:
	@echo "-- Compiling"
	$(GCCOPENMP) -fopenmp $(FLAGS) $(SPEEDUPFLAGS) main.cpp -o run
run:
	@echo "-- Running program"
	./run input_graph new_employee_list sme_list

actual_run:
	\rm output; ./run ../input_graph ../new_employee_list ../sme_list
	\rm output; ./run ../input_graph ../new_employee_list ../sme_list
	\rm output; ./run ../input_graph ../new_employee_list ../sme_list

callgrind:
	valgrind --tool=callgrind ./run input_graph new_employee_list sme_list

clean:
	\rm -f output.txt output* run callgrind* log* >/dev/null
check:
	sort output -o output.txt
	diff output.txt ../gold_200_emp_first_sme.txt
check1:
	sed 's/\-\-[0-9]*\-\-/\-\-/' output > output1
	sort output1 -o output.txt
	diff output.txt gold_200_emp_sme.txt
	\rm output1
checklarge:
	./run ../input_graph ../new_employee_list ../sme_list
	sed 's/\-\-[0-9]*\-\-/\-\-/' output > output1
	sort output1 -o output.txt
	diff output.txt /in/inndt254/amits/CodeContest2014_gold/sorted.gold_5m_emp_sme.txt
	\rm output1
checklarge1:
	./run ../input_graph ../new_employee_list ../sme_list
	sort output -o output.txt
	diff output.txt /in/inndt254/amits/CodeContest2014_gold/sorted.gold_5m_emp_first_sme.txt

.PHONY: compile run
