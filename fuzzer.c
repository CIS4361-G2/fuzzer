#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 to print debug messages
// Set to 0 to suppress debug messages
#define DEBUG 1

// System exit codes
#define SEGMENTATION_FAULT 35584

// The name for the generated JPG file(s)
#define JPG_FILE "fuzzer_input_file.jpg"

// True & False values
#define false 0
#define true 1

typedef struct JPGFile {
    FILE *jpgFile;
    unsigned long fileSize;
} JPGFile;

char *int2bin(char n) {
    // determine the number of bits needed ("sizeof" returns bytes)
    int nbits = sizeof(n) * 8;
    char *s = malloc(nbits + 1);  // +1 for '\0' terminator
    s[nbits] = '\0';
    // forcing evaluation as an unsigned value prevents complications
    // with negative numbers at the left-most bit
    unsigned int u = *(unsigned int*)&n;
    int i;
    unsigned int mask = 1 << (nbits - 1); // fill in values right-to-left
    for (i = 0; i < nbits; i++, mask >>= 1)
        s[i] = ((u & mask) != 0) + '0';
    return s;
}

// Returns an array of bits from the given
// JPGFile
char *JPGtoBits(JPGFile *jpgFile) {
    FILE *file = jpgFile->jpgFile;
    int numBits = jpgFile->fileSize * 8;
    char *bits = (char*)calloc(numBits + 1, sizeof(int));
    char *byte;
    int i = 0;
    int k = 0;
    int c;

    if (file == NULL) {
        printf("jpgtobits: file is null!\n");
    }

    fseek(file, 0, SEEK_SET);
    if (DEBUG) printf("ftell() is %ld\n", ftell(file));
    if (feof(file)) {
        printf("End of file reached before we begin!\n");
    }
    while ((c = fgetc(file)) != EOF) {
        c = (char)c;
        byte = int2bin(c);
    //  if (DEBUG) printf("c is %c, byte is %s\n", c, byte);
        strcat(bits, byte);
    }

    bits[numBits] = '\0';
    return bits;
}

// Modifies the given number of bits using
// limitations given via paramaters. The
// JPGFile itself is modified, so nothing
// is returned.
void modifyBits(JPGFile *file, int startBit, int endBit, int bitsToChange, int excludeFirst, int excludeLast) {
    char *bits = JPGtoBits(file);
    int i = 0;
    printf("bits %s\n", bits);
}

// Clone a JPG file into a newly created JPG file
// jpgInputFile must already be openned elsewhere
// jpgCopyFileName will be opened in this function
JPGFile *copyJPG(FILE *jpgSource)
{
    FILE *jpgCopy;
    int i = 0;
    int jpgLength;
    JPGFile *file = malloc(sizeof(JPGFile));

    // Prepare our struct to store file information
    if (file == NULL) {
        printf("FUZZER: Unable to allocate memory for copied JPG file.\n");
        return NULL;
    }

    // Read through the end of the JPG file to count the length
    fseek(jpgSource, 0, SEEK_END);
    jpgLength = ftell(jpgSource);
    fseek(jpgSource, 0, SEEK_SET);

    // Attempt to create a new file in write-binary mode (for JPG copy)
    jpgCopy = fopen(JPG_FILE, "rwb");

    if (jpgCopy == NULL) {
        printf("FUZZER: Cannot create copied JPG file.\n");
        fclose(jpgSource);
        return NULL;
    }

    // Copy the JPG file byte by byte
    for (i = 0; i < jpgLength; i++) {
        fputc(fgetc(jpgSource), jpgCopy);
    }

    file->jpgFile = jpgCopy;
    file->fileSize = jpgLength;

    return file;
}

int main(int argc, char *argv[])
{
    char *jpgSourceName = argv[1];
    int start;
    int end;
    FILE *jpgSource = NULL;
    char systemString[50];

    if (DEBUG) printf("\n");
    if (DEBUG) printf("COMMAND-LINE ARGUMENTS\n");
    if (DEBUG) printf("argv[0]: %s\n", argv[0]);
    if (DEBUG) printf("argv[1]: %s\n", argv[1]);
    if (DEBUG) printf("argv[2]: %s\n", argv[2]);
    if (DEBUG) printf("argv[3]: %s\n\n", argv[3]);

    // Ensure that the command-line arguments are all passed correctly
    if (argc != 4) {
        printf("Error: Incorrect execution format.\n");
        printf("Execution format: fuzzer [JPG] [start] [end]\n");
        printf("Example: fuzzer example.jpg 0 9\n");
        printf("[JPG] is a JPG file that needs to include .jpg\n");
        printf("[start] and [end] represent the range of bugs being tested\n");
        printf("[start] and [end] should be integers between 0 and 9\n");

        return 0;
    }

    start = atoi(argv[2]);
    end = atoi(argv[3]);

    // Open the source JPG file
    jpgSource = fopen(jpgSourceName, "rb");
    if (jpgSource == NULL) {
        printf("FUZZER: Cannot open source JPG\n");
        return -1;
    }

    // Create a copy of the source JPG file
    JPGFile *jpgCopy = copyJPG(jpgSource);
    if (jpgCopy == NULL) {
        printf("FUZZER: Cannot create JPG copy");
        fclose(jpgSource);
        return -1;
    }

    int i = 0;
    int iterationCount = 0;
    int successCount = 0;
    int failureCount = 0;
    int systemReturnValue = 0;
    int crashDetected = 0;


    // This is just here for testing
    modifyBits(jpgCopy, 0, jpgCopy->fileSize * 8, 1, 0, 0);

    for (i = start; i <= end; i++)
    {
        crashDetected = 0;

        while (!crashDetected) {
            /* generate JPG */

            sprintf(systemString, "./jpg2pdf-%d fuzzer_input_file.jpg > /dev/null", i);
            puts(systemString);

            systemReturnValue = system(systemString);

            if (DEBUG) printf("systemReturnValue: %d\n", systemReturnValue);

            if (systemReturnValue == SEGMENTATION_FAULT) {
                printf("\nProgram crash in jpg2pdf-%d!\n", i);
                // Save the JPG file
                crashDetected = 1;
            } else if (systemReturnValue == -1) {
                failureCount++;
            } else if (systemReturnValue == 0) {
                successCount++;
            }

            // This is here just for testing
            if (iterationCount > 10) {
                break;
            }

            iterationCount++;
            printf("\r%d iterations (%d successes, %d failures)\n\n", iterationCount, successCount, failureCount);
            fflush(stdout);
        }
    }

    printf("\n");

    // Clean up
    free(jpgCopy);
    fclose(jpgSource);


    // Signal that the program reached the end of execution...
    if (DEBUG) printf("The end of the road...\n");

    return 0;
}
