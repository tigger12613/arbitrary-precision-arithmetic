#!/bin/bash
make test
time ./test
gnuplot plot.gp