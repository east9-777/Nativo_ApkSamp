# Correção: rotação não funcionava

## Causa
`android:screenOrientation="landscape"` trava numa única posição fixa e
**ignora completamente o sensor/giroscópio** — por isso ativar a rotação
automática do Android não tinha efeito nenhum.

## Correção
Troquei para `sensorLandscape` em todas as 5 activities (as 4 do launcher +
o `SAMP`, que é o jogo). Com isso:
- Continua **nunca indo pra retrato** (mantém o que você pediu antes)
- Mas agora **usa o sensor** pra alternar entre landscape normal e landscape
  invertido (virar 180°), como a maioria dos apps/jogos faz

## Onde aplicar
- `manifest/AndroidManifest.xml` → substitui `app/src/main/AndroidManifest.xml`

Esse manifest já é cumulativo — inclui tudo das fases anteriores
(`ConnectingActivity` registrada, todas as activities do launcher) mais essa
correção. Não precisa mesclar nada, é só substituir o arquivo inteiro.

## Teste
Abra o app, ative a rotação automática do Android, e gire o celular 180°
(de cabeça pra baixo) — a tela deve acompanhar. Girar pra retrato continua
não fazendo nada, como esperado.
