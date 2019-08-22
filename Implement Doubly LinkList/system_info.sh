# Yejee Jenny Lee
# March 14, 2019

#!/bin/bash


echo -n "Cores: "
nproc --all

echo -n "memory: "
free -m | awk 'NR==2{printf "%sMB/%sMB \n", $4,$2}'

echo -n "primary partition storage: "
df -h | awk '$NF=="/"{printf "%dGB/%dGB \n", $4,$2}'
