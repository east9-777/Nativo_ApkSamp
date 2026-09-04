#pragma once

#include <string>
#include <vector>
#include <bass.h>

// Tela de login/registro migrada da textdraw da GM pra esse widget nativo.
// Fica escondida (setVisible(false)) ate a GM mandar RPC_LOGIN_AUTHSHOW
// (224) dizendo qual modo mostrar - ver ScrNativoLoginAuthShow em
// ScriptRPC.cpp. Segue o mesmo framework de widget que Spawn/Chat/Dialog
// usam (Widget/Layout/Label/Button/EditBox/Image, ver UI/Widgets/), NAO o
// jeito baixo-nivel do NotificationManager (aquele e' so pra cards
// passivos, esse aqui precisa de input de verdade).
enum class LoginScreenMode {
	Hidden,
	Login,     // conta existe - mostra usuario (read-only) + campo de senha + Entrar
	Register   // conta nao existe - so mostra o aviso "registre-se pelo Discord"
};

// Campo de senha - copia minima do EditBox (UI/Widgets/EditBox.h) mas com
// mascara: guarda o texto real digitado, mostra so bullets no label. Nao
// herda de EditBox porque os membros dele (m_label/m_input) sao privados,
// sem gancho pra sobrescrever a exibicao. Visual proprio (caixa arredondada
// escura + icone de cadeado) pra bater com o mockup - NAO reaproveita o
// visual quadrado/contorno azul do EditBox generico usado no resto do app.
class PasswordBox : public Widget {
public:
	PasswordBox();

	const std::string& text() const { return m_input; }
	void clear();

	virtual void performLayout() override;
	virtual void draw(ImGuiRenderer* renderer) override;
	virtual void touchPopEvent() override;
	virtual void keyboardEvent(const std::string& input) override;

private:
	Label* m_maskLabel;
	std::string m_input;
};

// Campo somente-leitura com o MESMO visual arredondado do PasswordBox
// (icone de usuario a esquerda ao inves de cadeado) - usado pro nome de
// usuario, que so mostramos (conta ja existe, nao da pra editar aqui).
class ReadOnlyField : public Widget {
public:
	explicit ReadOnlyField(const std::string& text);

	void setText(const std::string& text);

	virtual void performLayout() override;
	virtual void draw(ImGuiRenderer* renderer) override;

private:
	Label* m_label;
};

// Botao "pilula" arredondado dos botoes Entrar/Cadastrar-se do login.
// primary=true preenche com a cor de destaque (Entrar), primary=false
// desenha so um contorno claro semi-transparente (Cadastrar-se) - bate
// com o mockup. Herda de Button so pra reaproveitar toda a logica de
// touch/focus (Widget::touchEvent, ja testada) - o draw() e 100% proprio,
// nao cai no retangulo azul-contornado generico de Button::draw().
class PillButton : public Button {
public:
	PillButton(const std::string& caption, bool primary);

	virtual void draw(ImGuiRenderer* renderer) override;

private:
	bool m_primary;
};

// Botao circular dos controles de musica (anterior/play-pause/proximo) -
// desenha um icone vetorial (triangulo/barras/chevron duplo) ao inves de
// reciclar o retangulo com contorno azul do Button padrao. So o play/pause
// tem fundo (circulo solido); anterior/proximo ficam so o icone, sem caixa
// nenhuma - e exatamente essa caixa que deixava os controles "feios/
// reciclados do proprio APK".
enum class PlayerIconKind {
	Prev,
	Play,
	Pause,
	Next
};

class PlayerIconButton : public Button {
public:
	explicit PlayerIconButton(PlayerIconKind kind);

	void setKind(PlayerIconKind kind) { m_kind = kind; }

	virtual void draw(ImGuiRenderer* renderer) override;

private:
	PlayerIconKind m_kind;
};

// Um "card" de musica (capa + titulo + artista), metadados fixos - ver
// LOGIN/MUSICA N/informacoes.md que vieram junto com os assets.
struct LoginTrack {
	const char* assetMp3;   // caminho relativo a assets/, ex: "login/music/1.mp3"
	const char* assetCover; // caminho relativo a assets/, ex: "login/covers/1.jpg"
	const char* title;
	const char* artist;
};

class LoginScreen : public Widget {
public:
	LoginScreen();

	// Chamado pelo handler do RPC_LOGIN_AUTHSHOW (224).
	void show(LoginScreenMode mode);

	// Chamado pelo handler do RPC_LOGIN_RESULT (226). Sucesso esconde a
	// tela e para a musica; erro mostra "message" e deixa tentar de novo.
	void onLoginResult(bool success, const std::string& message);

	virtual void performLayout() override;
	virtual void draw(ImGuiRenderer* renderer) override;

private:
	void hide();
	void onEntrarClicked();
	void onCadastrarClicked();
	void onPlayPauseClicked();
	void onNextClicked();
	void onPrevClicked();
	void playTrack(int index);
	void stopMusic();

private:
	LoginScreenMode m_mode;
	std::string m_errorMessage;

	Label* m_welcomeLabel;   // "Bem-vindo, jogador caro!"
	Label* m_subtitleLabel;  // "Para acessar o servidor, faca seu login abaixo"

	ReadOnlyField* m_usernameField; // usuario (read-only, Settings::nick())
	PasswordBox* m_passwordBox;
	Label* m_errorLabel;
	PillButton* m_entrarButton;
	PillButton* m_cadastrarButton;
	Label* m_registerInfoLabel; // aviso "registre-se pelo discord" (modo Register)

	// Mini player de musica
	Label* m_trackTitleLabel;
	Label* m_trackArtistLabel;
	PlayerIconButton* m_playPauseButton;
	PlayerIconButton* m_nextButton;
	PlayerIconButton* m_prevButton;
	ProgressBar* m_musicProgressBar; // preenchimento = posicao real da faixa (BASS_ChannelGetPosition/GetLength)
	int m_trackIndex;
	bool m_playing;

	void* m_bgTexture;    // RwRaster* (login_fundo.jpg)
	void* m_logoTexture;  // RwRaster* (login_logo.png)

	// BASS - stream dedicado do player de login (independente do
	// AudioStream usado pelo som ambiente/radio do mundo, que so sabe
	// tocar URL - ver Game/AudioStream.cpp). Toca direto da memoria porque
	// o mp3 vem embutido no APK (assets/), nao de rede.
	HSTREAM m_musicStream;
	std::vector<unsigned char> m_musicBuffer; // precisa ficar viva enquanto o stream tocar
};
