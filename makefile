
q6:q6.c
	gcc -o q6 q6.c

check: q6
	./q6 100 > q6ans
	bash diff.sh

test: test.c q6.c
	gcc -o test test.c

plot:
	gnuplot plot.gp

accuracy: accuracy.c q6.c
	gcc -o accuracy accuracy.c