#! /bin/bash
for i in {0..9};
do
    g++ -o exes/jpg2pdf-$i jpg2pdfs/jpg2pdf-$i.cpp;
done;
