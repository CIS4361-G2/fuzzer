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

#define FALSE 0
#define TRUE  1

typedef struct JPGFile {
    FILE *jpgFile;
    unsigned long fileSize;
} JPGFile;

// Function definitions
void printError(char *message);
char *JPGtoBits(JPGFile *jpgFile);
void modifyBits(JPGFile *file, int startByte, int endByte, int bytesToChange);
void saveJPGFile(char *bytes, int jpgLength, char *destination);
JPGFile *copyJPG(FILE *jpgSource, char *destination);
long random_at_most(long max);
char getRandomChar(int lowerBound, int upperBound);
int getRandomInt(int lowerBound, int upperBound);

void printError(char *message)
{
    printf("%s\n", message);
    fflush(stdin);
    exit(0);
}

// Returns an array of bits from the given JPG file
char *JPGtoBits(JPGFile *jpgFile)
{
    FILE *file = jpgFile->jpgFile;
    char *bytes = (char*) calloc(jpgFile->fileSize + 1, sizeof(int));
    char *string = (char*)malloc(sizeof(char) * 2);
    int c;

    // Ensure that string is null terminated
    string[1] = '\0';

    if (file == NULL)
    {
        printError("JPGtoBits(): File is null\n");
    }

    fseek(file, 0, SEEK_SET);

    if (DEBUG) printf("ftell() is %ld\n", ftell(file));

    if (feof(file))
    {
        printError("JPGtoBits(): End of file reached before beginning\n");
    }

    while ((c = fgetc(file)) != EOF)
    {
        string[0] = c;
        strcat(bytes, string);
    }

    // Ensure that string is null terminated
    bytes[jpgFile->fileSize] = '\0';

    return bytes;
}

// Modifies the given number of bytes using limitations given via parameters
// The JPGFile itself is modified, so nothing is returned.
void modifyBits(JPGFile *file, int startByte, int endByte, int bytesToChange) {
    char *bytes = JPGtoBits(file);
    int i;
    unsigned char replacementChar;
    int byteToChange;

    if (endByte >= file->fileSize)
    {
        printf("FUZZER: Invalid end byte specified for modification. Aborting.");
        return;
    }

    if (startByte < 0)
    {
        printf("FUZZER: Invalid start byte specified for modification. Aborting.");
        return;
    }

    for (i = 0; i < bytesToChange; i++)
    {
        byteToChange = getRandomInt(startByte, endByte);
        replacementChar = getRandomChar(0, 255);
        bytes[byteToChange] = replacementChar;
        saveJPGFile(bytes, file->fileSize, JPG_FILE);
    }
}

void saveJPGFile(char *bytes, int jpgLength, char *destination)
{
    // Attempt to create a new file in write-binary mode (for JPG copy)
    FILE *jpgCopy = fopen(destination, "w+b");
    int i;

    if (jpgCopy == NULL)
    {
        // Attempt to see if the file is missing. Let's create the file first, then see if the problem persists.
        printf("FUZZER: Cannot save JPG file %s\n", destination);
        return;
    }

    // Copy the JPG file byte by byte
    for (i = 0; i < jpgLength; i++)
    {
        fputc(bytes[i], jpgCopy);
    }

    fclose(jpgCopy);
}

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
long random_at_most(long max) {
    unsigned long num_bins;
    unsigned long num_rand;
    unsigned long bin_size;
    unsigned long defect;
    long x;

    if (max > RAND_MAX)
    {
        printf("FUZZER: random_at_most: max value is greater than RAND_MAX\n");
        return -1;
    }

    // max <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long) max + 1;
    num_rand = (unsigned long) RAND_MAX + 1;
    bin_size = num_rand / num_bins;
    defect   = num_rand % num_bins;

    // This is carefully written not to overflow
    do {
        x = random();
    } while (num_rand - defect <= (unsigned long) x);

    // Truncated (integer) division is intentional
    return x / bin_size;
}

// Returns a character that has the int value between lowerBound and upperBound
char getRandomChar(int lowerBound, int upperBound)
{
    return ((char) random_at_most(lowerBound + upperBound) - lowerBound);
}

int getRandomInt(int lowerBound, int upperBound)
{
    return ((int) random_at_most(lowerBound + upperBound) - lowerBound);
}


// Clone a JPG file into a newly created JPG file
// jpgInputFile must already be opened elsewhere
// jpgCopyFileName will be opened in this function
JPGFile *copyJPG(FILE *jpgSource, char *destination)
{
    FILE *jpgCopy;
    int i = 0;
    int jpgLength;
    JPGFile *file = malloc(sizeof(JPGFile));

    // Prepare our struct to store file information
    if (file == NULL)
    {
        printf("FUZZER: Unable to allocate memory for copied JPG file.\n");
        return NULL;
    }

    // Read through the end of the JPG file to count the length
    fseek(jpgSource, 0, SEEK_END);
    jpgLength = ftell(jpgSource);
    if (DEBUG) printf("ftell() is %ld\n", ftell(jpgSource));
    fseek(jpgSource, 0, SEEK_SET);


    // Attempt to create a new file in write-binary mode (for JPG copy)
    jpgCopy = fopen(destination, "w+b");

    if (jpgCopy == NULL)
    {
        // Attempt to see if the file is missing. Let's create the file first, then see if the problem persists.
        printf("FUZZER: Cannot create copied JPG file.\n");
        return NULL;
    }

    // Copy the JPG file byte by byte
    for (i = 0; i < jpgLength; i++)
    {
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
    char jpgCrashingFileName[20];

    int i = 0;
    int iterationCount = 0;
    int successCount = 0;
    int failureCount = 0;
    int systemReturnValue = 0;
    int crashDetected = 0;

    if (DEBUG) printf("\n");
    if (DEBUG) printf("COMMAND-LINE ARGUMENTS\n");
    if (DEBUG) printf("argv[0]: %s\n", argv[0]);
    if (DEBUG) printf("argv[1]: %s\n", argv[1]);
    if (DEBUG) printf("argv[2]: %s\n", argv[2]);
    if (DEBUG) printf("argv[3]: %s\n\n", argv[3]);

    // Ensure that the command-line arguments are all passed correctly
    if (argc != 4)
    {
        printf("Error: Incorrect execution format.\n");
        printf("Execution format: fuzzer [JPG] [start] [end]\n");
        printf("Example: fuzzer example.jpg 0 9\n");
        printf("[JPG] is a JPG file that needs to include .jpg\n");
        printf("[start] and [end] represent the range of bugs being tested\n");
        printf("[start] and [end] should be integers between 0 and 9\n");

        return 0;
    }

    start = atoi(argv[2]);
    end   = atoi(argv[3]);

    // Open the source JPG file
    jpgSource = fopen(jpgSourceName, "rb");

    if (jpgSource == NULL)
    {
        printf("FUZZER: Cannot open source JPG\n");
        return -1;
    }

    // Create a copy of the source JPG file
    JPGFile *jpgCopy = copyJPG(jpgSource, JPG_FILE);

    if (jpgCopy == NULL)
    {
        printf("FUZZER: Cannot create JPG copy\n");
        fclose(jpgSource);
        return -1;
    }

    for (i = start; i <= end; i++)
    {
        crashDetected = 0;
        sprintf(systemString, "exes/jpg2pdf-%d fuzzer_input_file.jpg > /dev/null", i);
        puts(systemString);
        while (!crashDetected)
        {
            /* generate JPG */
            systemReturnValue = system(systemString);

            if (DEBUG) printf("systemReturnValue: %d\n", systemReturnValue);

            if (systemReturnValue == SEGMENTATION_FAULT)
            {
                printf("\nProgram crash in jpg2pdf-%d!\n", i);
                // Save the JPG file
                sprintf(jpgCrashingFileName, "images/jpg2pdf-%d.jpg", i);
                copyJPG(jpgCopy->jpgFile, jpgCrashingFileName);
                crashDetected = 1;
            }
            else if (systemReturnValue == -1)
            {
                failureCount++;
            }
            else if (systemReturnValue == 65280)
            {
                successCount++;
            }

            iterationCount++;
            printf("\r%d iterations (%d successes, %d failures)", iterationCount, successCount, failureCount);
            fflush(stdout);

            // Modify image
            modifyBits(jpgCopy, 0, jpgCopy->fileSize - 1, 1);
        }
    }

    printf("\n");

    // Clean up
    free(jpgCopy);
    fclose(jpgSource);


    // State that the program reached the end of execution...
    if (DEBUG) printf("The end of the road...\n");

    return 0;
}
