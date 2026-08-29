#pragma once

#include <android/asset_manager.h>

/**
 * AssetImageLoader: carrega icones PNG que ficam DENTRO do APK
 * (app/src/main/assets/), nao na pasta de dados baixada.
 *
 * Diferente de tudo que tentamos antes (texdb, opencv-do-disco), isso nao
 * depende de NENHUM download nem da texdb do jogo - a imagem vem embutida
 * no proprio instalador do app, sempre presente, sem risco de crash de
 * banco de dados de textura.
 *
 * Precisa que o AssetManager tenha sido registrado uma vez (ver
 * AssetImageLoader_SetAssetManager, chamado do Java assim que o app abre).
 */

// Chamado uma vez, do lado Java (ver SAMP.java), passando o AssetManager
// do Android. Sem isso, LoadIconTextureFromAsset sempre falha.
void AssetImageLoader_SetAssetManager(AAssetManager* mgr);

// Caminho relativo a app/src/main/assets/, ex: "hud/hud_vida.png".
// Retorna o ID da textura OpenGL (GLuint), ou 0 se falhar ao carregar.
// Resultado fica em cache (mesmo caminho = mesma textura, nao recarrega
// nem duplica na GPU a cada frame).
unsigned int LoadIconTextureFromAsset(const char* assetRelativePath);
