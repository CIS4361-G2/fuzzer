#! /bin/bash
mkdir exes;
mkdir images;
gcc -Wall -o fuzzer fuzzer.c;

for i in {0..10};
do
    g++ -Wall -o exes/jpg2pdf-$i jpg2pdfs/jpg2pdf-$i.cpp;
done;

# (cmd) fuzzer: watch -d -n0.6 hexdump -Cv fuzzer_input_file.jpg
