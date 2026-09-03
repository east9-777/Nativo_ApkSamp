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

	// Grossura (thickness) ficou boa e foi mantida - so o DIAMETRO do
	// velocimetro inteiro diminuiu (estava grande de mais na v2).
	float radius = 108.0f;              // raio do arco principal (velocidade) - era 150
	float outerRadius = radius + 26.0f; // raio dos arcos de vida/combustivel - era +34

	ImVec2 center(basePos.x + outerRadius + 8.0f, basePos.y + outerRadius + 8.0f);

	// ===== arco de velocidade (amarelo) - o principal, 270 graus =====
	// Convencao de angulo do ImGui: 0 = direita (3h), 90 = baixo (6h),
	// 180 = esquerda (9h), 270 = cima (12h), sentido horario. 135 graus e'
	// exatamente o canto inferior-esquerdo (~7:30h); 405 (=45+360) e' o
	// inferior-direito (~4:30h) depois de dar a volta por cima - sweep
	// total de 270 graus, igual painel de carro de verdade.
	const float speedAngleMin = 135.0f;
	const float speedAngleMax = 405.0f;
	const float speedThickness = 20.0f; // era 12 - ficava fino de mais

	float speedPercent = m_speedKmh / kMaxSpeedKmh;
	if (speedPercent > 1.0f) speedPercent = 1.0f;

	drawTrack(renderer, center, radius, speedThickness, speedAngleMin, speedAngleMax, ImColor(35, 35, 35, 220));
	if (speedPercent > 0.0f) {
		renderer->drawArc(center, radius, speedThickness, ImColor(230, 205, 40),
			speedAngleMin, speedAngleMin + (speedAngleMax - speedAngleMin) * speedPercent);
	}

	// ===== arco de vida do veiculo (branco) - canto inferior-esquerdo =====
	// Antes ia ate o topo (135->270) e o de combustivel COMECAVA no topo
	// (270->405) - as duas se encontravam exatamente em cima e formavam uma
	// "ponte" continua. Agora cada uma cobre so METADE desse sweep (~67.5
	// graus), ficando perto do canto de baixo, com um vao vazio no topo -
	// igual a foto de referencia, onde vida e combustivel nao se tocam.
	const float healthAngleMin = 135.0f;
	const float healthAngleMax = 135.0f + (270.0f - 135.0f) / 2.0f; // 202.5
	const float sideThickness = 14.0f; // era 7 - ficava fino de mais

	drawTrack(renderer, center, outerRadius, sideThickness, healthAngleMin, healthAngleMax, ImColor(60, 60, 60, 180));
	if (m_health > 0.0f) {
		renderer->drawArc(center, outerRadius, sideThickness, ImColor(240, 240, 240),
			healthAngleMin, healthAngleMin + (healthAngleMax - healthAngleMin) * m_health);
	}

	// ===== arco de combustivel (vermelho) - canto inferior-direito =====
	// Espelho do arco de vida: metade do sweep, colada no canto de baixo-
	// direita, crescendo do canto (405) pra cima conforme o tanque enche -
	// nunca chega no topo, mesmo com o tanque cheio, deixando o vao contra
	// o arco de vida (ver comentario acima).
	const float fuelAngleMin = 405.0f - (270.0f - 135.0f) / 2.0f; // 337.5
	const float fuelAngleMax = 405.0f;

	drawTrack(renderer, center, outerRadius, sideThickness, fuelAngleMin, fuelAngleMax, ImColor(60, 60, 60, 180));
	if (m_fuel > 0.0f) {
		float fuelSweep = (fuelAngleMax - fuelAngleMin) * m_fuel;
		renderer->drawArc(center, outerRadius, sideThickness, ImColor(215, 45, 45), fuelAngleMax - fuelSweep, fuelAngleMax);
	}

	// ===== numero central (velocidade) + "KM/H" =====
	char speedBuf[16];
	snprintf(speedBuf, sizeof(speedBuf), "%d", (int) (m_speedKmh + 0.5f));
	std::string speedStr(speedBuf);

	const float speedFontSize = 64.0f; // era 46 - numero ficava pequeno de mais
	ImVec2 speedSize = renderer->calculateTextSize(speedStr, speedFontSize);
	renderer->drawText(
		ImVec2(center.x - speedSize.x / 2.0f, center.y - speedSize.y / 2.0f - 14.0f),
		ImColor(255, 255, 255), speedStr, true, speedFontSize);

	std::string unitStr = "KM/H";
	const float unitFontSize = 20.0f; // era 16
	ImVec2 unitSize = renderer->calculateTextSize(unitStr, unitFontSize);
	renderer->drawText(
		ImVec2(center.x - unitSize.x / 2.0f, center.y + speedSize.y / 2.0f - 2.0f),
		ImColor(190, 190, 190), unitStr, false, unitFontSize);

	// ===== rotulo "100%" (vida) perto do inicio do arco branco =====
	char healthBuf[8];
	snprintf(healthBuf, sizeof(healthBuf), "%d%%", (int) (m_health * 100.0f + 0.5f));
	std::string healthStr(healthBuf);

	const float healthFontSize = 22.0f; // era 18
	float healthLabelAngle = healthAngleMin * (float) M_PI / 180.0f;
	ImVec2 healthLabelPos(
		center.x + (outerRadius + 30.0f) * cosf(healthLabelAngle) - 28.0f,
		center.y + (outerRadius + 30.0f) * sinf(healthLabelAngle) - 12.0f);
	renderer->drawText(healthLabelPos, ImColor(255, 255, 255), healthStr, true, healthFontSize);

	Widget::draw(renderer);
}
