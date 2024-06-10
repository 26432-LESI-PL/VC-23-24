//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"
#include <math.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar mem�ria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

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

// Libertar mem�ria de uma imagem
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
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)))
			;
		if (c != '#')
			break;
		do
			c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF)
			break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#')
			ungetc(c, file);
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
				// datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

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

		if (strcmp(tok, "P4") == 0)
		{
			channels = 1;
			levels = 1;
		} // Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0)
			channels = 1; // Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0)
			channels = 3; // Se PPM (RGB [0,MAX(level,255)])
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

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

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

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

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

	if (image == NULL)
		return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

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

// Função para calcular fminf e fmaxf
float fminf(float a, float b)
{
	return a < b ? a : b;
}

float fmaxf(float a, float b)
{
	return a > b ? a : b;
}

/*
float fmodf(float a, float b)
{
	return a - (int)(a / b) * b;
}*/

int vc_3channels_to_1(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (src->channels != 3)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 3 canais\n");
		return 0;
	}
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (dst->channels != 1)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 1 canal\n");
		return 0;
	}
	if (src->width != dst->width || src->height != dst->height)
	{
		printf("(vc_3chanels_to_1) Imagens têm tamanhos diferentes\n");
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data_dst[y * width + x] = (unsigned char)(0.299 * data_src[pos] + 0.587 * data_src[pos + 1] + 0.114 * data_src[pos + 2]);
		}
	}

	return 1;
}

int vc_3channels_to_1_binary(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (src->channels != 3)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 3 canais\n");
		return 0;
	}
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
	{
		printf("(vc_3chanels_to_1) Tamanhos inválidos\n");
		return 0;
	}
	if (dst->channels != 1)
	{
		printf("(vc_3chanels_to_1) Imagem tem de ter 1 canal\n");
		return 0;
	}
	if (src->width != dst->width || src->height != dst->height)
	{
		printf("(vc_3chanels_to_1) Imagens têm tamanhos diferentes\n");
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data_dst[y * width + x] = data_src[pos];
		}
	}

	return 1;
}

// Função para converter uma imagem RGB para uma imagem binária
int vc_rgb_to_binary(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, y, cb, cr;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Conversão de RGB para YCbCr
		y = 0.299f * r + 0.587f * g + 0.114f * b;
		cb = 128 - 0.168736f * r - 0.331264f * g + 0.5f * b;
		cr = 128 + 0.5f * r - 0.418688f * g - 0.081312f * b;

		// Conversão de YCbCr para RGB
		r = y + 1.402f * (cr - 128);
		g = y - 0.344136f * (cb - 128) - 0.714136f * (cr - 128);
		b = y + 1.772f * (cb - 128);

		// Conversão de RGB para binário
		r = fminf(255.0f, fmaxf(0.0f, r));
		g = fminf(255.0f, fmaxf(0.0f, g));
		b = fminf(255.0f, fmaxf(0.0f, b));

		if (r > 127 || g > 127 || b > 127)
		{
			data[i] = 0;
			data[i + 1] = 0;
			data[i + 2] = 0;
		}
		else
		{
			data[i] = 255;
			data[i + 1] = 255;
			data[i + 2] = 255;
		}
	}

	return 1;
}


/* Função para converter uma imagem RGB para uma imagem HSV
 * Hue - 0 a 255 (convertido de 0 a 360 graus)
 * Saturation - 0 a 255
 * Value - 0 a 255 
 */
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
	if ((width <= 0) || (height <= 0) || (data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		b = (float)data[i];
		g = (float)data[i + 1];
		r = (float)data[i + 2];

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

// Função que receba uma imagem HSV e retorne uma imagem com 1 canal (admitindo valores entre 0 e 255 por pixel)
int vc_hsv_segmentation(IVC* src, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{

	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	float h, s, v;
	int i, size;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{

		h = (float)data[i] * 360.0f / 255.0f;
		s = (float)data[i + 1] * 100.0f / 255.0f;
		v = (float)data[i + 2] * 100.0f / 255.0f;

		if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax)
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

int vc_scale_gray_to_rgb(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL)
		return 0;

	// Verifica se a imagem de entrada está em escala de cinza
	if (src->channels != 1)
		return 0;

	int width = src->width;
	int height = src->height;
	int channels = dst->channels;
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// Obtém o valor do pixel na imagem em escala de cinza
			unsigned char gray_value = src->data[y * width + x];

			// Calcula os componentes R, G e B com base no valor de cinza
			unsigned char r, g, b;

			if (gray_value < 128)
			{
				// Azul aumenta de intensidade, verde diminui
				b = 255;
				g = 255 - (2 * gray_value);
				r = 0;
			}
			else
			{
				// Vermelho aumenta de intensidade, verde diminui
				r = 255;
				g = 2 * (gray_value - 128);
				b = 0;
			}

			// Define os componentes RGB no destino
			dst->data[(y * width + x) * channels] = r;	   // R
			dst->data[(y * width + x) * channels + 1] = g; // G
			dst->data[(y * width + x) * channels + 2] = b; // B
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL)
		return 0;

	// Verifica se a imagem de entrada está em RGB e se a de saída tem apenas um canal
	if (src->channels != 3 || dst->channels != 1)
		return 0;

	int width = src->width;
	int height = src->height;
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// Obtém os valores dos canais de cor RGB
			unsigned char r = src->data[(y * width + x) * src->channels];
			unsigned char g = src->data[(y * width + x) * src->channels + 1];
			unsigned char b = src->data[(y * width + x) * src->channels + 2];

			// Calcula o valor de cinza usando a média ponderada
			unsigned char gray_value = (r * 0.299) + (g * 0.587) + (b * 0.114);

			// Define o valor de cinza na imagem de destino
			dst->data[y * width + x] = gray_value;
		}
	}

	return 1;
}

int vc_gray_to_binary(IVC* srcdst, int threshold)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] < threshold)
			{
				data[pos] = 0;
			}
			else
			{
				data[pos] = 255;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_media(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;
	double media = 0;

	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			media += data[pos];
		}
	}

	media = media / (width * height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x;
			// Compara o valor do pixel com a média e binariza
			data[pos] = (data[pos] < media) ? 0 : 255;
		}
	}

	return 1;
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel)
{

	unsigned char* datasrc = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	;
	int channels = src->channels;
	int x, y, kx, ky, min, max;
	long int pos, posk;
	int offset = (kernel - 1) / 2; // Calculo do valor do offset
	unsigned char threshold = 0.0;

	unsigned char* datadst = (unsigned char*)dst->data;

	if (src->height <= 0 || src->width <= 0 || src->height <= 0 || src->width <= 0 || src == NULL || dst == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0; // Verifica se as imagens têm os canais corretos

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = 0;
			min = 255;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[posk] > max)
							max = datasrc[posk];
						if (datasrc[posk] < min)
							min = datasrc[posk];
					}
				}
			}

			threshold = (unsigned char)((float)(min + max) / (float)2);

			if (datasrc[pos] > threshold)
			{
				datadst[pos] = 255;
			}
			else
			{
				datadst[pos] = 0;
			}
		}
	}
	return 1;
}

int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin)
{
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int levels = src->levels;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	unsigned char threshold;
	int offset = (kernel - 1) / 2;
	int max, min;

	// Verificação de erros
	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 1)
		return 0;

	// Percorre todos os pixels da imagem de entrada
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = 0;
			min = 255;

			// NxM vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						if (data[posk] > max)
							max = data[posk];
						if (data[posk] < min)
							min = data[posk];
					}
				}
			}

			if ((max - min) < cmin)
			{
				threshold = (unsigned char)(float)levels / (float)2;
			}
			else
			{
				threshold = (unsigned char)((float)(max + min) / (float)2);
			}

			if (data[pos] > threshold)
			{
				data_dst[pos] = 255;
			}
			else
			{
				data_dst[pos] = 0;
			}
		}
	}
	return 1;
}

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int levels = src->levels;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, counter, yy, xx, max, min;
	unsigned char threshold;
	int offset = (kernel - 1) / 2;
	float media, desvio;

	// Verificação de erros
	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 1)
		return 0;

	// Percorre todos os pixels da imagem de entrada
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = datasrc[pos];
			min = datasrc[pos];

			media = 0.0f;
			desvio = 0.0f;

			// Calcula a média
			for (counter = 0, yy = -offset; yy <= offset; yy++)
			{
				for (xx = -offset; xx <= offset; xx++)
				{
					if ((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;
						media += (float)datasrc[posk];
						counter++;
					}
				}
			}
			media /= counter;

			for (counter = 0, yy = -offset; yy <= offset; yy++)
			{
				for (xx = -offset; xx <= offset; xx++)
				{
					if ((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;
						desvio += powf(((float)datasrc[posk]) - media, 2);

						counter++;
					}
				}
			}

			desvio = sqrtf(desvio / counter);

			// Calcula o limiar
			threshold = (unsigned char)(media + k * desvio);

			if (datasrc[pos] > threshold)
			{
				data_dst[pos] = 255;
			}
			else
			{
				data_dst[pos] = 0;
			}
		}
	}

	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int pixel;

	if (src->width <= 0 || src->height <= 0 || src->data == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = 0; // Definindo inicialmente como preto

			for (ky = -offset; ky <= offset; ky++)
			{ // percorre cada linha do kernel
				for (kx = -offset; kx <= offset; kx++)
				{ // percorre cada coluna do kernel
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{														  // verifica se a posição é válida
						posk = (y + ky) * bytesperline + (x + kx) * channels; // calcula a posição do pixel na imagem

						if (datasrc[posk] == 255)
						{
							// Se algum pixel na vizinhança for branco, marca o pixel central como branco
							pixel = 255;
							break; // Sair do loop interno, pois já encontramos um pixel branco
						}
					}
				}
				if (pixel == 255)
					break; // Sair do loop externo se já marcamos o pixel central como branco
			}

			// Atribuindo o valor ao pixel na imagem de destino
			datadst[pos] = pixel;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int pixel;

	if (src->width <= 0 || src->height <= 0 || src->data == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = 255; // Definindo inicialmente como branco

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[posk] == 0)
						{
							// Se algum pixel na vizinhança for zero, marca o pixel central como zero
							pixel = 0;
							break; // Sair do loop interno, pois já encontramos um pixel zero
						}
					}
				}
				if (pixel == 0)
					break; // Sair do loop externo se já marcamos o pixel central como zero
			}

			// Atribuindo o valor ao pixel na imagem de destino
			datadst[pos] = pixel;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel1, int kernel2)
{
	IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (tmp == NULL)
		return 0;

	if (!vc_binary_erode(src, tmp, kernel1))
	{
		vc_image_free(tmp);
		return 0;
	}

	if (!vc_binary_dilate(tmp, dst, kernel2))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);
	return 1;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel)
{
	IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (tmp == NULL)
		return 0;

	if (!vc_binary_dilate(src, tmp, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	if (!vc_binary_erode(tmp, dst, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);
	return 1;
}

int vc_gray_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int pixel, max;

	if (src->width <= 0 || src->height <= 0 || src->data == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = 0;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[posk] > pixel)
						{
							pixel = datasrc[posk];
						}
					}
				}
			}

			datadst[pos] = pixel;
		}
	}

	return 1;
}

int vc_binary_to_gray(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL)
		return 0;

	// Verifica se a imagem de entrada está em binário e se a de saída tem apenas um canal
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int width = src->width;
	int height = src->height;
	int x, y;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			// Obtém o valor do pixel na imagem binária
			unsigned char binary_value = src->data[y * width + x];

			// Define o valor de cinza na imagem de destino
			dst->data[y * width + x] = binary_value;
		}
	}

	return 1;
}

// Etiquetagem de blobs
// src		: Imagem bin�ria de entrada
// dst		: Imagem grayscale (ir� conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma vari�vel, onde ser� armazenado o n�mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels) // identifica os blobs apenas
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
	OVC* blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
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
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

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

					// Se A est� marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

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

	// printf("\nMax Label = %d\n", label);

	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se n�o h� blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs) // os blobs acima indentificados sao um corpo
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

	// Verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta area de cada blob
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
					// area
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Per�metro
					// Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
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
		// blobs[i].xc = (xmax - xmin) / 2;
		// blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_draw_boundingbox(IVC* src, OVC* blob)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	// Desenha a bounding box
	for (y = blob->y; y < blob->y + blob->height; y++)
	{
		for (x = blob->x; x < blob->x + blob->width; x++)
		{
			if (y == blob->y || y == blob->y + blob->height - 1 || x == blob->x || x == blob->x + blob->width - 1)
			{
				pos = y * bytesperline + x * channels;
				data[pos] = 255;
			}
		}
	}

	return 1;
}

int vc_draw_center_of_mass(IVC* src, OVC* blobs, int nblobs, int tamanho_alvo, int cor)
{
	unsigned char* data_src = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = width * channels;
	int x, y;
	long int pos;
	int i;

	if (width <= 0 || height <= 0 || src->data == NULL)
	{
		printf("(vc_draw_center_of_mass) Tamanhos inválidos\n");
		return 0;
	}

	// Desenha o centro de massa
	for (i = 0; i < nblobs; i++)
	{
		for (y = -tamanho_alvo; y <= tamanho_alvo; y++)
		{
			for (x = -tamanho_alvo; x <= tamanho_alvo; x++)
			{
				// desenha a cruz
				if (y == 0 || x == 0)
				{
					pos = (y + blobs[i].yc) * bytesperline + (x + blobs[i].xc) * channels;

					if (cor == 0)
					{
						data_src[pos] = 0;
					}
					else
					{
						data_src[pos] = 255;
					}
				}
			}
		}
	}

	return 1;
}

// Funcao para normalizar a imagem com labels e diferentes escalas de cinzas (pintar objetos que sao 1 a 255)
int vc_normalizar_imagem_labelling(IVC* src, IVC* dst, int nblobs)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{

			pos = y * bytesperline + x * channels;

			for (i = 1; i <= nblobs; i++)
			{ // i = labels
				if (data_src[pos] == i)
				{
					data_dst[pos] = (i * 255) / nblobs; // Pinta os objetos de acordo com o numero de blobs --- Ex: layer 2 * 255 / 3 = 170
				}
			}
		}
	}
}

int vc_gray_histogram_show(IVC* src, IVC* dst)
{
	int x, y, i;
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int height = src->height;
	int width = src->width;
	long int histogram[256] = { 0 }; // array que vai armazenar o histograma
	int maximo = 0;
	int max_height = dst->height;

	// Verificações básicas
	if (src == NULL || src->channels != 1 || dst == NULL || dst->channels != 1)
	{
		printf("Erro: imagens de entrada e saída devem ser em tons de cinza.\n");
		return 0;
	}

	// Cálculo do histograma
	for (y = 0; y < height * width; y++)
	{
		histogram[data_src[y]]++;
	}

	// Buscar Máximo do Histograma
	for (i = 0; i < 256; i++)
	{
		if (histogram[i] > maximo)
		{
			maximo = histogram[i];
		}
	}

	// Normalização do histograma para uma imagem binária
	for (x = 0; x < 256; x++)
	{
		histogram[x] = (int)((histogram[x] * max_height) / maximo);
	}

	// Preenchimento da imagem de destino com o histograma
	for (x = 0; x < 256; x++)
	{
		for (y = height - 1; y >= 0; y--)
		{
			if (height - y <= histogram[x])
			{
				data_dst[y * dst->width + x] = 255; // Branco
			}
			else
			{
				data_dst[y * dst->width + x] = 0; // Preto
			}
		}
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst)
{
	unsigned char* data_src = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int x, y;
	int histogram[256] = { 0 }; // array que vai armazenar o histograma
	int cdf[256] = { 0 };		  // array que vai armazenar o CDF
	int total_pixels = src->width * src->height;
	int max_intensity = 255;
	int cdf_min = total_pixels;
	float g[256]; // array que vai armazenar a função de equalização
	int height = src->height;
	int width = src->width;

	if (src == NULL || src->channels != 1 || dst == NULL || dst->channels != 1)
	{
		printf("Erro: imagens de entrada e saída devem ser em tons de cinza.\n");
		return 0;
	}

	// Calcular o histograma
	for (y = 0; y < height * width; y++)
	{
		histogram[data_src[y]]++;
	}

	// Calcular o cdf
	cdf[0] = histogram[0];
	for (x = 1; x < 256; x++)
	{
		cdf[x] = cdf[x - 1] + histogram[x];
	}

	// Calcular o cdf mínimo
	for (x = 0; x < 256; x++)
	{
		if (cdf[x] < cdf_min)
		{
			cdf_min = cdf[x];
		}
	}

	// Calcular a função de equalização
	for (x = 0; x < 256; x++)
	{
		g[x] = (float)(cdf[x] - cdf_min) / (total_pixels - cdf_min);
	}

	// Aplicar a equalização aos pixels da imagem de entrada
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int index = y * width + x;
			data_dst[index] = (unsigned char)(g[data_src[index]] * max_intensity);
		}
	}

	return 1;
}

#define CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	int offset = 1;
	int pixelx, pixely;
	float grad_x, grad_y;
	float magnitude;
	int i, j;

	// Verificação de erros
	if (width <= 0 || height <= 0 || datasrc == NULL || datadst == NULL)
		return 0;
	if (channels != 1) // Verificar se a imagem tem apenas 1 canal (gray)
		return 0;

	// Aplicar o operador em x (derivada)
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			grad_x = 0.0;
			grad_y = 0.0;

			for (ky = -1; ky <= 1; ky++)
			{
				for (kx = -1; kx <= 1; kx++)
				{
					pixelx = CLAMP(x + kx, 0, width - 1);
					pixely = CLAMP(y + ky, 0, height - 1);

					posk = pixely * bytesperline + pixelx;

					grad_x += datasrc[posk] * (-1 + (kx != 0)) / 3.0;
					grad_y += datasrc[posk] * (-1 + (ky != 0)) / 3.0;
				}
			}
			// Calcular a magnitude do vetor
			magnitude = sqrt(grad_x * grad_x + grad_y * grad_y);
			// Aplicar threshold
			datadst[y * bytesperline + x] = (magnitude > th) ? 255 : 0;
		}
	}

	return 1;
}


int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, yy, xx;
	long int pos_src, pos_dst;
	int media, soma;

	// Validações
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	// Percorrer a Imagem
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			pos_dst = y * bytesperline_dst + x * channels_dst; // Posição dst
			media = 0; // Resetar a média a cada loop
			soma = 0; // Resetar a soma a cada loop

			for (yy = y - ((kernel - 1) / 2); yy <= (y + ((kernel - 1) / 2)); yy++) {
				for (xx = x - ((kernel - 1) / 2); xx <= (x + ((kernel - 1) / 2)); xx++) {

					if ((xx >= 0) && (xx < width) && (yy >= 0) && (yy < height)) {

						pos_src = yy * bytesperline_src + xx * channels_src;
						soma += datasrc[pos_src];

					}

				}
			}

			// Media = ao somatorio das posições / pelo Kernel*kernel (3x3, 5x5, 9x9)
			media = soma / (kernel * kernel);
			datadst[pos_dst] = media;
		}
	}

	return 1;
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernel) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int channels_dst = dst->channels;
	int x, y, yy, xx;
	long int pos_src, pos_dst;
	int offset = kernel / 2;
	int count;

	// Array para armazenar os pixels da vizinhança
	unsigned char* lista = (unsigned char*)malloc(kernel * kernel * sizeof(unsigned char));

	// Validações de entrada
	if (width <= 0 || height <= 0 || datasrc == NULL || datadst == NULL) {
		free(lista); // Libera a memória alocada
		return 0;
	}
	if (width != dst->width || height != dst->height || channels_src != 1 || channels_dst != 1) {
		free(lista); // Libera a memória alocada
		return 0;
	}

	// Percorrer a imagem pixel por pixel
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			count = 0;

			// Percorrer a vizinhança do pixel central (x, y)
			for (yy = y - offset; yy <= y + offset; yy++) {
				for (xx = x - offset; xx <= x + offset; xx++) {
					// Verificar se o pixel vizinho está dentro dos limites da imagem
					if (xx >= 0 && xx < width && yy >= 0 && yy < height) {
						// Calcular a posição do pixel vizinho na imagem de origem
						pos_src = yy * src->bytesperline + xx * channels_src;
						// Adicionar o valor do pixel na janela
						lista[count++] = datasrc[pos_src];
					}
				}
			}

			// Ordenar os valores na janela para encontrar a mediana
			for (int i = 0; i < count - 1; i++) {
				for (int j = i + 1; j < count; j++) {
					if (lista[i] > lista[j]) {
						// Trocar os valores se estiverem fora de ordem
						unsigned char temp = lista[i];
						lista[i] = lista[j];
						lista[j] = temp;
					}
				}
			}

			// Calcular a posição do pixel na imagem de destino
			pos_dst = y * dst->bytesperline + x * channels_dst;
			// Atribuir o valor mediano ao pixel na imagem de destino
			datadst[pos_dst] = lista[count / 2];
		}
	}

	// Libera a memória alocada para a janela
	free(lista);
	return 1;
}

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	unsigned char* data_dst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, kx, ky;
	int values[9];
	int i, j, n, tmp;
	float mask[3][3] = {
		{1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0},
		{2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0},
		{1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0}
	};

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Percorre todos os pixels da imagem de entrada
	for (y = 1; y < height - 1; y++) {
		for (x = 1; x < width - 1; x++) {
			pos = y * bytesperline + x * channels;

			// NxM vizinhos
			n = 0;
			for (ky = -1; ky <= 1; ky++) {
				for (kx = -1; kx <= 1; kx++) {
					posk = (y + ky) * bytesperline + (x + kx) * channels;
					values[n] = data[posk];
					n++;
				}
			}

			// Novo valor do pixel
			data_dst[pos] = 0;
			for (i = 0; i < 3; i++) {
				for (j = 0; j < 3; j++) {
					data_dst[pos] += values[i * 3 + j] * mask[i][j];
				}
			}
		}
	}

	return 1;
}

