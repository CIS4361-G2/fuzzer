#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Copy a JPG file to a new file
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

    return jpgCopy;
}

char *createSystemString(char *jpgFileName)
{
    char *systemString;
    char systemStringA[11];
    char systemStringB[50];
    char systemStringC[13];

    systemString = malloc(sizeof(char) * 72);

    strcpy(systemString, "");
    strcpy(systemStringA, "./jpg2pdf ");
    strcpy(systemStringB, jpgFileName);
    strcpy(systemStringC, " > /dev/null");

    strcat(systemString, systemStringA);
    strcat(systemString, systemStringB);
    strcat(systemString, systemStringC);

    return systemString;
}

int main(int argc, char *argv[])
{
    // Copy the source JPG to ensure that we don't modify the source directly
    FILE *jpgCopy = copyJPG(argv[1], argv[2]);

    // Create the command to have the system execute
    char *systemString = createSystemString(argv[2]);

    // Can be used to ensure that the systemString is correctly built
    // printf("%s\n", systemString);

    int i = 0;

    // This is only set to 50 for testing purposes
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
        // This gives the effect that the number is updating in place
        printf("\r%d iterations...", i++);

        // This ensures that C prints every line, not just sometimes
        fflush(stdout);
    }

    printf("\n");

    free(systemString);
    fclose(jpgCopy);

    // Signal that the program reached the end of execution...
    printf("The end of the road...\n");

    return 0;
}
