/*
 *	Many filter
 *	Gut asked us to develop many filter, so I threw them all in a single file because I'm lazy (sorry)
 *	Works only with 24-bits BMP files 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void saveImage(char *name, unsigned char *data, unsigned int dataSize, HEADER *imgHeader, INFOHEADER *imgInfo);

unsigned int mask[3][3] = {{1, 1, 1},
							{1, 2, 1},
							{1, 1, 1}};

unsigned int negMask[3][3] = {{-1, -1, -1},
							  {-1,  8, -1},
							  {-1, -1, -1}};


int main(int argc, char const *argv[]) {

	FILE *fp;
	fp = fopen(argv[1], "r");
	if(fp == NULL) {
		fprintf(stderr, "%s %s\n", "Erro ao abrir o arquivo", argv[1]);
		return -1;
	}

	HEADER imgHeader;
	fread(&imgHeader, sizeof(HEADER), 1, fp);

	if(imgHeader.type != 0x4d42) {
		fprintf(stderr, "%s\n", "Formato inválido");
		return -1;
	}

	printf("File size: %d\nFile header: %x\n", imgHeader.size, imgHeader.type);
	printf("Image data starts at: %d\n", imgHeader.offset);

	INFOHEADER imgInfo;
	fread(&imgInfo, sizeof(INFOHEADER), 1, fp);

	if(imgInfo.compression) {
		fprintf(stderr, "%s\n", "Imagens comprimidas não são suportadas");
		return -1;
	}
	if(imgInfo.bits != 24) {
		fprintf(stderr, "%s\n", "Apenas imagens de 24 bits são suportadas");
		return -1;
	}
	printf("Tamanho da imagem: %d x %d\n", imgInfo.width, imgInfo.height);

	unsigned int dataSize = imgHeader.size - imgHeader.offset;

	fseek(fp, imgHeader.offset, SEEK_SET);
	unsigned char *srcImage = malloc(dataSize);
	unsigned char *bwImage  = malloc(dataSize);
	unsigned char *dstImage = malloc(dataSize);
	if(srcImage == NULL || dstImage == NULL || bwImage == NULL) {
		fprintf(stderr, "%s\n", "Erro ao alocar memória para as matrizes");
	}
	fread(srcImage, 1, dataSize, fp);
	fclose(fp);

	char chave[255];
	unsigned char limiar;
	printf("Insira um limiar: ");
	scanf("%d", &limiar);
	printf("Insira uma chave: ");
	scanf("%s", chave);
	unsigned int chaveLen = strlen(chave) - 1;

	// Convert to greyscale
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int accumulator = 0;
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
				accumulator += srcImage[baseAddress + c];
			}
			unsigned char meanValue = accumulator/3;
			bwImage[baseAddress]     = meanValue;
			bwImage[baseAddress + 1] = meanValue;
			bwImage[baseAddress + 2] = meanValue;
		}
	}

	char dstName[255];
	snprintf(dstName, sizeof(dstName), "bw_%s", argv[1]);
	saveImage(dstName, bwImage, dataSize, &imgHeader, &imgInfo);

	// Blur the image
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
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
						accumulator += bwImage[neighborAddress + c] * mask[k][l];
					}
				}
				dstImage[baseAddress + c] = accumulator/maskK;
			}
		}
	}

	snprintf(dstName, sizeof(dstName), "blurred_%s", argv[1]);
	saveImage(dstName, dstImage, dataSize, &imgHeader, &imgInfo);

	// Filter with negative values
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
				int accumulator = 0;
				int maskK = 0;
				for (int k = 0; k < 3; ++k) {
					for (int l = 0; l < 3; ++l) {
						// each neighbor
						int neighborX = j + k - 1;
						int neighborY = i + l - 1;
						if (neighborX < 0 || neighborX > imgInfo.width || neighborY < 0 || neighborY > imgInfo.height) {
							continue;
						}
						unsigned int neighborAddress = pixelAddress(neighborX, neighborY, imgInfo.width);
						maskK += negMask[k][l];
						accumulator += bwImage[neighborAddress + c] * negMask[k][l];
					}
				}
				// anything negatuve becomes 0
				dstImage[baseAddress + c] = accumulator/maskK > 0 ? accumulator/maskK : 0;
			}
		}
	}

	snprintf(dstName, sizeof(dstName), "gut_filtered_%s", argv[1]);
	saveImage(dstName, dstImage, dataSize, &imgHeader, &imgInfo);

	// Threshold the image
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
				dstImage[baseAddress + c] = bwImage[baseAddress + c] > limiar ? 255 : 0;
			}
		}
	}

	snprintf(dstName, sizeof(dstName), "thresholded_%s", argv[1]);
	saveImage(dstName, dstImage, dataSize, &imgHeader, &imgInfo);

	// Encrypt the image
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
				unsigned int byteAddress = baseAddress + c;
				dstImage[byteAddress] = bwImage[byteAddress] ^ chave[baseAddress % chaveLen];
			}
		}
	}

	snprintf(dstName, sizeof(dstName), "encrypted_%s", argv[1]);
	saveImage(dstName, dstImage, dataSize, &imgHeader, &imgInfo);
	
	for (int i = 0; i < imgInfo.height; ++i) {
		for (int j = 0; j < imgInfo.width; ++j) {
			// each pixel
			unsigned int baseAddress = pixelAddress(j, i, imgInfo.width);
			for (int c = 0; c < 3; ++c) {
				// each channel
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
	snprintf(dstName, sizeof(dstName), "my_filter_%s", argv[1]);
	saveImage(dstName, dstImage, dataSize, &imgHeader, &imgInfo);

	free(srcImage);
	free(bwImage);
	free(dstImage);
	return 0;
}

unsigned int pixelAddress(unsigned int x, unsigned int y, unsigned int width) {
	return x*3 + (y * width * 3);
}

void saveImage(char *name, unsigned char *data, unsigned int dataSize, HEADER *imgHeader, INFOHEADER *imgInfo) {
	FILE *fp;
	fp = fopen(name, "w");
	fwrite(imgHeader, sizeof(HEADER), 1, fp);
	fwrite(imgInfo, sizeof(INFOHEADER), 1, fp);
	fseek(fp, imgHeader->offset, SEEK_SET);
	fwrite(data, 1, dataSize, fp);
	fclose(fp);
}