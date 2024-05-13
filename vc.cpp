//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funções não seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memória de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

// Função para passar de tons de cinzento para uma escala de rgb
int grey_to_scaled_rgb(IVC* src, IVC* dst) {
	dst = vc_image_new(src->width, src->height, 3, src->levels);
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;


			if (datasrc[pos_src] <= 64 && datasrc[pos_src] > -1) {
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = datasrc[pos_src] * 4;
				datadst[pos_dst + 2] = 255;

			}
			else if (datasrc[pos_src] > 64 && datasrc[pos_src] <= 128) {
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 255 - (4 * (datasrc[pos_src] - 64));

			}
			else if (datasrc[pos_src] > 128 && datasrc[pos_src] <= 192) {
				datadst[pos_dst] = (datasrc[pos_src] - 128) * 4;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 0;

			}
			else {
				datadst[pos_dst] = 255;
				datadst[pos_dst + 1] = 255 - (4 * (datasrc[pos_src] - 192));
				datadst[pos_dst + 2] = 0;

			}
		}
	}

	vc_write_image("trnasformada.ppm", dst);

	vc_image_free(dst);

	printf("Press any key to exit...\n");
	getchar();

	return 0;
}

int vc_rgb_to_hsv(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores máximo e mínimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC* srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int h, s, v; // h=[0, 360] s=[0, 100] v=[0, 100]
	int i, size;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		h = (int)(((float)data[i]) / 255.0f * 360.0f);
		s = (int)(((float)data[i + 1]) / 255.0f * 100.0f);
		v = (int)(((float)data[i + 2]) / 255.0f * 100.0f);

		if ((h > hmin) && (h <= hmax) && (s >= smin) && (s <= smax) && (v >= vmin) && (v <= vmax))
		{
			data[i] = 255;
			data[i + 1] = 255;
			data[i + 2] = 255;
		}
		else
		{
			data[i] = 0;
			data[i + 1] = 0;
			data[i + 2] = 0;
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst) {
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int channels_src = src->channels;
	int width = src->width;
	int height = src->height;
	long int pos_src, pos_dst;
	float r, g, b;


	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL)) return 0;
	if (dst->channels > 1) {
		printf("Imagem de destino tem de ter apenas um canal\n");
	}
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			r = (float)data[pos_src];
			g = (float)data[pos_src + 1];
			b = (float)data[pos_src + 2];

			data_dst[pos_dst] = (unsigned char)((r * 0.299) + (g * 0.587) + (b * 0.114));
		}
	}


	return 1;
}

int vc_gray_to_binary(IVC* srcdst, IVC* dst, int threshold)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int i, size;

	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;

	if (dst->channels != 1 || srcdst->channels != 1) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels) {
		if (data[i] > threshold) {
			data_dst[i] = 255;
		}
		else {
			data_dst[i] = 0;
		}
	}

	return 1;
}

int global_threshold(IVC* src, IVC* dst, int threshold) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] >= threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}

	printf("Press any key to exit...\n");
	getchar();

	return 0;
}

int average_threshold(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst, soma;

	soma = 0;
	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			soma += datasrc[pos_src];
		}
	}

	int threshold = soma / (width * height);

	printf("threshold = %d", threshold);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}
	vc_write_image("segmented_imagewithglobal.pgm", dst);
	//printf("Press any key to exit...\n");
	//getchar();

	return 0;
}


int midpoint_threshold(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, kx, ky;
	int max = 0, min = 255;
	int offset = (kernel - 1) / 2;
	int counter;
	int threshold;
	long int pos_src, pos_dst, soma, posk;

	soma = 0;
	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			for (ky = -offset; ky <= offset; ky++) {

				for (kx = -offset; kx <= offset++; kx++) {
					if (x + kx > 0 && y + ky > 0 && x + kx < width && y + ky < height) {
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] > max) {
							max = datasrc[posk];
						}
						if (datasrc[posk] < min) {
							min = datasrc[posk];
						}

					}
				}
			}
			threshold = (max + min) / 2;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_src] = 255;
			}
			else {
				datadst[pos_src] = 0;
			}

		}
	}
	vc_write_image("segmented_imagewithmidpoint.pgm", dst);
	//printf("Press any key to exit...\n");
	//getchar();

	return 0;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	int x, y, i, j;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int offset = kernel / 2;

	if (src->width <= 0 || src->height <= 0 || src->channels <= 0 ||
		src->data == NULL || dst->data == NULL ||
		src->width != dst->width || src->height != dst->height || src->channels != dst->channels) {
		return 0;
	}

	// Percorre a imagem de entrada
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int max = 0;

			// Percorre o kernel
			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {
					int posx = x + i;
					int posy = y + j;

					// Verifica se a posição está dentro da imagem
					if (posx >= 0 && posx < width && posy >= 0 && posy < height) {
						int pixel_pos = (posy * width * channels) + (posx * channels);
						int pixel_value = data_src[pixel_pos];
						if (pixel_value > max) {
							max = pixel_value;
						}
					}
				}
			}

			// Define o valor do pixel de destino como o máximo encontrado
			data_dst[(y * width * channels) + (x * channels)] = max;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	int x, y, i, j;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int offset = kernel / 2;

	if (src->width <= 0 || src->height <= 0 || src->channels <= 0 ||
		src->data == NULL || dst->data == NULL ||
		src->width != dst->width || src->height != dst->height || src->channels != dst->channels) {
		return 0;
	}

	// Percorre a imagem de entrada
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int min = 255; // Assume o máximo valor de pixel

			// Percorre o kernel
			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {
					int posx = x + i;
					int posy = y + j;

					// Verifica se a posição está dentro da imagem
					if (posx >= 0 && posx < width && posy >= 0 && posy < height) {
						int pixel_pos = (posy * width * channels) + (posx * channels);
						int pixel_value = data_src[pixel_pos];
						if (pixel_value < min) {
							min = pixel_value;
						}
					}
				}
			}

			// Define o valor do pixel de destino como o mínimo encontrado
			data_dst[(y * width * channels) + (x * channels)] = min;
		}
	}

	return 1;
}

int subtraiImagem(IVC* src, IVC* dst, IVC* farmacia) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	unsigned char* datafarmacia = (unsigned char*)farmacia->data;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			datafarmacia[pos_src] = datasrc[pos_src] - datadst[pos_src];
		}
	}

	return 0;
}

int preencheImagem(IVC* src, IVC* dst, IVC* imagem) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	unsigned char* dataimagem = (unsigned char*)imagem->data;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if (src == NULL || dst == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 1;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] == 255) {
				dataimagem[pos_src] = datadst[pos_src];
			}
			else
				dataimagem[pos_src] = 0;
		}
	}

	return 0;
}

// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;

		blobs[i].xc = sumx / MAX(blobs[i].area, 1);

		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_gray_histogram_show(IVC* src)
{
	if (!src || src->width <= 0 || src->height <= 0 || src->data == NULL || src->channels != 1)
		return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline = src->bytesperline;
	int width = src->width;
	int height = src->height;

	int ni[256] = { 0 };
	float pdf[256], pdfmax = 0, pdfnorm[256];

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int pos = y * bytesperline + x;
			ni[datasrc[pos]]++;
		}
	}

	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)ni[i] / (width * height);
		if (pdf[i] > pdfmax)
			pdfmax = pdf[i];
	}

	for (int i = 0; i < 256; i++)
	{
		pdfnorm[i] = pdf[i] * 255 / pdfmax;
	}

	IVC* dst = vc_image_new(256, 256, 1, src->levels);

	unsigned char* datadst = (unsigned char*)dst->data;

	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256; y++)
		{
			if (y >= 256 - (int)pdfnorm[x])
			{
				datadst[y * dst->width + x] = 255;
			}
			else
			{
				datadst[y * dst->width + x] = 0;
			}
		}
	}

	vc_write_image("histogram.pbm", dst);
	vc_image_free(dst);

	return 1;
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst) {
	// Verifica se as imagens de entrada são válidas
	if (src == NULL || dst == NULL || src->width <= 0 || src->height <= 0 || src->channels != 1) {
		printf("Erro: imagens de entrada invalidas.\n");
		return 0;
	}

	// Aloca memória para o histograma acumulado
	int histogram[256] = { 0 };
	float acumulado[256] = { 0 };

	// Calcula o histograma da imagem em tons de cinza
	int x, y;
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			unsigned char* data = (unsigned char*)src->data;
			histogram[data[y * src->width + x]]++;
		}
	}

	// Calcula o histograma acumulado normalizado
	int total_pixels = src->width * src->height;
	acumulado[0] = (float)histogram[0] / total_pixels;
	for (int i = 1; i < 256; i++) {
		acumulado[i] = acumulado[i - 1] + (float)histogram[i] / total_pixels;
	}

	// Equaliza a imagem de acordo com o histograma acumulado
	for (y = 0; y < src->height; y++) {
		for (x = 0; x < src->width; x++) {
			unsigned char* src_data = (unsigned char*)src->data;
			unsigned char* dst_data = (unsigned char*)dst->data;
			int pixel_value = src_data[y * src->width + x];
			dst_data[y * src->width + x] = (unsigned char)(acumulado[pixel_value] * 255);
		}
	}

	return 1;
}