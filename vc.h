#pragma once
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Perímetro
	int label;					// Etiqueta
} OVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);
int grey_to_scaled_rgb(IVC* src, IVC* dst);
int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_gray_to_binary(IVC* srcdst, IVC* dst, int threshold);
int vc_rgb_to_hsv(IVC* srcdst);
int vc_hsv_segmentation(IVC* srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int global_threshold(IVC* src, IVC* dst, int threshold);
int average_threshold(IVC* src, IVC* dst);
int midpoint_threshold(IVC* src, IVC* dst, int kernel);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int subtraiImagem(IVC* src, IVC* dst, IVC* farmacia);
int preencheImagem(IVC* src, IVC* dst, IVC* imagem);
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
int global_threshold(IVC* src, IVC* dst, int threshold);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int subtraiImagem(IVC* src, IVC* dst, IVC* farmacia);
int preencheImagem(IVC* src, IVC* dst, IVC* imagem);
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
int vc_gray_histogram_show(IVC* src);