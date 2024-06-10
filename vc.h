//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
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
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

//FUNCOES PRODUZIDAS EM SALA
int vc_rgb_to_binary(IVC* srcdst);
int vc_rgb_to_hsv(IVC* srcdst);
int vc_hsv_segmentation(IVC* src, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_scale_gray_to_rgb(IVC* src, IVC* dst);
int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_gray_to_binary(IVC* srcdst, int threshold);
int vc_gray_to_binary_media(IVC* srcdst);
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin);
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int kernel1, int kernel2);
int vc_binary_close(IVC* src, IVC* dst, int kernel);
int vc_gray_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_to_gray(IVC* src, IVC* dst);
int vc_gray_histogram_show(IVC* src, IVC* dst);
int vc_gray_histogram_equalization(IVC* src, IVC* dst);
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernel);
int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernel);
int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);
int vc_3chanels_to_1(IVC* src, IVC* dst);
int vc_3chanels_to_1_binary(IVC* src, IVC* dst);




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

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
int vc_draw_boundingbox(IVC* src, OVC* blob);
int vc_draw_center_of_mass(IVC* src, OVC* blobs, int nblobs, int tamanho_alvo, int cor);
int vc_normalizar_imagem_labelling(IVC* src, IVC* dst, int nblobs);