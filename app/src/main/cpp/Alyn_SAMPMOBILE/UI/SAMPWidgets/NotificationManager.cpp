#include <algorithm>
#include <chrono>
#include <cmath>
#include "../UI.h"
#include "../../Client.h"
#include "../../Game/Game.h"
#include "NotificationManager.h"

extern Game* pGame;

namespace {

uint64_t NowMs()
{
	using namespace std::chrono;
	return (uint64_t) duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

NotificationManager::NotificationManager()
{
}

// ===================== estilo por categoria =====================

const NotificationManager::CategoryStyle& NotificationManager::styleFor(NotificationCategory category)
{
	// cor / titulo padrao / duracao padrao / prioridade padrao / som / glifo texto (nullptr = vetorial, ver drawIcon)
	static const CategoryStyle table[(size_t) NotificationCategory::COUNT] = {
		{ ImColor(46, 175, 90),   "Sucesso",        3000, NotificationPriority::Normal, 0, nullptr }, // Success
		{ ImColor(200, 35, 90),   "Erro",           5000, NotificationPriority::High,   0, nullptr }, // Error
		{ ImColor(235, 150, 30),  "Alerta",         4000, NotificationPriority::Normal, 0, nullptr }, // Warning
		{ ImColor(30, 190, 210),  "Info",           3000, NotificationPriority::Low,    0, "i" },     // Info
		{ ImColor(130, 130, 130), "Sistema",        3000, NotificationPriority::Low,    0, "S" },     // System
		{ ImColor(180, 180, 40),  "Dinheiro",       2500, NotificationPriority::Normal, 0, "$" },     // Money
		{ ImColor(40, 110, 220),  "Veiculo",        3000, NotificationPriority::Normal, 0, "V" },     // Vehicle
		{ ImColor(150, 80, 210),  "Item",           3000, NotificationPriority::Normal, 0, "I" },     // Item
		{ ImColor(230, 190, 40),  "Missao",         4000, NotificationPriority::Normal, 0, "M" },     // Mission
		{ ImColor(25, 55, 120),   "Policia",        4000, NotificationPriority::High,   0, "P" },     // Police
		{ ImColor(170, 30, 30),   "Administracao",  5000, NotificationPriority::High,   0, "A" },     // Admin
	};

	size_t idx = (size_t) category;
	if (idx >= (size_t) NotificationCategory::COUNT) idx = (size_t) NotificationCategory::Info;
	return table[idx];
}

// ===================== geometria/ancoragem =====================

float NotificationManager::anchorX() const
{
	switch (m_position) {
		case NotificationPosition::TopLeft:
		case NotificationPosition::CenterLeft:
		case NotificationPosition::BottomLeft:
			return kScreenMargin;

		case NotificationPosition::TopCenter:
		case NotificationPosition::Center:
		case NotificationPosition::BottomCenter:
			return (width() - m_cardWidth) / 2.0f;

		default: // *_Right
			return width() - m_cardWidth - kScreenMargin;
	}
}

bool NotificationManager::growsUpward() const
{
	return m_position == NotificationPosition::BottomLeft ||
		m_position == NotificationPosition::BottomCenter ||
		m_position == NotificationPosition::BottomRight;
}

float NotificationManager::anchorYStart() const
{
	switch (m_position) {
		case NotificationPosition::TopLeft:
		case NotificationPosition::TopCenter:
		case NotificationPosition::TopRight:
			// kScreenMargin sozinho colidia com o HUD nativo (vida/dinheiro/
			// arma) que o proprio jogo desenha nesse canto - m_topReserve
			// empurra o inicio da pilha pra baixo dele (ver setTopReserve()).
			return kScreenMargin + m_topReserve;

		case NotificationPosition::BottomLeft:
		case NotificationPosition::BottomCenter:
		case NotificationPosition::BottomRight:
			return height() - kScreenMargin; // ponto de partida - cresce pra CIMA a partir daqui

		default: // Center*
			return height() / 2.0f;
	}
}

float NotificationManager::slideDirection() const
{
	switch (m_position) {
		case NotificationPosition::TopLeft:
		case NotificationPosition::CenterLeft:
		case NotificationPosition::BottomLeft:
			return -1.0f;

		case NotificationPosition::TopRight:
		case NotificationPosition::CenterRight:
		case NotificationPosition::BottomRight:
			return 1.0f;

		default: // posicoes centrais - sem deslocamento horizontal, so fade+scale
			return 0.0f;
	}
}

void NotificationManager::layoutStack()
{
	float y = anchorYStart();

	if (growsUpward()) {
		// a notificacao mais NOVA (fim do vetor, ja que m_active preserva
		// ordem de insercao) fica perto da margem; as mais antigas sobem.
		for (auto it = m_active.rbegin(); it != m_active.rend(); ++it) {
			if (it->state == NotifState::Dead) continue;
			y -= it->cachedHeight;
			it->targetY = y;
			y -= m_spacing;
		}
	}
	else {
		// no TOPO/CENTRO: a notificacao mais NOVA (fim do vetor, m_active
		// preserva ordem de insercao) fica na "cabeca" da pilha, perto da
		// margem/reserva - as mais antigas vao sendo empurradas pra BAIXO.
		// (antes era o contrario: a mais antiga ficava fixa no topo e as
		// novas entravam embaixo - trocado a pedido).
		for (auto it = m_active.rbegin(); it != m_active.rend(); ++it) {
			if (it->state == NotifState::Dead) continue;
			it->targetY = y;
			y += it->cachedHeight + m_spacing;
		}
	}
}

// ===================== texto =====================

std::vector<std::string> NotificationManager::wrapText(ImGuiRenderer* renderer, const std::string& text, float maxWidth, float fontSize) const
{
	std::vector<std::string> lines;
	std::string currentLine;
	std::string word;

	auto flushWord = [&](const std::string& w) {
		if (w.empty()) return;
		std::string candidate = currentLine.empty() ? w : currentLine + " " + w;
		ImVec2 sz = renderer->calculateTextSize(candidate, fontSize);
		if (sz.x <= maxWidth || currentLine.empty()) {
			// currentLine.empty() garante que uma palavra sozinha maior que
			// maxWidth ainda vai pra uma linha (nao trava num loop infinito
			// tentando quebrar no meio da palavra - limitacao conhecida pra
			// tokens gigantes tipo URL sem espaco).
			currentLine = candidate;
		}
		else {
			lines.push_back(currentLine);
			currentLine = w;
		}
	};

	for (char c : text) {
		if (c == ' ') {
			flushWord(word);
			word.clear();
		}
		else if (c == '\n') {
			flushWord(word);
			word.clear();
			lines.push_back(currentLine);
			currentLine.clear();
		}
		else {
			word += c;
		}
	}
	flushWord(word);
	if (!currentLine.empty() || lines.empty()) {
		lines.push_back(currentLine);
	}

	return lines;
}

float NotificationManager::computeCardHeight(ImGuiRenderer* renderer, const Notification& n) const
{
	float textAreaWidth = m_cardWidth - (kPadding * 2.0f) - kIconDiameter - kIconGap;
	std::vector<std::string> lines = wrapText(renderer, n.message, textAreaWidth, kMsgFontSize);

	float textBlockHeight = kTitleFontSize + kTitleToMsgGap + lines.size() * (kMsgFontSize + kLineSpacing);
	float contentHeight = textBlockHeight > kIconDiameter ? textBlockHeight : kIconDiameter;

	float bottomArea = (m_progressBarEnabled && n.showProgressBar) ? kProgressBarAreaHeight : (kPadding * 0.5f);
	return kPadding + contentHeight + bottomArea;
}

// ===================== desenho =====================

void NotificationManager::drawIcon(ImGuiRenderer* renderer, const ImVec2& center, float hexRadius, NotificationCategory category, const CategoryStyle& style) const
{
	// hexagono com fundo branco + borda colorida grossa - reaproveita a
	// mesma primitiva do HUD de hexagonos (StatusHUD): percent=1.0 desenha
	// o contorno inteiro.
	renderer->drawHexagonFilled(center, hexRadius, ImColor(255, 255, 255, 255));
	renderer->drawHexagonProgress(center, hexRadius, 5.0f, style.color, 1.0f);

	// miolo colorido (circulo) com o glifo branco em cima
	float circleRadius = hexRadius * 0.62f;
	renderer->drawCircleFilled(center, circleRadius, style.color);

	ImColor white(255, 255, 255);

	if (style.glyphText != nullptr) {
		// glifo em texto - simples e funcional pras categorias estendidas.
		// Pra icones "de verdade" (silhueta de carro, distintivo, etc), o
		// caminho natural e' trocar isso por textura via AssetImageLoader,
		// igual o StatusHUD ja faz com os icones de fome/sede/vida.
		float glyphFontSize = hexRadius * 0.85f;
		ImVec2 glyphSize = renderer->calculateTextSize(style.glyphText, glyphFontSize);
		renderer->drawText(ImVec2(center.x - glyphSize.x / 2.0f, center.y - glyphSize.y / 2.0f), white, style.glyphText, false, glyphFontSize);
		return;
	}

	// glifos vetoriais das 3 categorias da foto de referencia (Info usa "i"
	// em texto mesmo, ja coberto pelo bloco acima)
	float s = circleRadius * 0.55f;
	float thickness = hexRadius * 0.12f;
	if (thickness < 3.0f) thickness = 3.0f;

	if (category == NotificationCategory::Success) {
		// checkmark: dois segmentos formando um "V" assimetrico
		renderer->drawLine(ImVec2(center.x - s, center.y + s * 0.1f), ImVec2(center.x - s * 0.25f, center.y + s * 0.75f), white, thickness);
		renderer->drawLine(ImVec2(center.x - s * 0.25f, center.y + s * 0.75f), ImVec2(center.x + s, center.y - s * 0.6f), white, thickness);
	}
	else if (category == NotificationCategory::Error) {
		// X: duas linhas cruzadas
		renderer->drawLine(ImVec2(center.x - s, center.y - s), ImVec2(center.x + s, center.y + s), white, thickness);
		renderer->drawLine(ImVec2(center.x - s, center.y + s), ImVec2(center.x + s, center.y - s), white, thickness);
	}
	else if (category == NotificationCategory::Warning) {
		// "!" - barra vertical + ponto
		renderer->drawLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y + s * 0.25f), white, thickness);
		renderer->drawCircleFilled(ImVec2(center.x, center.y + s * 0.65f), thickness * 0.65f, white);
	}
}

void NotificationManager::drawCard(ImGuiRenderer* renderer, const Notification& n, float alpha, float offsetX, float scale) const
{
	if (alpha <= 0.01f) return;

	const CategoryStyle& style = styleFor(n.category);
	const std::string title = n.title.empty() ? style.defaultTitle : n.title;

	float cardX = anchorX() + offsetX;
	float cardY = n.currentY;
	float cardW = m_cardWidth;
	float cardH = n.cachedHeight;

	ImVec2 rectMin(cardX, cardY);
	ImVec2 rectMax(cardX + cardW, cardY + cardH);

	// "scale" leve (entrada/saida) - infla/desinfla a caixa em volta do
	// proprio centro. O texto e' desenhado na posicao final o tempo todo;
	// durante os ~200ms de transicao ele pode "vazar" 1-2px da caixa
	// encolhida - imperceptivel na pratica, e evita ter que reimplementar
	// um transform completo so pra isso.
	if (scale < 1.0f) {
		ImVec2 c((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);
		rectMin.x = c.x + (rectMin.x - c.x) * scale;
		rectMax.x = c.x + (rectMax.x - c.x) * scale;
		rectMin.y = c.y + (rectMin.y - c.y) * scale;
		rectMax.y = c.y + (rectMax.y - c.y) * scale;
	}

	// sombra suave: 3 camadas deslocadas com alpha decrescente. Nao existe
	// blur de verdade nas primitivas do ImDrawList - isso e' uma
	// aproximacao barata que fica bem o suficiente numa tela de celular.
	for (int i = 3; i >= 1; i--) {
		float shadowAlpha = alpha * (0.05f * i);
		ImVec2 sMin(rectMin.x + i * 1.5f, rectMin.y + i * 2.0f);
		ImVec2 sMax(rectMax.x + i * 1.5f, rectMax.y + i * 2.0f);
		renderer->drawRoundedRectFilled(sMin, sMax, kCardRounding, ImColor(0.0f, 0.0f, 0.0f, shadowAlpha));
	}

	renderer->drawRoundedRectFilled(rectMin, rectMax, kCardRounding, ImColor(1.0f, 1.0f, 1.0f, 0.97f * alpha));

	ImVec2 iconCenter(cardX + kPadding + kIconDiameter / 2.0f, cardY + cardH / 2.0f);
	drawIcon(renderer, iconCenter, kIconDiameter / 2.0f, n.category, style);

	float textX = cardX + kPadding + kIconDiameter + kIconGap;
	float textAreaWidth = cardW - (kPadding * 2.0f) - kIconDiameter - kIconGap;

	std::vector<std::string> lines = wrapText(renderer, n.message, textAreaWidth, kMsgFontSize);
	float textBlockHeight = kTitleFontSize + kTitleToMsgGap + lines.size() * (kMsgFontSize + kLineSpacing);
	float bottomArea = (m_progressBarEnabled && n.showProgressBar) ? kProgressBarAreaHeight : (kPadding * 0.5f);
	float textY = cardY + (cardH - bottomArea - textBlockHeight) / 2.0f;

	ImColor blackTitle(0.0f, 0.0f, 0.0f, alpha);
	ImColor grayMsg(0.15f, 0.15f, 0.15f, alpha * 0.92f);
	ImColor grayCount(0.45f, 0.45f, 0.45f, alpha * 0.8f);

	renderer->drawText(ImVec2(textX, textY), blackTitle, title, false, kTitleFontSize);

	float msgY = textY + kTitleFontSize + kTitleToMsgGap;
	for (const auto& line : lines) {
		renderer->drawText(ImVec2(textX, msgY), grayMsg, line, false, kMsgFontSize);
		msgY += kMsgFontSize + kLineSpacing;
	}

	// badge "(N)" no canto superior direito
	char countBuf[16];
	snprintf(countBuf, sizeof(countBuf), "(%d)", n.count);
	std::string countStr(countBuf);
	ImVec2 countSize = renderer->calculateTextSize(countStr, kCountFontSize);
	renderer->drawText(ImVec2(cardX + cardW - kPadding - countSize.x, cardY + kPadding - 2.0f), grayCount, countStr, false, kCountFontSize);

	// barra de progresso (tempo restante) - so faz sentido enquanto Showing;
	// durante Entering fica cheia (100%), e simplesmente some junto com o
	// resto do card durante Exiting (segue a mesma alpha).
	if (m_progressBarEnabled && n.showProgressBar) {
		float percentRemaining = 1.0f;
		if (n.state == NotifState::Showing && n.durationMs > 0) {
			uint64_t elapsed = NowMs() - n.showStartMs;
			percentRemaining = 1.0f - (float) elapsed / (float) n.durationMs;
			if (percentRemaining < 0.0f) percentRemaining = 0.0f;
			if (percentRemaining > 1.0f) percentRemaining = 1.0f;
		}

		const float barThickness = 4.0f;
		float barY = cardY + cardH - 13.0f;
		float barLeft = cardX + kPadding * 0.5f;
		float barRight = cardX + cardW - kPadding * 0.5f;

		renderer->drawRoundedRectFilled(ImVec2(barLeft, barY), ImVec2(barRight, barY + barThickness), barThickness * 0.5f, ImColor(0.82f, 0.82f, 0.82f, alpha * 0.9f));

		float fillRight = barLeft + (barRight - barLeft) * percentRemaining;
		if (fillRight > barLeft) {
			renderer->drawRoundedRectFilled(ImVec2(barLeft, barY), ImVec2(fillRight, barY + barThickness), barThickness * 0.5f,
				ImColor(style.color.Value.x, style.color.Value.y, style.color.Value.z, alpha));
		}
	}
}

void NotificationManager::draw(ImGuiRenderer* renderer)
{
	if (m_active.empty()) return;

	// 1) recalcula a altura de cada card (depende do texto, que so o
	//    renderer sabe medir de verdade) e o layout (targetY) usando essas
	//    alturas frescas - isso alimenta a suavizacao de posicao que
	//    update() faz no PROXIMO frame (defasagem de 1 frame, imperceptivel).
	for (auto& n : m_active) {
		if (n.state == NotifState::Dead) continue;
		n.cachedHeight = computeCardHeight(renderer, n);
	}
	layoutStack();

	// 2) desenha cada card com a alpha/offset/scale da animacao atual
	for (const auto& n : m_active) {
		if (n.state == NotifState::Dead) continue;

		float alpha = 1.0f;
		float offsetX = 0.0f;
		float scale = 1.0f;

		if (n.state == NotifState::Entering) {
			float t = n.animT < 0.0f ? 0.0f : (n.animT > 1.0f ? 1.0f : n.animT);
			float eased = 1.0f - (1.0f - t) * (1.0f - t); // ease-out quad
			alpha = eased;
			scale = 0.92f + 0.08f * eased;
			offsetX = (1.0f - eased) * slideDirection() * 40.0f;
		}
		else if (n.state == NotifState::Exiting) {
			float t = n.animT < 0.0f ? 0.0f : (n.animT > 1.0f ? 1.0f : n.animT);
			float eased = t * t; // ease-in quad
			alpha = 1.0f - eased;
			scale = 1.0f - 0.06f * eased;
			offsetX = eased * slideDirection() * 60.0f;
		}

		drawCard(renderer, n, alpha, offsetX, scale);
	}

	Widget::draw(renderer);
}

// ===================== fila / ciclo de vida =====================

void NotificationManager::promoteFromQueue()
{
	while (!m_queue.empty()) {
		// capacidade conta so quem esta "viva" de verdade (Entering/Showing)
		// - uma que ja esta Exiting nao ocupa mais vaga, ela so continua
		// desenhada ali pela animacao de saida (ver draw()). De quebra, essa
		// varredura ja acha a mais antiga ainda viva (m_active preserva
		// ordem de insercao, entao a primeira que bater e' ela).
		int aliveCount = 0;
		Notification* oldestAlive = nullptr;
		for (auto& n : m_active) {
			if (n.state == NotifState::Dead || n.state == NotifState::Exiting) continue;
			aliveCount++;
			if (oldestAlive == nullptr) oldestAlive = &n;
		}

		if (aliveCount >= m_maxVisible) {
			// no limite (ex.: 4/4) - a mais antiga sai automaticamente pra
			// abrir espaco pra essa nova, em vez de deixar a nova esperando
			// na fila ate uma expirar sozinha. So marca Exiting aqui (ela
			// ainda anima saindo por kExitAnimMs); no proximo update() ela
			// ja nao entra mais em aliveCount e a promocao segue.
			if (oldestAlive != nullptr) {
				oldestAlive->state = NotifState::Exiting;
				oldestAlive->animT = 0.0f;
			}
			break;
		}

		PendingNotification pending = m_queue.front();
		m_queue.pop_front();

		const CategoryStyle& style = styleFor(pending.category);

		Notification n;
		n.id = pending.id;
		n.category = pending.category;
		n.title = pending.title;
		n.message = pending.message;
		n.durationMs = pending.durationMs > 0 ? pending.durationMs : style.defaultDurationMs;
		n.priority = pending.priority;
		n.showProgressBar = pending.showProgressBar;
		n.state = NotifState::Entering;
		n.animT = 0.0f;
		n.targetY = anchorYStart(); // valor provisorio - o proximo draw()/layoutStack() ajusta de verdade
		n.currentY = n.targetY;

		m_active.push_back(n);

		playSound(style);
	}
}

void NotificationManager::playSound(const CategoryStyle& style) const
{
	if (!m_soundEnabled || style.soundId <= 0 || !pGame) return;

	// Game::PlaySound e' posicional (som 3D nativo do GTA). O ideal pra um
	// som de UI e' passar a posicao da camera/ped local, mas nao confirmei
	// nesse codebase qual e' o getter exato de posicao do sa::CPed (os
	// outros usos de PlaySound que achei, em LocalPlayer.cpp, usam uma
	// CMatrix ja calculada ali mesmo - "matPlayer.pos"). Troquem os 3 zeros
	// abaixo pela posicao real do player antes de compilar com som ligado.
	pGame->PlaySound(style.soundId, 0.0f, 0.0f, 0.0f);
}

uint32_t NotificationManager::show(NotificationCategory category, const std::string& title, const std::string& message,
	uint32_t durationMs, NotificationPriority priority, bool showProgressBar)
{
	// dedupe: mesma categoria + mesma mensagem ja ativa (e nao saindo) =
	// so incrementa o contador e reinicia o tempo, em vez de empilhar duas
	// notificacoes iguais.
	for (auto& n : m_active) {
		if (n.category == category && n.message == message && n.state != NotifState::Dead && n.state != NotifState::Exiting) {
			n.count++;
			n.showStartMs = NowMs();
			return n.id;
		}
	}

	uint32_t id = m_nextId++;

	PendingNotification pending;
	pending.category = category;
	pending.title = title;
	pending.message = message;
	pending.durationMs = durationMs;
	pending.priority = priority;
	pending.showProgressBar = showProgressBar;
	pending.id = id;

	// insere na fila respeitando prioridade (maior primeiro, FIFO entre
	// notificacoes de mesma prioridade)
	auto it = m_queue.begin();
	while (it != m_queue.end() && (uint8_t) it->priority >= (uint8_t) priority) {
		++it;
	}
	m_queue.insert(it, pending);

	promoteFromQueue();
	return id;
}

void NotificationManager::remove(uint32_t id)
{
	for (auto& n : m_active) {
		if (n.id == id && n.state != NotifState::Dead && n.state != NotifState::Exiting) {
			n.state = NotifState::Exiting;
			n.animT = 0.0f;
			return;
		}
	}

	// tambem pode estar so esperando espaco na fila - nesse caso remove direto
	m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(),
		[id](const PendingNotification& p) { return p.id == id; }), m_queue.end());
}

void NotificationManager::removeAll()
{
	for (auto& n : m_active) {
		if (n.state != NotifState::Dead && n.state != NotifState::Exiting) {
			n.state = NotifState::Exiting;
			n.animT = 0.0f;
		}
	}
	m_queue.clear();
}

void NotificationManager::update()
{
	uint64_t nowMs = NowMs();

	float dtMs = m_lastUpdateMs == 0 ? 16.0f : (float) (nowMs - m_lastUpdateMs);
	if (dtMs <= 0.0f || dtMs > 250.0f) dtMs = 16.0f; // primeiro frame ou hitch grande (app voltou do background etc)
	m_lastUpdateMs = nowMs;

	float dt = dtMs / 1000.0f;
	float posSmoothing = 1.0f - powf(0.0001f, dt); // convergencia rapida e independente de FPS

	bool anyDead = false;

	for (auto& n : m_active) {
		switch (n.state) {
			case NotifState::Entering:
				n.animT += dtMs / (float) kEnterAnimMs;
				if (n.animT >= 1.0f) {
					n.animT = 1.0f;
					n.state = NotifState::Showing;
					n.showStartMs = nowMs;
				}
				break;

			case NotifState::Showing:
				if (n.durationMs > 0 && (nowMs - n.showStartMs) >= n.durationMs) {
					n.state = NotifState::Exiting;
					n.animT = 0.0f;
				}
				break;

			case NotifState::Exiting:
				n.animT += dtMs / (float) kExitAnimMs;
				if (n.animT >= 1.0f) {
					n.animT = 1.0f;
					n.state = NotifState::Dead;
					anyDead = true;
				}
				break;

			case NotifState::Dead:
				anyDead = true;
				break;
		}

		// suaviza a posicao vertical rumo ao alvo calculado no ultimo draw()
		n.currentY += (n.targetY - n.currentY) * posSmoothing;
	}

	if (anyDead) {
		m_active.erase(std::remove_if(m_active.begin(), m_active.end(),
			[](const Notification& n) { return n.state == NotifState::Dead; }),
			m_active.end());
	}

	promoteFromQueue();

	setVisible(hasActive());
}
