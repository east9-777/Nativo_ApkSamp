#include "../UI.h"
#include "StatusHUD.h"

StatusHUD::StatusHUD()
{
	m_health.color = ImColor(220, 40, 40);     // vermelho
	m_health.iconPath = "SAMP/Hud/hud_icons/vida.png";

	m_armour.color = ImColor(60, 130, 220);    // azul
	m_armour.iconPath = "SAMP/Hud/hud_icons/colete.png";

	m_hunger.color = ImColor(215, 190, 40);    // amarelo
	m_hunger.iconPath = "SAMP/Hud/hud_icons/fome.png";

	m_thirst.color = ImColor(50, 190, 220);    // ciano
	m_thirst.iconPath = "SAMP/Hud/hud_icons/sede.png";
}

void StatusHUD::update()
{
	CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
	if (pPlayerPed) {
		m_health.value = pPlayerPed->GetHealth() / 100.0f;
		m_armour.value = pPlayerPed->GetArmour() / 100.0f;

		if (m_health.value < 0.0f) m_health.value = 0.0f;
		if (m_health.value > 1.0f) m_health.value = 1.0f;
		if (m_armour.value < 0.0f) m_armour.value = 0.0f;
		if (m_armour.value > 1.0f) m_armour.value = 1.0f;
	}
}

void StatusHUD::setHunger(float percent)
{
	m_hunger.value = percent < 0.0f ? 0.0f : (percent > 1.0f ? 1.0f : percent);
}

void StatusHUD::setThirst(float percent)
{
	m_thirst.value = percent < 0.0f ? 0.0f : (percent > 1.0f ? 1.0f : percent);
}

void StatusHUD::drawBar(ImGuiRenderer* renderer, HexBar& bar, const ImVec2& center, float radius)
{
	// Textura carregada sob demanda, no primeiro draw() (aqui garantidamente
	// estamos no render thread, com contexto GL valido).
	if (bar.iconTexture == 0) {
		std::string fullPath = std::string(Client::gameDir()) + bar.iconPath;
		bar.iconTexture = LoadIconTextureFromPNG(fullPath.c_str());
	}

	// 1. fundo hexagonal solido escuro
	renderer->drawHexagonFilled(center, radius, ImColor(20, 20, 20, 230));

	// 2. borda colorida, preenchendo proporcional ao valor
	renderer->drawHexagonProgress(center, radius, radius * 0.12f, bar.color, bar.value);

	// 3. icone centralizado
	if (bar.iconTexture != 0) {
		float iconSize = radius * 0.85f;
		ImVec2 iconMin(center.x - iconSize / 2.0f, center.y - iconSize / 2.0f);
		ImVec2 iconMax(center.x + iconSize / 2.0f, center.y + iconSize / 2.0f);
		renderer->drawImage(iconMin, iconMax, (ImTextureID)(intptr_t) bar.iconTexture);
	}
}

void StatusHUD::draw(ImGuiRenderer* renderer)
{
	float radius = 42.0f;
	float gap = 16.0f;
	float diameter = radius * 2.0f;

	ImVec2 basePos = absolutePosition();

	// 4 hexagonos lado a lado, da esquerda pra direita: vida, colete, fome, sede
	ImVec2 healthCenter(basePos.x + radius, basePos.y + radius);
	ImVec2 armourCenter(healthCenter.x + diameter + gap, healthCenter.y);
	ImVec2 hungerCenter(armourCenter.x + diameter + gap, healthCenter.y);
	ImVec2 thirstCenter(hungerCenter.x + diameter + gap, healthCenter.y);

	drawBar(renderer, m_health, healthCenter, radius);
	drawBar(renderer, m_armour, armourCenter, radius);
	drawBar(renderer, m_hunger, hungerCenter, radius);
	drawBar(renderer, m_thirst, thirstCenter, radius);

	Widget::draw(renderer);
}
