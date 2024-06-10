#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <fstream>
#include <vector>
#include <filesystem>
#include <map>

extern "C"
{
#include "vc.h"
}

void vc_timer(void)
{
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running)
	{
		running = true;
	}
	else
	{
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

		// Tempo em segundos.
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
		double nseconds = time_span.count();

		std::cout << "Tempo decorrido: " << nseconds << "segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

// Tabela de cores de resistores
std::map<std::string, int> colorValueMap = {
	{"Preto", 0},
	{"Castanho", 1},
	{"Vermelho", 2},
	{"Laranja", 3},
	{"Amarelo", 4},
	{"Verde", 5},
	{"Azul", 6},
	{"Violeta", 7},
	{"Cinzento", 8},
	{"Branco", 9}
};

// Tabela de multiplicadores de resistores
std::map<std::string, float> multiplierMap = {
	{"Preto", 1},
	{"Castanho", 10},
	{"Vermelho", 100},
	{"Laranja", 1000},
	{"Amarelo", 10000},
	{"Verde", 100000},
	{"Azul", 1000000},
	{"Dourado", 0.1},
	{"Cinzento", 0.01}
};

// Tabela de tolerâncias de resistores
std::map<std::string, float> toleranceMap = {
	{"Castanho", 1.0f},
	{"Vermelho", 2.0f},
	{"Dourado", 5.0f},
	{"Cinzento", 10.0f}
};

// Tabela de valores das cores de cada faixa
std::map<std::string, std::pair<int, int>> HueColorMap = {
	{"Castanho", {10, 25}},    // Castanho pode variar de 10 a 20
	{"Vermelho", {0, 10}},     // Vermelho varia de 0 a 10
	{"Laranja", {25, 35}},     // Laranja varia de 20 a 30
	{"Verde", {35, 85}},       // Verde varia de 45 a 85
	{"Azul", {75, 135}},       // Azul varia de 75 a 135
	{"Violeta", {135, 150}},   // Violeta varia de 135 a 150
	{"Dourado", {25, 35}}      // Dourado varia de 20 a 30
};

// Identifica a cor com base no valor HSV
// Hue é 0 a 255
std::string identificarCorHSV(int hue, int saturation, int value) {
	if (saturation < 20) { // Considera cores achromáticas
		if (value < 50) return "Preto";
		else if (value < 200) return "Cinzento";
		else return "Branco";
	} else {
		if (hue >= 20 && hue <= 30) {
			// Distingue entre Dourado e Laranja baseado na saturação e valor
			if (saturation < 100 && value < 200) return "Dourado";
			else return "Laranja";
		} else {
			for (const auto &color : HueColorMap) {
				if (hue >= color.second.first && hue <= color.second.second) {
					return color.first;
				}
			}
		}
	}
	// Se nenhuma cor for identificada
	return "Desconhecido";
}

int main(void)
{
	// Vídeo
	// Meter o video no mesmo diretório que os ficheiros de cóodigo
	char videofile[120] = "video_resistors.mp4";
	cv::VideoCapture capture; // Objeto para captura de vídeo
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;

	/* Leitura de vídeo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
	*/
	capture.open(videofile);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_GUI_NORMAL);

	/* Inicia o timer */
	vc_timer();

	// Declara a variável para armazenar a frame
	cv::Mat frame;

	// Cria novas imagens IVC
	IVC* image = vc_image_new(video.width, video.height, 3, 255);
	IVC* image2 = vc_image_new(video.width, video.height, 1, 255);
	IVC* imagemDilatada = vc_image_new(video.width, video.height, 1, 255);
	IVC* imagemEtiquetada = vc_image_new(video.width, video.height, 1, 255);
	IVC* imagemHSV = vc_image_new(video.width, video.height, 3, 255);

	int resistorsCounter = 0;
	int* resistencia = nullptr;

	while (key != 'q')
	{
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty())
			break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inserção texto na frame */
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


		std::string str = "Valor do resistor " + std::to_string(resistorsCounter) + ": " + std::to_string(resistencia == nullptr ? 0 : *resistencia) + " ohms";
		cv::putText(frame, str, cv::Point(20, 900), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 900), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);

		// Converte imagem RGB para HSV
		vc_rgb_to_hsv(image);

		// Copia imagem HSV
		memcpy(imagemHSV->data, image->data, video.width * video.height * 3);

		// Segmentação da imagem HSV
		vc_hsv_segmentation(image, 0, 200, 40, 60, 40, 75);


		// Tranforma imagem HSV em imagem binária
		vc_3channels_to_1(image, image2);

		// Faz a dilatação da imagem binária
		// Converte IVC para cv::Mat
		cv::Mat matImagemBinaria(video.height, video.width, CV_8UC1, image2->data);
		cv::Mat matDilatada;

		// Cria kernel
		int kernelSize = 48;

		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));

		cv::dilate(matImagemBinaria, matDilatada, kernel);

		// Converte de volta para IVC
		memcpy(imagemDilatada->data, matDilatada.data, video.width * video.height);

		// Etiquetagem dos blobs
		int nblobs;
		OVC* blobs = vc_binary_blob_labelling(imagemDilatada, imagemEtiquetada, &nblobs);

		// Extração de informação dos blobs
		vc_binary_blob_info(imagemEtiquetada, blobs, nblobs);

		// Converte IVC para cv::Mat
		cv::Mat hsvImage(image->height, image->width, CV_8UC3, imagemHSV->data);

		//Bounding box e identificação de resistências
		if (blobs != nullptr)
		{
			int altura = video.width / 2;
			const int tolerance = 3;

			// Verifica se o blob é uma resistência
			if (blobs->area > 15000 && blobs->area < 28000 && blobs->perimeter > 500 && blobs->perimeter < 700 && blobs->height < 130 && blobs->height > 85)
			{
				// Desenha as bounding boxes e cruzes no centro de massa
				vc_draw_boundingbox(imagemHSV, blobs);
				vc_draw_center_of_mass(imagemHSV, blobs, nblobs, 10, 255);

				// Quando o centro de massa passa pelo centro da tela, conta um blob como resistência
				if (abs(blobs->yc - altura) <= tolerance)
				{
					resistorsCounter++;

					// Identifica todas as cores na linha do centro de massa dentro do blob
					// Acessa a linha do centro de massa
					cv::Mat linha = hsvImage.row(blobs->yc);

					// Delimita a área do blob na linha
					int startX = std::max(0, blobs->xc - blobs->width / 2);
					int endX = std::min(linha.cols - 1, blobs->xc + blobs->width / 2);
					cv::Mat linhaBlob = linha(cv::Range::all(), cv::Range(startX, endX));

					// Separa os canais HSV
					std::vector<cv::Mat> hsvChannels;
					cv::split(linhaBlob, hsvChannels);

					int segmentSize = linhaBlob.cols / 4; // Tamanho do segmento
					std::vector<std::string> coresResistor;

					// Itera sobre os segmentos da linha
					for (int i = 0; i < linhaBlob.cols; i += segmentSize)
					{
						// Recorta o segmento
						cv::Rect roi(i, 0, std::min(segmentSize, linhaBlob.cols - i), 1);
						cv::Mat segment = linhaBlob(roi);

						// Separa os canais HSV dentro do segmento
						std::vector<cv::Mat> segmentChannels;
						cv::split(segment, segmentChannels);

						// Calcula o histograma do canal H (Hue)
						int histSize = 180;
						float range[] = { 0, 180 };
						const float* histRange = { range };
						cv::Mat hist;
						cv::calcHist(&segmentChannels[0], 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

						// Encontra o bin com o maior valor no histograma
						cv::Point maxLoc;
						cv::minMaxLoc(hist, 0, 0, 0, &maxLoc);
						int hue = maxLoc.y;

						// Calcula a média de saturação e valor
						int saturation = cv::mean(segmentChannels[1])[0];
						int value = cv::mean(segmentChannels[2])[0];

						// Identifica a cor dominante
						std::string corDominante = identificarCorHSV(hue, saturation, value);
						// Adiciona a cor à lista
						coresResistor.push_back(corDominante);
					}

					// Exibe as cores na ordem
					for (const auto& cor : coresResistor) {
						std::cout << "Cor: " << cor << std::endl;
					}

					// Verifica se 4 cores foram detectadas
					if (coresResistor.size() >= 4) {
						// Calcula o valor da resistência
						int digit1 = colorValueMap[coresResistor[0]];
						int digit2 = colorValueMap[coresResistor[1]];
						int multiplier = multiplierMap[coresResistor[2]];
						int tolerancia = toleranceMap[coresResistor[3]];

						// Calcula o novo valor da resistência
						int novoValorResistencia = (digit1 * 10 + digit2) * multiplier;

						// Se o ponteiro para a resistência ainda for nulo, atribua o novo valor da resistência a ele
						if (resistencia == nullptr) {
							resistencia = new int(novoValorResistencia);
						}
						else {
							// Caso contrário, atualize o valor da resistência
							*resistencia = novoValorResistencia;
						}
						std::cout << "Valor da resistência: " << *resistencia << " ohms" << std::endl;
						std::cout << "Tolerância: ±" << tolerancia << "%" << std::endl;
					}
					else if (coresResistor.size() == 3) {
						// Calcula o valor da resistência
						int digit1 = colorValueMap[coresResistor[0]];
						int digit2 = colorValueMap[coresResistor[1]];
						int multiplier = multiplierMap[coresResistor[2]];
						int tolerancia = 5.0; //Dourado

						// Calcula o novo valor da resistência
						int novoValorResistencia = (digit1 * 10 + digit2) * multiplier;

						// Se o ponteiro para a resistência ainda for nulo, atribua o novo valor da resistência a ele
						if (resistencia == nullptr) {
							resistencia = new int(novoValorResistencia);
						}
						else {
							// Caso contrário, atualize o valor da resistência
							*resistencia = novoValorResistencia;
						}
						std::cout << "Valor da resistência: " << resistencia << " ohms" << std::endl;
						std::cout << "Tolerância: ±" << tolerancia << "%" << std::endl;
					}
					else {
						std::cout << "Erro: Não foram detectadas 4 cores." << std::endl;
					}
				}
			}
		}

		//  Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		memcpy(frame.data, imagemHSV->data, video.width * video.height * 3);

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);

		// Interrompe a execução após o frame 780
		if (video.nframe == 780)
		{
			break;
		}

	}

	std::cout << "Numero de resistencias: " << resistorsCounter << std::endl;

	/* Para o timer e exibe o tempo decorrido */
	vc_timer();

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de vídeo */
	capture.release();

	// Liberta a memória da imagem IVC que havia sido criada
	vc_image_free(image);
	vc_image_free(image2);
	vc_image_free(imagemDilatada);
	vc_image_free(imagemEtiquetada);
	vc_image_free(imagemHSV);


	return 0;
}