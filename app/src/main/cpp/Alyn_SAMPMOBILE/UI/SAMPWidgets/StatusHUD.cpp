#include "../UI.h"
#include "../../Client.h"
#include "../../Game/Game.h"
#include "StatusHUD.h"

extern Game* pGame;

StatusHUD::StatusHUD()
{
	m_health.color = ImColor(220, 40, 40);     // vermelho
	m_health.iconPath = "hud/hud_vida.png";

	m_armour.color = ImColor(60, 130, 220);    // azul
	m_armour.iconPath = "hud/hud_colete.png";

	m_hunger.color = ImColor(215, 190, 40);    // amarelo
	m_hunger.iconPath = "hud/hud_fome.png";

	m_thirst.color = ImColor(50, 190, 220);    // ciano
	m_thirst.iconPath = "hud/hud_sede.png";
}

void StatusHUD::update()
{
// FindPlayerPed() is a lazy factory. Calling it from the HUD during the
// first frames can construct CPlayerPed before GTA has created the local
// actor, which executes script commands against an invalid actor.
if (!pGame) {
setVisible(false);
return;
}

// Read the already-existing GTA ped directly. This path never allocates a
// CPlayerPed and is therefore safe while the game is still booting.
sa::CPed* pPlayerPed = GamePool_FindPlayerPed();
if (!pPlayerPed) {
setVisible(false);
return;
	}

// So aparece se o ped existe E o servidor (GM) nao escondeu o HUD.
setVisible(m_gmVisible);
if (!m_gmVisible) {
	return;
}
m_health.value = pPlayerPed->m_fHealth / 100.0f;
m_armour.value = pPlayerPed->m_fArmour / 100.0f;

if (m_health.value < 0.0f) m_health.value = 0.0f;
if (m_health.value > 1.0f) m_health.value = 1.0f;
if (m_armour.value < 0.0f) m_armour.value = 0.0f;
if (m_armour.value > 1.0f) m_armour.value = 1.0f;
}

void StatusHUD::setHunger(float percent)
{
	m_hunger.value = percent < 0.0f ? 0.0f : (percent > 1.0f ? 1.0f : percent);
}

void StatusHUD::setThirst(float percent)
{
	m_thirst.value = percent < 0.0f ? 0.0f : (percent > 1.0f ? 1.0f : percent);
}

void StatusHUD::setGMVisible(bool visible)
{
	m_gmVisible = visible;
	// aplica na hora, sem esperar o proximo update() (evita 1 frame de atraso
	// visivel quando o GM manda esconder o HUD).
	if (!m_gmVisible) {
		setVisible(false);
	}
}

void StatusHUD::drawBar(ImGuiRenderer* renderer, HexBar& bar, const ImVec2& center, float radius)
{
	// Textura (RwRaster) carregada sob demanda, no primeiro draw(). Le direto
	// de dentro do APK (app/src/main/assets/), nao da pasta de dados baixada.
	if (bar.iconTexture == nullptr) {
		bar.iconTexture = LoadIconTextureFromAsset(bar.iconPath.c_str());
	}

	// 1. fundo hexagonal solido escuro
	renderer->drawHexagonFilled(center, radius, ImColor(20, 20, 20, 230));

	// 2. borda colorida, preenchendo proporcional ao valor
	renderer->drawHexagonProgress(center, radius, radius * 0.12f, bar.color, bar.value);

	// 3. icone centralizado
	if (bar.iconTexture != nullptr) {
		float iconSize = radius * 0.85f;
		ImVec2 iconMin(center.x - iconSize / 2.0f, center.y - iconSize / 2.0f);
		ImVec2 iconMax(center.x + iconSize / 2.0f, center.y + iconSize / 2.0f);
		renderer->drawImage(iconMin, iconMax, (ImTextureID) bar.iconTexture);
	}
}

void StatusHUD::draw(ImGuiRenderer* renderer)
{
	float radius = 32.0f;
	float gap = 12.0f;
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
