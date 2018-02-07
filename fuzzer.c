#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Copy a JPG file to another file
FILE *copyJPG(char *jpgInputFileName, char *jpgCopyFileName)
{
    FILE *jpgSource, *jpgCopy;
    int i = 0;
    int jpgLength;

    jpgSource = fopen(jpgInputFileName, "rb");
    if (jpgSource == NULL) {
        printf("FUZZER: Cannot open sample.jpg");
        return NULL;
    }

    fseek(jpgSource, 0, SEEK_END);
    jpgLength = ftell(jpgSource);
    fseek(jpgSource, 0, SEEK_SET);

    jpgCopy = fopen(jpgCopyFileName, "wb");

    if (jpgCopy == NULL) {
        printf("FUZZER: Cannot create sample_copy.jpg");
        fclose(jpgSource);
        return NULL;
    }

    for (i = 0; i < jpgLength; i++) {
        fputc(fgetc(jpgSource), jpgCopy);
    }

    printf("FUZZER: Image copied successfully.\n");

    fclose(jpgSource);
    fclose(jpgCopy);

    return jpgCopy;
}

int main(int argc, char *argv[])
{
    char *jpgInputFileName = argv[1];
    char *jpgCopyFileName = argv[2];

    char systemString[72];
    char systemStringA[11];
    char systemStringB[50];
    char systemStringC[13];

    strcpy(systemString, "");
    strcpy(systemStringA, "./jpg2pdf ");
    strcpy(systemStringB, jpgCopyFileName);
    strcpy(systemStringC, " > /dev/null");

    strcat(systemString, systemStringA);
    strcat(systemString, systemStringB);
    strcat(systemString, systemStringC);

    // Can be used to ensure that the systemString is correctly built
    // printf("%s\n", systemString);

    // Copy the source JPG to ensure that we don't modify the source directly
    FILE *jpgCopy = copyJPG(jpgInputFileName, jpgCopyFileName);

    int i = 0;

    while (i <= 50) {
        // "> /dev/null" will redirect the output of jpg2pdf to nowhere
        // This prevents the output of jpg2pdf from printing to the terminal
        // system("./jpg2pdf sample.jpg > /dev/null");
        if (system(systemString) == 11) {
            printf("CRASH\n");
            return 0;
        }
        // The carriage return ("\r") moves the cursor to the line start
        // This prevents this line from printing many times
        // Instead it just overwrites itself
        printf("\r%d iterations...", i++);
        // This causes the print lines to print with every iteration
        fflush(stdout);
    }

    printf("\n");

    // Ensure that the program reached the end of execution...
    printf("The end of the road...\n");

    return 0;
}
