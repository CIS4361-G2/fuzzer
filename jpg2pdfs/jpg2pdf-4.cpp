#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

#define FALSE               0
#define TRUE                1

WORD  swapEndian(WORD S);
DWORD getFileSize(FILE *fp);
BOOL  copyStream(FILE *src, FILE *dest);
BOOL  getJPGSize(FILE *jpgStream, WORD *aWidth, WORD *aHeight, BOOL *cmyk);
void  writeCrossReferenceTable(FILE *aStream, DWORD objectPosArray[], int Count);
void  writeContentsObject(FILE *aStream, DWORD objectPosArray[], int *objectIndex, int w, int h);
int   jpgToPDF(const char *openName, const char *saveName);

int main(int argc, char *argv[])
{
    if (argc == 2) {
        return jpgToPDF(argv[1], "jpg2pdf.pdf");
    } else {
        printf("Error: Incorrect execution format.\n");
        printf("Execution format: jpg2pdf [inputFileName]\n");
        return -1;
    }
}

WORD swapEndian(WORD S)
{
    return ((S & 0x00FF) << 8) | ((S & 0xFF00) >> 8);
}

DWORD getFileSize(FILE *fp)
{
    int pos;
    DWORD size;

    pos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, pos, SEEK_SET);

    return size;
}

BOOL copyStream(FILE *src, FILE *dest)
{
    BYTE  *buffer;
    int   pos;
    DWORD fileSize;

    pos = ftell(src);
    fileSize = getFileSize(src);

    buffer = (BYTE *)malloc(fileSize);

    if (buffer == NULL) {
        return FALSE;
    }

    fseek(src, 0, SEEK_SET);
    fread(buffer, 1, fileSize, src);
    fwrite(buffer, 1, fileSize, dest);
    free(buffer);

    fseek(src, pos, SEEK_SET);

    return TRUE;
}

BOOL getJPGSize(FILE *jpgStream, WORD *aWidth, WORD *aHeight, BOOL *cmyk)
{
    WORD wrk;
    BYTE sampling;

    WORD SOF0 = 0xFFC0; /* Normal */
    WORD SOF2 = 0xFFC2; /* Progressive */

    /* JFIF */
    if (fread(&wrk, 2, 1, jpgStream) < 1) {
        return FALSE;
    }

    if (swapEndian(wrk) != 0xFFD8) {
        return FALSE;
    }

    while (1) {
        if (fread(&wrk, 2, 1, jpgStream) < 1) {
            return FALSE;
        }

        wrk = swapEndian(wrk);

        /* JPG Maker */
        if ((wrk == SOF0) | (wrk == SOF2)) {
            /* Skip Segment Length  */
            if (fseek(jpgStream, ftell(jpgStream) + 2, SEEK_SET)) {
                return FALSE;
            }

            /* Skip Sample */
            if (fseek(jpgStream, ftell(jpgStream) + 1, SEEK_SET)) {
                return FALSE;
            }

            /* Height */
            if (fread(&wrk, 2, 1, jpgStream) < 1) {
                return FALSE;
            }

            *aHeight = swapEndian(wrk);

            /* Width */
            if (fread(&wrk, 2, 1, jpgStream) < 1) {
                return FALSE;
            }

            *aWidth = swapEndian(wrk);

            /* ColorMode */
            if (fread(&sampling, 1, 1, jpgStream) < 1) {
                return FALSE;
            }

            switch (sampling) {
                case 3: /* RGB  */
                    *cmyk = FALSE;
                    break;
                case 4: /* CMYK */
                    *cmyk = TRUE;
                    break;
                default: /* ???  */
                    return FALSE;
            }

            return TRUE;
        // } else if ((wrk == 0xFFFF) | (wrk == 0xFFD9)) {
        } else if ((wrk == 0xFFFF) || (wrk == 0xFFD9)) {
            return FALSE;
        }

        /* Skip Segment */
        if (fread(&wrk, 2, 1, jpgStream) < 1) {
            return FALSE;
        }

        if (fseek(jpgStream, ftell(jpgStream) + swapEndian(wrk) - 2, SEEK_SET)) {
            return FALSE;
        }
    }
}

void writeCrossReferenceTable(FILE *aStream, DWORD objectPosArray[], int Count)
{
    int i;

    fprintf(aStream, "xref\n");
    fprintf(aStream, "0 %d\n", Count + 1);
    fprintf(aStream, "0000000000 65535 f \n");

    for (i = 0; i <= Count - 1; i++) {
        fprintf(aStream, "%0.10lu 00000 n \n", objectPosArray[i]);
    }
}

void writeContentsObject(FILE *aStream, DWORD objectPosArray[], int *objectIndex, int w, int h)
{
    int length;

    /* Contents */
/* BUG */
    // objectPosArray[*objectIndex]  = (DWORD)ftell(aStream);
    objectPosArray[*objectIndex + w]  = (DWORD)ftell(aStream);
/* BUG */

    fprintf(aStream, "%d 0 obj\n", *objectIndex + 1);
    fprintf(aStream, "<< /Length %d 0 R >>\n", *objectIndex + 2);
    fprintf(aStream, "stream\n");

    /* stream */
    length = ftell(aStream);
    fprintf(aStream, "q\n");
    fprintf(aStream, "%d 0 0 %d 0 0 cm\n", w, h);
    fprintf(aStream, "/Im0 Do\n");
    fprintf(aStream, "Q\n");
    length = ftell(aStream) - length;

    fprintf(aStream, "endstream\n");
    fprintf(aStream, "endobj\n");
    *objectIndex = *objectIndex + 1;

    /* stream length */
    objectPosArray[*objectIndex] = (DWORD)ftell(aStream);
    fprintf(aStream, "%d 0 obj\n", *objectIndex + 1);
    fprintf(aStream, "%d\n", length);
    fprintf(aStream, "endobj\n");
    *objectIndex = *objectIndex + 1;
}

int jpgToPDF(const char *openName, const char *saveName)
{
    BOOL  cmyk;
    WORD  w, h;
    int   objectIndex;
    DWORD objectPosArray[10];
    FILE  *jpgStream, *aStream;

    objectIndex = 0;

    /* Open JPG File */
    jpgStream = fopen(openName, "rb");
    if (jpgStream == NULL) {
        printf("Error: Can not open file.\n");
        return -1;
    }

    /* Get JPG size */
    if (getJPGSize(jpgStream, &w, &h, &cmyk) == FALSE) {
        printf("Error: Can not get JPG size.\n");
        // return -1;
    }

    /* Create PDF File */
    aStream = fopen(saveName, "wb+");
    if (aStream == NULL) {
        printf("Error: Can not create file.\n");
        fclose(jpgStream);
        return -1;
    }

    /* ------------------------------------------------------------- */
    /*  Writting PDF                                                 */
    /* ------------------------------------------------------------- */

    /* PDF version */
    fprintf(aStream, "%%PDF-1.2\n");

    /* Catalog */
    objectPosArray[objectIndex] = ftell(aStream);
    fprintf(aStream, "%d 0 obj\n", objectIndex + 1);
    fprintf(aStream, "<<\n");
    fprintf(aStream, "/Type /Catalog\n");
    fprintf(aStream, "/Pages 2 0 R\n");
    /* View Option (100%) */
    /*fprintf(aStream,"/OpenAction [3 0 R /XYZ -32768 -32768 1 ]\n"); */
    fprintf(aStream, ">>\n");
    fprintf(aStream, "endobj\n");
    objectIndex++;

    /* Parent Pages */
    objectPosArray[objectIndex] = ftell(aStream);
    fprintf(aStream, "%d 0 obj\n", objectIndex + 1);
    fprintf(aStream, "<<\n");
    fprintf(aStream, "/Type /Pages\n");
    fprintf(aStream, "/Kids [ 3 0 R ]\n");
    fprintf(aStream, "/Count 1\n");
    fprintf(aStream, ">>\n");
    fprintf(aStream, "endobj\n");
    objectIndex++;

    /* Kids Page */
    objectPosArray[objectIndex] = ftell(aStream);
    fprintf(aStream, "%d 0 obj\n", objectIndex + 1);
    fprintf(aStream, "<<\n");
    fprintf(aStream, "/Type /Page\n");
    fprintf(aStream, "/Parent 2 0 R\n");
    fprintf(aStream, "/Resources\n");
    fprintf(aStream, "<<\n");
    fprintf(aStream, "/XObject << /Im0 4 0 R >>\n");
    fprintf(aStream, "/ProcSet [ /PDF /ImageC ]\n");
    fprintf(aStream, ">>\n");
    fprintf(aStream, "/MediaBox [ 0 0 %d %d ]\n", w, h);
    fprintf(aStream, "/Contents 5 0 R\n");
    fprintf(aStream, ">>\n");
    fprintf(aStream, "endobj\n");
    objectIndex++;

    /* XObject Resource */
    objectPosArray[objectIndex] = ftell(aStream);
    fprintf(aStream, "%d 0 obj\n", objectIndex + 1);

    fprintf(aStream, "<<\n");
    fprintf(aStream, "/Type /XObject\n");
    fprintf(aStream, "/Subtype /Image\n");
    fprintf(aStream, "/Name /Im0\n");
    fprintf(aStream, "/Width %d\n", w);
    fprintf(aStream, "/Height %d\n", h);
    fprintf(aStream, "/BitsPerComponent 8\n");
    fprintf(aStream, "/Filter [/DCTDecode]\n");

    if (cmyk == FALSE) {
        fprintf(aStream, "/ColorSpace /DeviceRGB\n");
    } else {
        fprintf(aStream, "/ColorSpace /DeviceCMYK\n");
        fprintf(aStream, "/Decode[1 0 1 0 1 0 1 0]\n"); /* Photoshop CMYK (NOT BIT) */
    }

    fprintf(aStream, "/Length %lu >>\n", getFileSize(jpgStream));
    fprintf(aStream, "stream\n");

    if (copyStream(jpgStream, aStream) == FALSE) {
        printf("Error : No Memory \n");
        return -1;
    }

    fprintf(aStream, "endstream\n");
    fprintf(aStream, "endobj\n");
    objectIndex++;

    /* Contents Stream & Object */
    writeContentsObject(aStream, objectPosArray, &objectIndex, w, h);

    /* CrossReferenceTable */
    objectPosArray[objectIndex] = ftell(aStream);
    writeCrossReferenceTable(aStream, objectPosArray, (int)objectIndex);

    /* trailer */
    fprintf(aStream, "trailer\n");
    fprintf(aStream, "<<\n");
    fprintf(aStream, "/Size %d\n", objectIndex + 1);
    fprintf(aStream, "/Root 1 0 R\n");
    fprintf(aStream, ">>\n");
    fprintf(aStream, "startxref\n");
    fprintf(aStream, "%lu\n", objectPosArray[objectIndex]);
    fprintf(aStream, "%%%%EOF\n");

    fclose(jpgStream); fclose(aStream);

    printf("\nSuccess!\n");

    return 0;
}
