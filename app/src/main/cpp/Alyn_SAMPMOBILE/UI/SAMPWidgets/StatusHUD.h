#pragma once

#include "../../Game/Game.h"
#include "../../Game/Util.h"

/**
 * StatusHUD: 4 barras hexagonais no estilo FiveM/MTA (vida, colete, fome,
 * sede). Cada uma e' um hexagono de fundo solido + borda colorida que
 * preenche o perimetro proporcional ao valor (0.0 a 1.0) + um icone
 * centralizado.
 *
 * Vida e Colete sao atualizados sozinhos, lendo direto do jogo via
 * GamePool_FindPlayerPed() (nao aloca CPlayerPed cedo demais, seguro
 * durante o boot). Nao depende do servidor.
 *
 * Fome e Sede sao atualizados via RPC customizado (ID 220, ver
 * ScrNativoStatusUpdate em Net/ScriptRPC.cpp), mandado pela gamemode
 * atraves do plugin Pawn.RakNet.
 *
 * Icones vem da TXD "txd" (mesma onde o icone "fist" - punho desarmado -
 * ja vive), texturas: hud_vida, hud_colete, hud_fome, hud_sede.
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

private:
	struct HexBar {
		float value = 1.0f;      // 0.0 a 1.0
		ImColor color;
		std::string iconTexdbName; // nome da textura dentro da TXD "txd"
		RwTexture* iconTexture = nullptr; // carregado sob demanda no primeiro draw()
	};

	void drawBar(ImGuiRenderer* renderer, HexBar& bar, const ImVec2& center, float radius);

private:
	HexBar m_health;
	HexBar m_armour;
	HexBar m_hunger;
	HexBar m_thirst;
};
