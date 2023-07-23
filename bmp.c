#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BYTE	uint8_t
#define WORD	uint16_t
#define DWORD	uint32_t
#define LONG	uint32_t

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

int main(int argc, char* argv[])
{
	FILE* fp;
	long int size;
	uint32_t lineByte, linePad, pixelBytes;
	BYTE* inbuf;
	BYTE* p;
	BYTE* q;
	BITMAPFILEHEADER* fh;
	BITMAPINFOHEADER* ih;
	
	if(argc < 2)
	{
		printf("Usage: %s bmpfile\n", argv[0]);
		return 0;
	}
	
	fp = fopen(argv[1], "rb");
	if(NULL == fp)
	{
		printf("Cannot open %s \n", argv[1]);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("%s has %d bytes\n", argv[1], size);
	printf("BITMAPFILEHEADER has %d bytes | BITMAPINFOHEADER has %d bytes\n", (int)sizeof(BITMAPFILEHEADER),(int)sizeof(BITMAPINFOHEADER));
	inbuf = (BYTE*)malloc(size);
	if(NULL == inbuf)
	{
		printf("Cannot malloc %d bytes\n", size);
		fclose(fp);
		return 0;
	}
	if(1 != fread(inbuf, size, 1, fp))
	{
		printf("Cannot read %d bytes\n", size);
		free(inbuf);
		fclose(fp);
		return 0;
	}
	fclose(fp);
	
	p = inbuf;
	fh = (BITMAPFILEHEADER*)p;
	p += sizeof(BITMAPFILEHEADER);
	ih = (BITMAPINFOHEADER*)p;
	p += sizeof(BITMAPINFOHEADER);
	printf("BITMAPFILEHEADER.bfType = %d\n", fh->bfType);
	printf("BITMAPFILEHEADER.bfSize = %d\n", fh->bfSize);
	printf("BITMAPFILEHEADER.bfOffBits = %d\n", fh->bfOffBits);
	printf("--------------------------------\n");
	printf("BITMAPINFOHEADER.biSize = %d\n", ih->biSize);
	printf("BITMAPINFOHEADER.biWidth = %d\n", ih->biWidth);
	printf("BITMAPINFOHEADER.biHeight = %d\n", ih->biHeight);
	printf("BITMAPINFOHEADER.biBitCount = %d\n", ih->biBitCount);
	printf("BITMAPINFOHEADER.biCompression = %d\n", ih->biCompression);
	printf("BITMAPINFOHEADER.biSizeImage = %d\n", ih->biSizeImage);
	
	lineByte = ih->biSizeImage / ih->biHeight;
	pixelBytes = (ih->biBitCount >> 3);
	linePad = lineByte - (ih->biWidth * pixelBytes);
	printf("Each line has %d bytes, Each pixel has %d bytes. Pading: %d bytes\n", lineByte, pixelBytes, linePad);
	printf("-------------------------------------------------------------------------\n");
	printf("static const unsigned int xbmp[%d * %d] = \n", ih->biWidth, ih->biHeight);
	printf("{\n");
	q = p + lineByte * (ih->biHeight - 1);
	while(q >= p)
	{
		for(int i=0; i<ih->biWidth * pixelBytes; i+=pixelBytes)
		{
			if(q == p && i == (ih->biWidth - 1) * pixelBytes)
				printf("0xFF%02X%02X%02X", q[i], q[i+1], q[i+2]);  // the last one 
			else
				printf("0xFF%02X%02X%02X,", q[i], q[i+1], q[i+2]);
		}
		printf("\n");
		q -= lineByte;
	}
	printf("};\n");
	if(NULL != inbuf) free(inbuf);
	return 0;

}