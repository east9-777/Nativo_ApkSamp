# Diagnóstico: por que textureId=0

## O que o log já provou
A imagem carrega e decodifica certinho (128x128) — o problema é
especificamente na criação da textura OpenGL (`glGenTextures` retornando 0,
inválido).

## Suspeita
Isso pode explicar **todas** as tentativas anteriores que também ficaram
vazias (texdb, PNG do disco) — se for um problema de timing (textura sendo
criada antes do contexto OpenGL estar pronto naquela thread), o mesmo bug
afetaria qualquer forma de carregar textura nova nesse ponto do código.

## O que adicionei
Duas linhas de diagnóstico logo depois do `glGenTextures`:
- `eglGetCurrentContext()` → mostra se existe contexto OpenGL válido
  naquele momento (se vier `0x0`, confirma que é isso)
- `glGetError()` → mostra o código de erro exato do OpenGL

## Onde aplicar
- `AssetImageLoader.cpp` → substitui o existente

## Teste
Aplique, compile, entre no servidor, e me manda o log de novo. Procure por
linhas tipo:
```
LoadIconTextureFromAsset: contexto EGL atual = 0x...
LoadIconTextureFromAsset: apos glGenTextures - textureId=0, glGetError()=0x...
```
Com esses dois números eu vou saber com certeza se é problema de contexto
(e já te mando o fix certo) ou se é outra coisa completamente diferente.
