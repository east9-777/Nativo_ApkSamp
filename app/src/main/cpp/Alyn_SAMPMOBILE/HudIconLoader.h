#pragma once

/**
 * HudIconLoader: carrega icones PNG soltos (fora da texdb do jogo) e sobe
 * como textura OpenGL, pra usar no HUD novo (vida/colete/fome/sede).
 *
 * Diferente de LoadTextureFromTxd (que le da texdb binaria do jogo), essa
 * funcao le um arquivo .png comum direto da pasta de arquivos do app,
 * usando opencv (ja linkado no projeto, so nunca usado ate agora).
 *
 * IMPORTANTE: so pode ser chamada de dentro do render thread (ou seja, de
 * dentro de um Widget::draw(), nunca do zero em outro lugar), porque cria
 * uma textura OpenGL de verdade (glGenTextures), que exige um contexto GL
 * valido na thread atual.
 */

// Retorna o ID da textura OpenGL (GLuint), ou 0 se falhar ao carregar.
// O resultado fica em cache internamente (mesmo caminho = mesma textura,
// nao recarrega/nao duplica na GPU a cada frame).
unsigned int LoadIconTextureFromPNG(const char* filePath);
