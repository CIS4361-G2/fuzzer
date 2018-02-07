#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int i = 0;

    while (i <= 50) {
        // "> /dev/null" will redirect the output of jpg2pdf to nowhere
        // This prevents the output of jpg2pdf from printing to the terminal
        system("./jpg2pdf sample.jpg > /dev/null");
        // The carriage return ("\r") moves the cursor to the line start
        // This prevents this line from printing many times
        // Instead it just overwrites itself
        printf("\r%d iterations...", i++);
        // This causes the print lines to print with every iteration
        fflush(stdout);
    }

    printf("\n");

    printf("The end of the road...\n");

    return 0;
}
