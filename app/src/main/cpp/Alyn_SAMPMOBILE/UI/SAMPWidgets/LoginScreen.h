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
// sem gancho pra sobrescrever a exibicao.
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

	Label* m_titleLabel;      // usuario (read-only, Settings::nick())
	PasswordBox* m_passwordBox;
	Label* m_errorLabel;
	Button* m_entrarButton;
	Button* m_cadastrarButton;
	Label* m_registerInfoLabel; // aviso "registre-se pelo discord" (modo Register)

	// Mini player de musica
	Label* m_trackTitleLabel;
	Label* m_trackArtistLabel;
	Button* m_playPauseButton;
	Button* m_nextButton;
	Button* m_prevButton;
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
