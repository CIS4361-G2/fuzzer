#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 to print debug messages
// Set to 0 to suppress debug messages
#define DEBUG 0

typedef struct Jpgfileinfo {
	FILE *jpgFile;
	char *fileName;
	unsigned long fileSize;
} JPGFile;

// Modifies the given number of bits using
// limitations given via paramaters. The
// JPGFile itself is modified, so nothing
// is returned.
void modifyBits(JPGFile *file, int startBit, int endBit, int bitsToChange, int excludeFirst, int excludeLast) {
	int numBits = file->fileSize * 8;
	int *bits = (int*)calloc(numBits, sizeof(int));
	int i = 0;
	int k = 0;
	char curChar;

	fseek(file->jpgFile, 0, SEEK_SET);

	for (i = 0; i < file -> fileSize; i++) {
		curChar = fgetc(file->jpgFile);

		for (k = 0; k < 8; k++) {
			// Not 100% sure that this is correct.
			bits[i + k] = (curChar & 128);
			curChar <<= 1;
		}
	}
}

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

    // Add the rest of the filename
    // From (and including) '.' to the end of the filename
    // Example: "example_copy.jpg" <- example.jpg
    strcat(appendedString, &jpgSourceFileName[i]);

    // Print the resulting string
    if (DEBUG) printf("TEST: %s\n", appendedString);

    return appendedString;
}

// Clone a JPG file into a newly created JPG file
// jpgInputFile must already be openned elsewhere
// jpgCopyFileName will be opened in this function
JPGFile *copyJPG(FILE *jpgSource, char *jpgCopyFileName)
{
    FILE *jpgCopy;
    int i = 0;
    int jpgLength;
	JPGFile *file = (JPGFile*)malloc(sizeof(JPGFile));

	// Prepare our struct to store file information
	if (file == NULL) {
		printf("FUZZER: Unable to allocate memory for an object.\n");
		return NULL;
	}

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
    file->jpgFile = jpgCopy;
	if (DEBUG) printf("Stored jpgCopy successfully.\n");
	file->fileName = jpgCopyFileName;
	if (DEBUG) printf("Stored jpgCopyFileName successfully.\n");
	file->fileSize = jpgLength;
	if (DEBUG) printf("Stored jpgLength successfully.\n");

	if (DEBUG) printf("Created JPGFile successfully.\n");
	return file;
}

// Generate the string used for the system call to execute the jpg2pdf program
char *createSystemString(char *jpgFileName, int fileNumber)
{
    char *systemString = malloc(sizeof(char) * 100);

    sprintf(systemString, "./jpg2pdf-%d %s > /dev/null", fileNumber, jpgFileName);

    if (DEBUG) printf("System String: %s\n", systemString);
    return systemString;
}

int main(int argc, char *argv[])
{
    // A set of strings that represent the jpg2pdf versions
    char version[10][10] = {
        "jpg2pdf-0", "jpg2pdf-1", "jpg2pdf-2", "jpg2pdf-3", "jpg2pdf-4",
        "jpg2pdf-5", "jpg2pdf-6", "jpg2pdf-7", "jpg2pdf-8", "jpg2pdf-9",
    };

    char *jpgSourceName = argv[1];
    int start = atoi(argv[2]);
    int end = atoi(argv[3]);
    FILE *jpgSource = NULL;
    char *systemString = NULL;

    if (DEBUG) printf("1: %s\n2:%s\n3:%s\n", argv[1], argv[2], argv[3]);

	if (argc != 4) {
		printf("Error: Incorrect execution format.\n");
		printf("Execution format: fuzzer [JPG] [start] [end]\n");
        printf("Example: fuzzer example.jpg 0 9\n");
        printf("[JPG] is a JPG file that needs to include .jpg\n");
        printf("[start] and [end] represent the range of bugs being tested\n");
        printf("[start] and [end] should be integers between 0 and 9\n");
		return 0;
	}

    // TEST
    char *jpgCopyFileName = createJPGCopyFileName(jpgSourceName);

    // Open the source JPG file
    jpgSource = fopen(jpgSourceName, "rb");
    if (jpgSource == NULL) {
        printf("FUZZER: Cannot open source JPG\n");
        return -1;
    }

    // Create a copy of the source JPG file
	JPGFile *jpgCopy = copyJPG(jpgSource, jpgCopyFileName);
    if (jpgCopy == NULL) {
        // This line needs to be fixed (argv[2] is no longer the copy string)
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

    for (i = start; i <= end; i++)
    {
        // Create the command to have the system execute
        char *systemString = createSystemString(jpgCopyFileName, i);

        crashDetected = 0;

        // This currently isn't working; the return value isn't correct
        while (!crashDetected) {
            /* generate JPG */

            systemReturnValue = system(systemString);
            if (DEBUG) printf(" *** %d ***", systemReturnValue);

            if (systemReturnValue == 11) {
                printf("\nProgram crash in jpg2pdf-%d!\n", i);
                // Save the JPG file
                crashDetected = 1;
            } else if (systemReturnValue == -1) {
                failureCount++;
            } else if (systemReturnValue == 0) {
                successCount++;
            }

            // This is here just for testing
            if (iterationCount > 100) {
                break;
            }

            printf("\r%d iterations (%d successes, %d failures)", iterationCount++, successCount, failureCount);
            fflush(stdout);
        }
    }

    printf("\n");

    // Clean up
    free(systemString);
    fclose(jpgSource);
    free(jpgCopy);


    // Signal that the program reached the end of execution...
    if (DEBUG) printf("The end of the road...\n");

    return 0;
}
