#! /bin/bash
mkdir exes;
mkdir imgs;
gcc -Wall -o fuzzer fuzzer.c;

for i in {0..9};
do
    g++ -Wall -o exes/jpg2pdf-$i jpg2pdfs/jpg2pdf-$i.cpp;
done;
