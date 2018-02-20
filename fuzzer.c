#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 to print debug messages
// Set to 0 to suppress debug messages
#define DEBUG 1

// Build a file name string with "_copy" appended
// Example: example.jpg -> example_copy.jpg
char *createJPGCopyFileName(char *jpgSourceFileName)
{
    char *appendedString;
    char appendage[6];

    int i;

    appendedString = malloc(sizeof(char) * (strlen(jpgSourceFileName) + 5));

    // Ensure that the string is null-terminated
    // This prevents garbage from being present at the beginning of the string
    appendedString[0] = '\0';

    // This is what will be appended to the string
    strcpy(appendage, "_copy");

    // Find where the '.' character in the filename is located
    for (i = 0; i < strlen(jpgSourceFileName); i++) {
        if (jpgSourceFileName[i] == '.') {
            break;
        }
    }

    // Copy the filename up to (not including) the '.' character
    // Example: "example" <- example.jpg
    strncat(appendedString, jpgSourceFileName, i);

    // Add "_copy" to the filename
    // Example: "example_copy" <- example.jpg
    strcat(appendedString, appendage);

    // Add the rest of the filename; from (and including) '.' to the end of the filename
    // Example: "example_copy.jpg" <- example.jpg
    strcat(appendedString, &jpgSourceFileName[i]);

    // Print the resulting string
    if (DEBUG) printf("TEST: %s\n", appendedString);

    return appendedString;
}

// Clone a JPG file into a newly created JPG file
// jpgInputFile must already be openned elsewhere
// jpgCopyFileName will be opened in this function
FILE *copyJPG(FILE *jpgSource, char *jpgCopyFileName)
{
    FILE *jpgCopy;
    int i = 0;
    int jpgLength;

    // Open the JPG source file in read-binary mode
    if (jpgSource == NULL) {
        printf("FUZZER: Cannot open source JPG\n");
        return NULL;
    }

    // Read through the end of the JPG file to count the length
    fseek(jpgSource, 0, SEEK_END);
    jpgLength = ftell(jpgSource);
    fseek(jpgSource, 0, SEEK_SET);

    // Append "_copy" to the source file


    // Attempt to create a new file in write-binary mode (for JPG copy)
    jpgCopy = fopen(jpgCopyFileName, "wb");
    if (jpgCopy == NULL) {
        printf("FUZZER: Cannot create %s\n", (jpgCopyFileName != NULL ? jpgCopyFileName : "JPG copy"));
        fclose(jpgSource);
        return NULL;
    }

    // Copy the JPG file byte by byte
    for (i = 0; i < jpgLength; i++) {
        fputc(fgetc(jpgSource), jpgCopy);
    }

    printf("FUZZER: Image copied successfully.\n");

    return jpgCopy;
}

// Generate the string used for the system call to execute the jpg2pdf program
char *createSystemString(char *jpgFileName)
{
    char *systemString;
    char systemStringA[11];
    char systemStringB[50];
    char systemStringC[13];

    systemString = malloc(sizeof(char) * 72);

    // Example: ./jpg2pdf example.jpg > /dev/null
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
    // TEST
    char *jpgCopyFileName = createJPGCopyFileName(argv[1]);

    // Open the source JPG file
    FILE *jpgSource = fopen(argv[1], "rb");
    if (jpgSource == NULL) {
        printf("FUZZER: Cannot open source JPG\n");
        return -1;
    }

    // Create a copy of the source JPG file
    FILE *jpgCopy = copyJPG(jpgSource, jpgCopyFileName);
    if (jpgCopy == NULL) {
        printf("FUZZER: Cannot create %s\n", (argv[2] != NULL ? argv[2] : "JPG copy"));
        fclose(jpgSource);
        return -1;
    }

    // Create the command to have the system execute
    char *systemString = createSystemString(argv[2]);

    int i = 0;

    // Can be used to ensure that the systemString is correctly built
    if (DEBUG) printf("%s\n", systemString);

    // This is only set to 50 for testing purposes
    while (i <= 50) {
        // Example: system("./jpg2pdf example.jpg > /dev/null");
        // "> /dev/null" will redirect the output of jpg2pdf to nowhere
        // This prevents the output of jpg2pdf from printing to the terminal
        if (system(systemString) == 11) {
            printf("CRASH\n");
            return 0;
        }

        // The carriage return ("\r") moves the cursor to the line start
        // This prevents this line from printing many times
        // Instead it just overwrites itself
        // This gives the effect that the number is updating in place
        printf("\r%d iterations...", i++);

        // This ensures that every line is printed (not just some of them)
        fflush(stdout);
    }

    printf("\n");

    // Clean up
    free(systemString);
    fclose(jpgSource);
    fclose(jpgCopy);


    // Signal that the program reached the end of execution...
    if (DEBUG) printf("The end of the road...\n");

    return 0;
}
