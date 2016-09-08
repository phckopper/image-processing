/*
 *	Blur
 *	Blurs an image in-place using a 3x3 weighting mask
 *	Works only with 24-bits BMP files 
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct __attribute__((__packed__)) {
    unsigned short type;                 /* Magic identifier            */
    unsigned int size;                       /* File size in bytes          */
    unsigned int reserved;
    unsigned int offset;                     /* Offset to image data, bytes */
} HEADER;

typedef struct __attribute__((__packed__)) {
    unsigned int size;               /* Header size in bytes      */
    int width, height;                /* Width and height of image */
    unsigned short planes;       /* Number of colour planes   */
    unsigned short bits;         /* Bits per pixel            */
    unsigned int compression;        /* Compression type          */
    unsigned int imagesize;          /* Image size in bytes       */
    int x_resolution, y_resolution;     /* Pixels per meter          */
    unsigned int n_colours;           /* Number of colours         */
    unsigned int important_colours;   /* Important colours         */
} INFOHEADER;

unsigned int pixelAddress(unsigned int x, unsigned int y, unsigned int width);

unsigned int mask[3][3] = {{1, 1, 1},
							{1, 2, 1},
							{1, 1, 1}};


int main(int argc, char const *argv[]) {

	FILE *fp;
	fp = fopen(argv[1], "r+");
	if(fp == NULL) {
		fprintf(stderr, "%s %s\n", "Error opening file", argv[1]);
		return -1;
	}

	HEADER imgHeader;
	fread(&imgHeader, sizeof(HEADER), 1, fp);

	if(imgHeader.type != 0x4d42) {
		fprintf(stderr, "%s\n", "Not a BMP file");
		return -1;
	}

	printf("File size: %d\nFile header: %x\n", imgHeader.size, imgHeader.type);
	printf("Image data starts at: %d\n", imgHeader.offset);

	INFOHEADER imgInfo;
	fread(&imgInfo, sizeof(INFOHEADER), 1, fp);

	if(imgInfo.compression) {
		fprintf(stderr, "%s\n", "I don't know how to handle compression");
		return -1;
	}
	if(imgInfo.bits != 24) {
		fprintf(stderr, "%s\n", "Only 24-bits images are supported yet");
		return -1;
	}
	printf("Image size: %d x %d\n", imgInfo.width, imgInfo.height);

	unsigned int dataSize = imgHeader.size - imgHeader.offset;

	fseek(fp, imgHeader.offset, SEEK_SET);
	unsigned char *srcImage = malloc(dataSize);
	unsigned char *dstImage = malloc(dataSize);
	if(srcImage == NULL || dstImage == NULL) {
		fprintf(stderr, "%s\n", "Error allocating memory");
	}
	fread(srcImage, 1, dataSize, fp);

	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			for (int c = 0; c < 3; ++c) {
				// each channel
				unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
				unsigned int accumulator = 0;
				unsigned int maskK = 0;
				for (int k = 0; k < 3; ++k) {
					for (int l = 0; l < 3; ++l) {
						// each neighbor
						int neighborX = j + k - 1;
						int neighborY = i + l - 1;
						if (neighborX < 0 || neighborX > imgInfo.width || neighborY < 0 || neighborY > imgInfo.height) {
							continue;
						}
						unsigned int neighborAddress = pixelAddress(neighborX, neighborY, imgInfo.width);
						maskK += mask[k][l];
						accumulator += srcImage[neighborAddress + c] * mask[k][l];
					}
				}
				dstImage[baseAddress + c] = accumulator/maskK;
			}
		}
	}


	fseek(fp, imgHeader.offset, SEEK_SET);
	fwrite(dstImage, 1, dataSize, fp);

	fclose(fp);
	free(srcImage);
	free(dstImage);
	return 0;
}

unsigned int pixelAddress(unsigned int x, unsigned int y, unsigned int width) {
	return x*3 + (y * width * 3);
}