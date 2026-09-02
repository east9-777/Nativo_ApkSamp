# Onde cada arquivo entra na source do APK (Nativo_ApkSamp.zip)

Caminhos relativos à RAIZ do projeto (a pasta onde ficam `app/`, `gradle/`,
`build.gradle`, etc — a raiz do zip que vocês me mandaram).

## Novos arquivos (criar)

| Arquivo entregue | Onde criar dentro do projeto |
|---|---|
| `SpeedometerHUD.h` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/SAMPWidgets/SpeedometerHUD.h` |
| `SpeedometerHUD.cpp` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/SAMPWidgets/SpeedometerHUD.cpp` |

É a mesma pasta onde já está o `StatusHUD.h`/`StatusHUD.cpp` (o HUD de
hexágonos) — os dois arquivos novos ficam lado a lado com eles.

## Arquivos modificados (sobrescrever os que já existem)

| Arquivo entregue | Sobrescreve |
|---|---|
| `ImGuiRenderer.h` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/ImGuiRenderer.h` |
| `ImGuiRenderer.cpp` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/ImGuiRenderer.cpp` |
| `UI.h` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/UI.h` |
| `UI.cpp` | `app/src/main/cpp/Alyn_SAMPMOBILE/UI/UI.cpp` |
| `Hooks.cpp` | `app/src/main/cpp/Alyn_SAMPMOBILE/Game/Hooks.cpp` |
| `ScriptRPC.cpp` | `app/src/main/cpp/Alyn_SAMPMOBILE/Net/ScriptRPC.cpp` |

## Lado da gamemode (fora do APK)

| Arquivo entregue | Onde colocar |
|---|---|
| `Combustivel.pwn` | Na mesma pasta onde está `Necessidade.pwn`, no projeto Pawn da gamemode (fora do repositório do APK). Depois, adicionar `#include "Combustivel.pwn"` no arquivo principal da gamemode, do jeito que `Necessidade.pwn` já deve estar incluído. |

## Árvore resumida (só o que muda)

```
<raiz do projeto do APK>/
└── app/src/main/cpp/Alyn_SAMPMOBILE/
    ├── Game/
    │   └── Hooks.cpp                      ← MODIFICADO
    ├── Net/
    │   └── ScriptRPC.cpp                  ← MODIFICADO
    └── UI/
        ├── ImGuiRenderer.h                ← MODIFICADO
        ├── ImGuiRenderer.cpp              ← MODIFICADO
        ├── UI.h                           ← MODIFICADO
        ├── UI.cpp                         ← MODIFICADO
        └── SAMPWidgets/
            ├── StatusHUD.h                (não mexe - só referência)
            ├── StatusHUD.cpp              (não mexe - só referência)
            ├── SpeedometerHUD.h           ← NOVO
            └── SpeedometerHUD.cpp         ← NOVO

<projeto da gamemode>/
└── Combustivel.pwn                        ← NOVO (mesma pasta de Necessidade.pwn)
```
