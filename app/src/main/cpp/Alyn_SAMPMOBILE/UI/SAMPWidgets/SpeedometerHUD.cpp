#include <cmath>
#include "../UI.h"
#include "../../Client.h"
#include "../../Game/Game.h"
#include "SpeedometerHUD.h"

extern Game* pGame;

SpeedometerHUD::SpeedometerHUD()
{
}

void SpeedometerHUD::update()
{
	// Mesma logica de guarda do StatusHUD::update() - FindPlayerPed() e' uma
	// factory preguicosa, chamar cedo demais pode criar o CPlayerPed antes
	// da hora. Aqui so lemos o ped/veiculo ja existentes.
	if (!pGame) {
		m_inVehicle = false;
		setVisible(false);
		return;
	}

	sa::CPed* pPed = GamePool_FindPlayerPed();
	if (!pPed || !pPed->IsInVehicle()) {
		m_inVehicle = false;
		setVisible(false);
		return;
	}

	m_inVehicle = true;

	// So aparece se estiver num veiculo E o GM nao escondeu o HUD.
	setVisible(m_gmVisible);
	if (!m_gmVisible) {
		return;
	}

	sa::CVehicle* pVeh = pPed->pVehicle;
	if (!pVeh) {
		setVisible(false);
		return;
	}

	// --- velocidade ---
	// Modulo do vetor de velocidade (unidades de jogo por frame) convertido
	// pra km/h. 180.0f e' a constante empirica usada por varios mods
	// GTA:SA/SA-MP pra essa conversao - calibrem contra um carro de
	// velocidade conhecida (ex: velocimetro nativo de algum outro client)
	// se acharem o numero muito longe da realidade.
	sa::CVector vel = pVeh->GetMoveSpeed();
	float speedUnits = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
	m_speedKmh = speedUnits * 180.0f;
	if (m_speedKmh < 0.0f) m_speedKmh = 0.0f;

	// --- vida do veiculo ---
	// Lido direto do jogo, igual StatusHUD faz com vida/colete do player -
	// nao precisa de RPC pra isso.
	m_health = pVeh->fHealth / kMaxVehicleHealth;
	if (m_health < 0.0f) m_health = 0.0f;
	if (m_health > 1.0f) m_health = 1.0f;

	// combustivel NAO e' lido aqui - nao existe campo nativo pra isso no
	// jogo. So muda via setFuel(), chamado quando chega o RPC 222 da GM.
}

void SpeedometerHUD::setFuel(float percent)
{
	m_fuel = percent < 0.0f ? 0.0f : (percent > 1.0f ? 1.0f : percent);
}

void SpeedometerHUD::setGMVisible(bool visible)
{
	m_gmVisible = visible;
	if (!m_gmVisible) {
		setVisible(false);
	}
}

void SpeedometerHUD::drawTrack(ImGuiRenderer* renderer, const ImVec2& center, float radius, float thickness, float angleMinDeg, float angleMaxDeg, const ImColor& trackColor)
{
	renderer->drawArc(center, radius, thickness, trackColor, angleMinDeg, angleMaxDeg);
}

void SpeedometerHUD::draw(ImGuiRenderer* renderer)
{
	if (!m_inVehicle) {
		return;
	}

	ImVec2 basePos = absolutePosition();

	float radius = 130.0f;       // raio do arco principal (velocidade)
	float outerRadius = radius + 24.0f; // raio dos arcos de vida/combustivel

	ImVec2 center(basePos.x + outerRadius + 4.0f, basePos.y + outerRadius + 4.0f);

	// ===== arco de velocidade (amarelo) - o principal, 270 graus =====
	// Convencao de angulo do ImGui: 0 = direita (3h), 90 = baixo (6h),
	// 180 = esquerda (9h), 270 = cima (12h), sentido horario. 135 graus e'
	// exatamente o canto inferior-esquerdo (~7:30h); 405 (=45+360) e' o
	// inferior-direito (~4:30h) depois de dar a volta por cima - sweep
	// total de 270 graus, igual painel de carro de verdade.
	const float speedAngleMin = 135.0f;
	const float speedAngleMax = 405.0f;

	float speedPercent = m_speedKmh / kMaxSpeedKmh;
	if (speedPercent > 1.0f) speedPercent = 1.0f;

	drawTrack(renderer, center, radius, 12.0f, speedAngleMin, speedAngleMax, ImColor(35, 35, 35, 220));
	if (speedPercent > 0.0f) {
		renderer->drawArc(center, radius, 12.0f, ImColor(230, 205, 40),
			speedAngleMin, speedAngleMin + (speedAngleMax - speedAngleMin) * speedPercent);
	}

	// ===== arco de vida do veiculo (branco) - esquerda/topo, por fora =====
	// Ocupa so a primeira metade do sweep de velocidade (canto inferior-
	// esquerdo ate o topo). Enche crescendo do inicio (canto) pro topo.
	const float healthAngleMin = 135.0f;
	const float healthAngleMax = 270.0f;

	drawTrack(renderer, center, outerRadius, 7.0f, healthAngleMin, healthAngleMax, ImColor(60, 60, 60, 180));
	if (m_health > 0.0f) {
		renderer->drawArc(center, outerRadius, 7.0f, ImColor(240, 240, 240),
			healthAngleMin, healthAngleMin + (healthAngleMax - healthAngleMin) * m_health);
	}

	// ===== arco de combustivel (verde) - direita/topo, por fora =====
	// Espelho do arco de vida: topo ate o canto inferior-direito. Enche
	// crescendo do FIM (canto) pro topo, pra visualmente "nascer" do mesmo
	// lugar que a barra de vida nasce (o canto de baixo).
	const float fuelAngleMin = 270.0f;
	const float fuelAngleMax = 405.0f;

	drawTrack(renderer, center, outerRadius, 7.0f, fuelAngleMin, fuelAngleMax, ImColor(60, 60, 60, 180));
	if (m_fuel > 0.0f) {
		float fuelSweep = (fuelAngleMax - fuelAngleMin) * m_fuel;
		renderer->drawArc(center, outerRadius, 7.0f, ImColor(70, 195, 90), fuelAngleMax - fuelSweep, fuelAngleMax);
	}

	// ===== numero central (velocidade) + "KM/H" =====
	char speedBuf[16];
	snprintf(speedBuf, sizeof(speedBuf), "%d", (int) (m_speedKmh + 0.5f));
	std::string speedStr(speedBuf);

	ImVec2 speedSize = renderer->calculateTextSize(speedStr, 46.0f);
	renderer->drawText(
		ImVec2(center.x - speedSize.x / 2.0f, center.y - speedSize.y / 2.0f - 12.0f),
		ImColor(255, 255, 255), speedStr, true, 46.0f);

	std::string unitStr = "KM/H";
	ImVec2 unitSize = renderer->calculateTextSize(unitStr, 16.0f);
	renderer->drawText(
		ImVec2(center.x - unitSize.x / 2.0f, center.y + speedSize.y / 2.0f - 4.0f),
		ImColor(190, 190, 190), unitStr, false, 16.0f);

	// ===== rotulo "100%" (vida) perto do inicio do arco branco =====
	char healthBuf[8];
	snprintf(healthBuf, sizeof(healthBuf), "%d%%", (int) (m_health * 100.0f + 0.5f));
	std::string healthStr(healthBuf);

	float healthLabelAngle = healthAngleMin * (float) M_PI / 180.0f;
	ImVec2 healthLabelPos(
		center.x + (outerRadius + 26.0f) * cosf(healthLabelAngle) - 24.0f,
		center.y + (outerRadius + 26.0f) * sinf(healthLabelAngle) - 10.0f);
	renderer->drawText(healthLabelPos, ImColor(255, 255, 255), healthStr, true, 18.0f);

	Widget::draw(renderer);
}
