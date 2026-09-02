#pragma once

#include "../../Game/Game.h"

/**
 * SpeedometerHUD: velocimetro circular estilo "dashboard" (arco amarelo de
 * velocidade no meio, numero grande no centro, arco branco de vida do
 * veiculo na esquerda, arco verde de combustivel na direita).
 *
 * Velocidade e vida do veiculo sao lidas sozinhas, direto do jogo via
 * GamePool_FindPlayerPed()->pVehicle - igual o StatusHUD faz com vida do
 * player. Nao precisa de nada do servidor pra essas duas.
 *
 * Combustivel NAO existe nativamente no GTA SA/SA-MP (nao ha campo de
 * "gasolina" no veiculo). Por isso ele so muda via RPC customizado (ID 222,
 * ver ScrNativoVehicleFuel em Net/ScriptRPC.cpp), mandado pela gamemode
 * atraves do plugin Pawn.RakNet - mesma familia dos RPCs 220/221 do
 * StatusHUD (fome/sede). Ver Combustivel.pwn no lado do servidor.
 *
 * O HUD so aparece quando o ped local existe E esta dentro de um veiculo.
 * Igual o StatusHUD, tambem aceita ser escondido a forca pelo GM
 * (setGMVisible) se um dia vocês quiserem um /vhud como o /fhud.
 */
class SpeedometerHUD : public Widget {
public:
	SpeedometerHUD();

	virtual void draw(ImGuiRenderer* renderer) override;

	// chamado todo frame (Idle hook) - le velocidade e vida do veiculo atual
	void update();

	// chamado quando o RPC customizado 222 de combustivel chega
	// (Net/ScriptRPC.cpp) - unico valor que NAO vem do jogo nativo.
	void setFuel(float percent);

	// igual StatusHUD::setGMVisible - permite o GM forcar o HUD a
	// aparecer/sumir independente do estado automatico (dentro/fora de
	// veiculo). Opcional: so chame isso se implementarem um RPC de
	// visibilidade pro velocimetro (nao incluso aqui).
	void setGMVisible(bool visible);
	bool gmVisible() const { return m_gmVisible; }

private:
	void drawTrack(ImGuiRenderer* renderer, const ImVec2& center, float radius, float thickness, float angleMinDeg, float angleMaxDeg, const ImColor& trackColor);

private:
	// velocidade atual em km/h, calculada a partir do vetor de velocidade
	// do veiculo (ver update()). Nao vem do servidor.
	float m_speedKmh = 0.0f;

	// 0.0 a 1.0 - vida do veiculo, lida direto de sa::CVehicle::fHealth
	float m_health = 1.0f;

	// 0.0 a 1.0 - combustivel, so muda via setFuel() (RPC do servidor)
	float m_fuel = 1.0f;

	bool m_inVehicle = false;

	// true = pode aparecer (default). false = GM escondeu na unha.
	bool m_gmVisible = true;

	// velocidade (km/h) que enche 100% do arco amarelo. Ajustem ao gosto -
	// 240 cobre a maioria dos carros comuns do SA sem o ponteiro "bater no
	// fim" toda hora.
	static constexpr float kMaxSpeedKmh = 240.0f;

	// escala nativa de vida de veiculo do GTA SA (0-1000 pra carro comum,
	// full health default = 1000.0f). Alguns veiculos especiais (ex: tanque)
	// tem vida maxima diferente - se notarem a barra de vida sempre cheia ou
	// sempre baixa num veiculo especifico, e' esse valor que precisa ajustar.
	static constexpr float kMaxVehicleHealth = 1000.0f;
};
