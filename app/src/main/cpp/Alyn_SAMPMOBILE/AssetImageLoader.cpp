#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <android/asset_manager.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <RenderWare/rw.h>

#include "AssetImageLoader.h"

static AAssetManager* s_assetManager = nullptr;
static std::unordered_map<std::string, void*> s_iconCache; // caminho -> RwRaster*

void AssetImageLoader_SetAssetManager(AAssetManager* mgr)
{
	s_assetManager = mgr;
	spdlog::info("AssetImageLoader: AssetManager registrado ({})", (void*) mgr);
}

void* LoadIconTextureFromAsset(const char* assetRelativePath)
{
	if (assetRelativePath == nullptr) {
		return nullptr;
	}

	std::string path(assetRelativePath);

	auto it = s_iconCache.find(path);
	if (it != s_iconCache.end()) {
		return it->second;
	}

	if (s_assetManager == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: AssetManager ainda nao foi registrado (path=\"{}\")", path);
		return nullptr; // nao guarda no cache - tenta de novo no proximo draw()
	}

	AAsset* asset = AAssetManager_open(s_assetManager, path.c_str(), AASSET_MODE_BUFFER);
	if (asset == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: nao encontrei o asset \"{}\" (confira se o arquivo esta em app/src/main/assets/{})", path, path);
		return nullptr; // nao guarda no cache - tenta de novo no proximo draw()
	}

	off_t length = AAsset_getLength(asset);
	const void* buffer = AAsset_getBuffer(asset);

	if (buffer == nullptr || length <= 0) {
		spdlog::warn("LoadIconTextureFromAsset: asset \"{}\" veio vazio", path);
		AAsset_close(asset);
		return nullptr; // nao guarda no cache - tenta de novo no proximo draw()
	}

	// Decodifica os bytes do PNG direto da memoria (nao precisa de arquivo
	// temporario em disco). Isso continua igual - o problema nunca foi a
	// decodificacao do PNG.
	std::vector<uchar> rawBytes((uchar*) buffer, (uchar*) buffer + length);
	AAsset_close(asset);

	cv::Mat image = cv::imdecode(rawBytes, cv::IMREAD_UNCHANGED);

	if (image.empty()) {
		spdlog::warn("LoadIconTextureFromAsset: falha ao decodificar PNG \"{}\"", path);
		return nullptr; // nao guarda no cache - tenta de novo no proximo draw()
	}

	// RwImage/RwRaster (abaixo) esperam RGBA na mesma ordem que o atlas de
	// fonte do ImGui usa (GetTexDataAsRGBA32) - cv::imdecode devolve
	// BGR(A), entao convertemos aqui antes de montar o RwImage.
	if (image.channels() == 4) {
		cv::cvtColor(image, image, cv::COLOR_BGRA2RGBA);
	} else if (image.channels() == 3) {
		cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);
	} else {
		spdlog::warn("LoadIconTextureFromAsset: formato inesperado em \"{}\" ({} canais)", path, image.channels());
		return nullptr; // nao guarda no cache - tenta de novo no proximo draw()
	}

	// A partir daqui, mesma receita que ImGuiWrapper::createFontTexture usa
	// pra fonte: RwImage (buffer device-independent) -> RwRaster (textura
	// de verdade do RenderWare). O client so sabe desenhar RwRaster*, nunca
	// GLuint - ver o comentario grande no .h.
	RwImage* iconImage = RwImageCreate(image.cols, image.rows, 32);
	if (iconImage == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: RwImageCreate falhou para \"{}\"", path);
		return nullptr;
	}

	RwImageAllocatePixels(iconImage);

	RwUInt8* dst = iconImage->cpPixels;
	const uchar* src = image.data;
	int rowBytes = image.cols * 4; // RGBA, 4 bytes por pixel
	for (int y = 0; y < iconImage->height; ++y) {
		memcpy((void*) dst, src + (size_t) image.step * y, rowBytes);
		dst += iconImage->stride;
	}

	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(iconImage, rwRASTERTYPETEXTURE, &w, &h, &d, &flags);

	RwRaster* raster = RwRasterCreate(w, h, d, flags);
	if (raster == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: RwRasterCreate falhou para \"{}\"", path);
		RwImageDestroy(iconImage);
		return nullptr;
	}

	raster = RwRasterSetFromImage(raster, iconImage);
	RwImageDestroy(iconImage);

	if (raster == nullptr) {
		spdlog::warn("LoadIconTextureFromAsset: RwRasterSetFromImage falhou para \"{}\"", path);
		return nullptr;
	}

	spdlog::info("LoadIconTextureFromAsset: \"{}\" carregado ({}x{}) como RwRaster {}", path, image.cols, image.rows, (void*) raster);

	s_iconCache[path] = (void*) raster;
	return (void*) raster;
}
