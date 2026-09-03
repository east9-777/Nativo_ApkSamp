#pragma once

#include <vector>
#include <deque>
#include <string>
#include <cstdint>
#include "../../Game/Game.h"

/**
 * NotificationManager: sistema de notificacoes tipo "toast" (cards
 * empilhados, hexagono colorido + icone, titulo, mensagem com quebra de
 * linha automatica, badge de contagem e barra de progresso). Estilo visual
 * baseado na referencia enviada (cards brancos arredondados).
 *
 * Uso a partir do resto do APK (client-side):
 *     pUI->notifications()->show(NotificationCategory::Success, "", "Compra realizada!");
 *
 * Uso a partir da gamemode (server-side, Pawn): ver Toast.pwn. IMPORTANTE:
 * esse sistema NAO tem nenhuma relacao com o Notification_Show/
 * NotificationNT9 que ja existe em outro lugar da GM de voces - achei que
 * fossem a mesma coisa numa entrega anterior, mas nao sao (por isso o lado
 * Pawn usa nomenclatura ToastNotify_*/ToastCategory, pra nunca colidir com
 * aquele outro sistema). O RPC 223 (ScrNativoNotificationShow em
 * Net/ScriptRPC.cpp) e' o unico ponto de entrada externo - o resto do APK
 * e a GM NUNCA mexem direto nos cards, so chamam show()/remove()/
 * removeAll() aqui (mesma filosofia pedida no prompt: "o restante do APK
 * deve interagir somente com o Notification Manager").
 *
 * Titulo, cor e icone de cada card vem da CATEGORIA (CategoryStyle,
 * styleFor()) - por padrao, sem precisar passar titulo (ToastNotify_Show
 * no lado Pawn nao pede titulo). Se precisarem de um titulo customizado
 * por notificacao, o campo title ja existe (ToastNotify_ShowAdvanced no
 * lado Pawn preenche ele).
 *
 * Animacao: fade + slide horizontal (a partir do lado de fora mais proximo
 * da posicao configurada) + um leve "scale" na caixa do card. Simples de
 * proposito - da pra trocar a curva de easing ou adicionar novos tipos
 * depois sem mexer na arquitetura (tudo passa por um unico "t" 0..1 em
 * draw(), ver slideDirection()/animT).
 */

enum class NotificationCategory : uint8_t {
	Success = 0,
	Error,
	Warning,
	Info,
	System,
	Money,
	Vehicle,
	Item,
	Mission,
	Police,
	Admin,

	COUNT
};

enum class NotificationPriority : uint8_t {
	Low = 0,
	Normal,
	High,
	Critical
};

// Onde o stack de notificacoes fica ancorado na tela. Posicoes de TOPO e
// CENTRO empilham pra BAIXO (a mais nova entra embaixo da pilha). Posicoes
// de BAIXO empilham pra CIMA (a mais nova entra perto da margem inferior,
// empurrando as mais antigas pra cima) - assim a pilha nunca "foge" da
// tela, nao importa quantas notificacoes tenham simultaneamente.
enum class NotificationPosition : uint8_t {
	TopLeft = 0,
	TopCenter,
	TopRight,
	CenterLeft,
	Center,
	CenterRight,
	BottomLeft,
	BottomCenter,
	BottomRight
};

class NotificationManager : public Widget {
public:
	NotificationManager();

	// chamado todo frame (Idle hook) - avanca timers/estado (entrando,
	// mostrando, saindo), remove notificacoes mortas e promove a fila. Nao
	// precisa do renderer (so draw() precisa, pra medir texto).
	void update();

	virtual void draw(ImGuiRenderer* renderer) override;

	// ===== API principal =====
	// title vazio ("") = usa o titulo padrao da categoria (igual o prompt
	// original: cada categoria ja tem um titulo fixo tipo "Sucesso"/"Erro").
	// durationMs 0 = usa a duracao padrao da categoria.
	// Se ja existir uma notificacao IGUAL (mesma categoria+mensagem) ativa
	// na tela, em vez de empilhar duas iguais o sistema so incrementa o
	// contador dela (badge "(N)") e reinicia o tempo - devolve o id dessa
	// notificacao existente nesse caso.
	uint32_t show(NotificationCategory category, const std::string& title, const std::string& message,
		uint32_t durationMs = 0, NotificationPriority priority = NotificationPriority::Normal,
		bool showProgressBar = true);

	void remove(uint32_t id);
	void removeAll();
	bool hasActive() const { return !m_active.empty() || !m_queue.empty(); }

	// ===== configuracao (camada pedida no prompt) =====
	void setScreenPosition(NotificationPosition position) { m_position = position; }
	void setMaxVisible(int max) { m_maxVisible = max < 1 ? 1 : max; }
	void setDefaultDuration(uint32_t ms) { m_defaultDurationMs = ms; }
	void setSpacing(float px) { m_spacing = px; }
	void setCardWidth(float px) { m_cardWidth = px; }
	void setProgressBarEnabled(bool enabled) { m_progressBarEnabled = enabled; }
	void setSoundEnabled(bool enabled) { m_soundEnabled = enabled; }

private:
	enum class NotifState : uint8_t { Entering, Showing, Exiting, Dead };

	struct CategoryStyle {
		ImColor color;
		const char* defaultTitle;
		uint32_t defaultDurationMs;
		NotificationPriority defaultPriority;
		int soundId;           // ID de som nativo do jogo (Game::PlaySound) - 0 = mudo/desabilitado
		const char* glyphText; // glifo em texto (nullptr = usa desenho vetorial, ver drawIcon)
	};

	struct Notification {
		uint32_t id = 0;
		NotificationCategory category = NotificationCategory::Info;
		std::string title;
		std::string message;
		uint32_t durationMs = 0;
		NotificationPriority priority = NotificationPriority::Normal;
		bool showProgressBar = true;
		int count = 1; // badge "(N)" - sobe quando a MESMA notificacao repete (ver show())

		NotifState state = NotifState::Entering;
		uint64_t showStartMs = 0;  // quando entrou em Showing (comeca a contar a duracao)
		float animT = 0.0f;        // 0..1, progresso da animacao atual (entrada OU saida)

		float currentY = 0.0f;     // posicao vertical atual (anima suave ate targetY)
		float targetY = 0.0f;      // posicao vertical alvo (definida por layoutStack())
		float cachedHeight = 0.0f; // altura calculada no ultimo draw() (card e' variavel, texto quebra linha)
	};

	struct PendingNotification {
		NotificationCategory category;
		std::string title;
		std::string message;
		uint32_t durationMs;
		NotificationPriority priority;
		bool showProgressBar;
		uint32_t id;
	};

	static const CategoryStyle& styleFor(NotificationCategory category);

	// devolve as linhas ja quebradas da mensagem pra caber em maxWidth
	std::vector<std::string> wrapText(ImGuiRenderer* renderer, const std::string& text, float maxWidth, float fontSize) const;

	float computeCardHeight(ImGuiRenderer* renderer, const Notification& n) const;
	void drawIcon(ImGuiRenderer* renderer, const ImVec2& center, float hexRadius, NotificationCategory category, const CategoryStyle& style) const;
	void drawCard(ImGuiRenderer* renderer, const Notification& n, float alpha, float offsetX, float scale) const;

	void layoutStack();
	void promoteFromQueue();
	void playSound(const CategoryStyle& style) const;

	// geometria/ancoragem dependentes de m_position (ver comentario do enum)
	float anchorX() const;
	float anchorYStart() const;
	bool growsUpward() const;
	float slideDirection() const; // -1 esquerda / 0 sem deslocamento / +1 direita

private:
	std::vector<Notification> m_active;
	std::deque<PendingNotification> m_queue;
	uint32_t m_nextId = 1;
	uint64_t m_lastUpdateMs = 0;

	NotificationPosition m_position = NotificationPosition::TopRight;
	int m_maxVisible = 4;
	uint32_t m_defaultDurationMs = 4000;
	float m_spacing = 14.0f;
	float m_cardWidth = 380.0f;
	bool m_progressBarEnabled = true;
	bool m_soundEnabled = true;

	// duracao das animacoes de entrada/saida, em ms
	static constexpr uint32_t kEnterAnimMs = 220;
	static constexpr uint32_t kExitAnimMs = 200;

	// constantes de layout do card - compartilhadas entre computeCardHeight()
	// e drawCard() de proposito, pra nunca ficarem fora de sincronia (se um
	// dia mudarem o tamanho do icone, por exemplo, so muda aqui).
	static constexpr float kScreenMargin = 20.0f;
	static constexpr float kPadding = 16.0f;
	static constexpr float kIconDiameter = 56.0f;
	static constexpr float kIconGap = 14.0f;
	static constexpr float kTitleFontSize = 20.0f;
	static constexpr float kMsgFontSize = 15.0f;
	static constexpr float kLineSpacing = 4.0f;
	static constexpr float kTitleToMsgGap = 4.0f;
	static constexpr float kCountFontSize = 14.0f;
	static constexpr float kProgressBarAreaHeight = 22.0f;
	static constexpr float kCardRounding = 18.0f;
};
