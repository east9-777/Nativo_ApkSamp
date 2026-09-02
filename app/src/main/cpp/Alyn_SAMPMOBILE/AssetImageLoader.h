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
 *
 * IMPORTANTE: o renderer deste client NAO desenha via OpenGL puro - ele usa
 * o RenderWare do proprio GTA SA (ver ImGuiWrapper::renderDrawData, que
 * chama RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pcmd->TextureId)).
 * Isso significa que TextureId precisa ser um RwRaster* de verdade (o mesmo
 * tipo que ImGuiWrapper::createFontTexture cria pra fonte), NUNCA um GLuint
 * cru de glGenTextures - RW tentaria ler aquele numero pequeno como se
 * fosse um ponteiro pra struct RwRaster, o que nao desenha nada (na melhor
 * hipotese) ou corrompe memoria (na pior). Por isso este loader monta um
 * RwImage a partir do PNG decodificado e converte pra RwRaster com
 * RwRasterSetFromImage, exatamente como a fonte faz.
 */

// Chamado uma vez, do lado Java (ver SAMP.java), passando o AssetManager
// do Android. Sem isso, LoadIconTextureFromAsset sempre falha.
void AssetImageLoader_SetAssetManager(AAssetManager* mgr);

// Caminho relativo a app/src/main/assets/, ex: "hud/hud_vida.png".
// Retorna um RwRaster* (compativel com ImTextureID) pronto pra ser usado no
// drawImage()/AddImage() do ImGuiRenderer, ou nullptr se falhar ao carregar.
// Resultado fica em cache (mesmo caminho = mesmo raster, nao recarrega nem
// duplica na GPU a cada frame).
void* LoadIconTextureFromAsset(const char* assetRelativePath);
