#include <string>
#include <unordered_map>
#include <GLES2/gl2.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

#include "HudIconLoader.h"

// Cache: caminho do arquivo -> ID da textura ja carregada na GPU.
// Evita recarregar o mesmo PNG do disco/GPU toda vez que o widget desenha.
static std::unordered_map<std::string, unsigned int> s_iconCache;

unsigned int LoadIconTextureFromPNG(const char* filePath)
{
	if (filePath == nullptr) {
		return 0;
	}

	std::string path(filePath);

	auto it = s_iconCache.find(path);
	if (it != s_iconCache.end()) {
		return it->second;
	}

	// IMREAD_UNCHANGED preserva o canal alpha (transparencia) do PNG.
	cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

	if (image.empty()) {
		spdlog::warn("LoadIconTextureFromPNG: nao consegui abrir \"{}\"", path);
		s_iconCache[path] = 0;
		return 0;
	}

	// opencv sempre carrega em ordem de canais BGR(A), o OpenGL espera RGB(A).
	if (image.channels() == 4) {
		cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
	} else if (image.channels() == 3) {
		cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);
	} else {
		spdlog::warn("LoadIconTextureFromPNG: formato de imagem inesperado em \"{}\" ({} canais)", path, image.channels());
		s_iconCache[path] = 0;
		return 0;
	}

	GLuint textureId = 0;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

	glBindTexture(GL_TEXTURE_2D, 0);

	spdlog::info("LoadIconTextureFromPNG: \"{}\" carregado ({}x{}), textureId={}", path, image.cols, image.rows, textureId);

	s_iconCache[path] = textureId;
	return textureId;
}
