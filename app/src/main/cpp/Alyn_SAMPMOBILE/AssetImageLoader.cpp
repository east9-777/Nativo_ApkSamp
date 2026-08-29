#include <string>
#include <vector>
#include <unordered_map>
#include <GLES2/gl2.h>
#include <android/asset_manager.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

#include "AssetImageLoader.h"

static AAssetManager* s_assetManager = nullptr;
static std::unordered_map<std::string, unsigned int> s_iconCache;

void AssetImageLoader_SetAssetManager(AAssetManager* mgr)
{
	s_assetManager = mgr;
	spdlog::info("AssetImageLoader: AssetManager registrado ({})", (void*) mgr);
}

unsigned int LoadIconTextureFromAsset(const char* assetRelativePath)
{
	if (assetRelativePath == nullptr) {
		return 0;
	}

	std::string path(assetRelativePath);

	auto it = s_iconCache.find(path);
	if (it != s_iconCache.end()) {
		return it->second;
	}

	if (s_assetManager == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: AssetManager ainda nao foi registrado (path=\"{}\")", path);
		s_iconCache[path] = 0;
		return 0;
	}

	AAsset* asset = AAssetManager_open(s_assetManager, path.c_str(), AASSET_MODE_BUFFER);
	if (asset == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: nao encontrei o asset \"{}\" (confira se o arquivo esta em app/src/main/assets/{})", path, path);
		s_iconCache[path] = 0;
		return 0;
	}

	off_t length = AAsset_getLength(asset);
	const void* buffer = AAsset_getBuffer(asset);

	if (buffer == nullptr || length <= 0) {
		spdlog::warn("LoadIconTextureFromAsset: asset \"{}\" veio vazio", path);
		AAsset_close(asset);
		s_iconCache[path] = 0;
		return 0;
	}

	// Decodifica os bytes do PNG direto da memoria (nao precisa de arquivo
	// temporario em disco).
	std::vector<uchar> rawBytes((uchar*) buffer, (uchar*) buffer + length);
	AAsset_close(asset);

	cv::Mat image = cv::imdecode(rawBytes, cv::IMREAD_UNCHANGED);

	if (image.empty()) {
		spdlog::warn("LoadIconTextureFromAsset: falha ao decodificar PNG \"{}\"", path);
		s_iconCache[path] = 0;
		return 0;
	}

	if (image.channels() == 4) {
		cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
	} else if (image.channels() == 3) {
		cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);
	} else {
		spdlog::warn("LoadIconTextureFromAsset: formato inesperado em \"{}\" ({} canais)", path, image.channels());
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

	spdlog::info("LoadIconTextureFromAsset: \"{}\" carregado ({}x{}), textureId={}", path, image.cols, image.rows, textureId);

	s_iconCache[path] = textureId;
	return textureId;
}
