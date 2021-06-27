#!/bin/bash

diff -u -B q6ans pyans
ret=$?

if [[ $ret -eq 0 ]]; then
    echo "passed."
else
    echo "failed."
fi