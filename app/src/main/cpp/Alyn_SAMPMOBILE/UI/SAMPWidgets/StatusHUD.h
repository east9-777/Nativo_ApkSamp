#pragma once

#include "../../Game/Game.h"
#include "../../AssetImageLoader.h"

/**
 * StatusHUD: 4 barras hexagonais no estilo FiveM/MTA (vida, colete, fome,
 * sede). Cada uma e' um hexagono de fundo solido + borda colorida que
 * preenche o perimetro proporcional ao valor (0.0 a 1.0) + um icone
 * centralizado.
 *
 * Vida e Colete sao atualizados sozinhos, lendo direto do jogo via
 * GamePool_FindPlayerPed() - nao precisa de nada do servidor.
 *
 * Fome e Sede sao atualizados via RPC customizado (ID 220, ver
 * ScrNativoStatusUpdate em Net/ScriptRPC.cpp), mandado pela gamemode
 * atraves do plugin Pawn.RakNet.
 *
 * Icones vem de dentro do proprio APK (app/src/main/assets/hud/*.png) -
 * nao da texdb, nao de download. Ver AssetImageLoader.h/.cpp.
 *
 * Visibilidade: por padrao o HUD comeca ESCONDIDO quando o player entra no
 * servidor (m_gmVisible = false). Ele so aparece depois que a gamemode
 * manda mostrar, atraves do RPC customizado ID 221 (ver
 * ScrNativoStatusVisibility em Net/ScriptRPC.cpp / MostrarNecessidades()
 * no lado Pawn), chamado via setGMVisible(true). Mesmo com o HUD liberado
 * pela gamemode, ele so desenha vida/colete de fato quando o ped local
 * existe (ver update()) - as duas condicoes precisam ser verdadeiras.
 */
class StatusHUD : public Widget {
public:
	StatusHUD();

	virtual void draw(ImGuiRenderer* renderer) override;

	// chamado todo frame (Idle hook) pra vida/colete
	void update();

	// chamado quando o RPC customizado de fome/sede chega (ScriptRPC.cpp)
	void setHunger(float percent);
	void setThirst(float percent);

	// chamado quando o RPC customizado de visibilidade chega (ScriptRPC.cpp)
	// - controla se o HUD PODE aparecer, por decisao do servidor/GM.
	void setGMVisible(bool visible);
	bool gmVisible() const { return m_gmVisible; }

private:
	struct HexBar {
		float value = 1.0f;      // 0.0 a 1.0
		ImColor color;
		std::string iconPath;    // caminho relativo a app/src/main/assets/
		// RwRaster* (ver AssetImageLoader.h) carregado sob demanda no primeiro
		// draw(). Precisa ser um ponteiro de verdade (void*), nao um inteiro de
		// 32 bits - em build arm64-v8a um GLuint/unsigned int truncaria a parte
		// alta do ponteiro real e corromperia o raster.
		void* iconTexture = nullptr;
	};

	void drawBar(ImGuiRenderer* renderer, HexBar& bar, const ImVec2& center, float radius);

private:
	HexBar m_health;
	HexBar m_armour;
	HexBar m_hunger;
	HexBar m_thirst;

	// true = servidor permite exibir o HUD. false = escondido (default: so
	// aparece quando a gamemode manda mostrar via setGMVisible/RPC 221).
	bool m_gmVisible = false;
};
