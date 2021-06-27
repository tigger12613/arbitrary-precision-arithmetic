
q6:q6.c
	gcc -o q6 q6.c




check: q6
	./q6 100 > q6ans
	bash diff.sh



