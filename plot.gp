reset                                                                           
set xlabel 'bits'
set ylabel 'time'
set title 'perfomance comparison'
set term png enhanced font 'Verdana,10'
set output 'runtime.png'
set key left top
set logscale y
set xtics 0 ,32
set xtics rotate by 45 right


#plot [:][:] 'out' using 1:2 with points title 'karatsuba'

plot [:][:]'origin.txt' using 1:2 with points title 'origin',\
'karatsuba.txt' using 1:2 with points title 'karatsuba'