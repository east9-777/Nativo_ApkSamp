#include <android/asset_manager.h>

#include "../UI.h"
#include "../../AssetImageLoader.h"

extern UI* pUI;

// Metadados fixos das 3 faixas (LOGIN/MUSICA N/informacoes.md dos assets
// que vieram). Caminhos batem com onde os arquivos foram copiados dentro
// de assets/login/ - ver instrucoes de copia junto do resto da entrega.
static const LoginTrack kLoginTracks[3] = {
	{ "login/music/1.mp3", "login/covers/1.jpg", "After Dark x Sweater Weather", "Mr. Kitty" },
	{ "login/music/2.mp3", "login/covers/2.jpg", "Vida Louca",                   "Mc Poze"    },
	{ "login/music/3.mp3", "login/covers/3.jpg", "Mas Existe Um Lugar",          "Caneta Azul"},
};

// ============================== PasswordBox ==============================

PasswordBox::PasswordBox()
{
	m_maskLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() / 2);
	this->addChild(m_maskLabel);
}

void PasswordBox::clear()
{
	m_input.clear();
	m_maskLabel->setText(" ");
}

void PasswordBox::performLayout()
{
	m_maskLabel->performLayout();
	m_maskLabel->setPosition(ImVec2(
			UISettings::padding(),
			(height() - m_maskLabel->height()) / 2
	));
}

void PasswordBox::draw(ImGuiRenderer* renderer)
{
	renderer->drawRect(
			absolutePosition() + ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			(absolutePosition() + size()) - ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			ImColor(0.287f, 0.287f, 0.287f, 0.890f), false, UISettings::outlineSize());

	Widget::draw(renderer);
}

void PasswordBox::touchPopEvent()
{
	pUI->keyboard()->show(this);
}

void PasswordBox::keyboardEvent(const std::string& input)
{
	m_input = input;

	std::string mask(input.size(), (char) 0x95); // bullet do fonte padrao do client
	m_maskLabel->setText(mask.empty() ? " " : mask);
}

// ============================== LoginScreen ==============================

LoginScreen::LoginScreen()
	: m_mode(LoginScreenMode::Hidden)
	, m_trackIndex(0)
	, m_playing(false)
	, m_bgTexture(nullptr)
	, m_logoTexture(nullptr)
	, m_musicStream(0)
{
	m_titleLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f));
	this->addChild(m_titleLabel);

	m_passwordBox = new PasswordBox();
	this->addChild(m_passwordBox);

	m_errorLabel = new Label(" ", ImColor(0.906f, 0.298f, 0.235f)); // vermelho, mesma familia do ToastCategory_Error
	this->addChild(m_errorLabel);

	m_entrarButton = new Button("Entrar");
	m_entrarButton->setCallback([this]() { onEntrarClicked(); });
	this->addChild(m_entrarButton);

	m_cadastrarButton = new Button("Cadastrar-se");
	m_cadastrarButton->setCallback([this]() { onCadastrarClicked(); });
	this->addChild(m_cadastrarButton);

	m_registerInfoLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f));
	this->addChild(m_registerInfoLabel);

	m_trackTitleLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f));
	this->addChild(m_trackTitleLabel);

	m_trackArtistLabel = new Label(" ", ImColor(0.7f, 0.7f, 0.7f));
	this->addChild(m_trackArtistLabel);

	m_playPauseButton = new Button(">");
	m_playPauseButton->setCallback([this]() { onPlayPauseClicked(); });
	this->addChild(m_playPauseButton);

	m_nextButton = new Button(">>");
	m_nextButton->setCallback([this]() { onNextClicked(); });
	this->addChild(m_nextButton);

	m_prevButton = new Button("<<");
	m_prevButton->setCallback([this]() { onPrevClicked(); });
	this->addChild(m_prevButton);
}

void LoginScreen::show(LoginScreenMode mode)
{
	m_mode = mode;
	m_errorMessage.clear();
	m_errorLabel->setText(" ");
	m_passwordBox->clear();

	m_titleLabel->setText(Settings::nick());

	bool isLogin = (mode == LoginScreenMode::Login);
	m_passwordBox->setVisible(isLogin);
	m_entrarButton->setVisible(isLogin);
	m_errorLabel->setVisible(isLogin);
	m_registerInfoLabel->setVisible(!isLogin);
	m_registerInfoLabel->setText("Sua conta ainda nao existe. Registre-se pelo nosso Discord: discord.gg/nativorp");

	// Cadastrar-se fica disponivel nos dois modos (a pedido - mesmo com
	// conta existente, o player pode querer ver a instrucao de novo).
	m_cadastrarButton->setVisible(true);

	setVisible(true);
	performLayout();

	if (!m_playing) {
		playTrack(0);
	}
}

void LoginScreen::hide()
{
	setVisible(false);
	stopMusic();
}

void LoginScreen::onLoginResult(bool success, const std::string& message)
{
	if (success) {
		hide();
		return;
	}

	m_errorMessage = message;
	m_errorLabel->setText(message);
	m_passwordBox->clear();
}

void LoginScreen::onEntrarClicked()
{
	const std::string& senha = m_passwordBox->text();
	if (senha.empty()) {
		m_errorLabel->setText("Digite uma senha.");
		return;
	}

	// Envia RPC_LOGIN_ATTEMPT (225) - ver Login_SendAttempt em ScriptRPC.cpp.
	extern void Login_SendAttempt(const std::string& senha);
	Login_SendAttempt(senha);
}

void LoginScreen::onCadastrarClicked()
{
	// So troca a exibicao local pro aviso do Discord - nao manda nada pro
	// servidor (o registro em si continua manual, via !registrar no
	// Discord). Nao mexe no m_mode "de verdade" (o servidor continua
	// achando que estamos no modo que ele mandou), so troca o que aparece
	// na tela - se o player voltar pra digitar a senha, os campos de login
	// continuam do jeito que estavam.
	m_registerInfoLabel->setText("Registre-se pelo nosso Discord: discord.gg/nativorp");
	m_registerInfoLabel->setVisible(true);
}

void LoginScreen::onPlayPauseClicked()
{
	if (!m_musicStream) {
		playTrack(m_trackIndex);
		return;
	}

	if (m_playing) {
		BASS_ChannelPause(m_musicStream);
		m_playPauseButton->setCaption(">");
	}
	else {
		BASS_ChannelPlay(m_musicStream, false);
		m_playPauseButton->setCaption("||");
	}
	m_playing = !m_playing;
}

void LoginScreen::onNextClicked()
{
	playTrack((m_trackIndex + 1) % 3);
}

void LoginScreen::onPrevClicked()
{
	playTrack((m_trackIndex + 2) % 3); // -1 sem dar negativo
}

void LoginScreen::playTrack(int index)
{
	stopMusic();
	m_trackIndex = index;

	const LoginTrack& track = kLoginTracks[index];

	m_trackTitleLabel->setText(track.title);
	m_trackArtistLabel->setText(track.artist);

	// Capa - RwRaster* via AssetImageLoader (ja tem cache proprio, chamada
	// repetida com o mesmo caminho e' barata).
	// Nao guardamos o ponteiro por track porque o proprio LoadIconTextureFromAsset
	// ja cacheia; so pedimos de novo aqui e usamos no draw().

	AAssetManager* mgr = AssetImageLoader_GetAssetManager();
	if (mgr == nullptr) {
		spdlog::warn("LoginScreen::playTrack: AssetManager ainda nao registrado");
		return;
	}

	AAsset* asset = AAssetManager_open(mgr, track.assetMp3, AASSET_MODE_BUFFER);
	if (asset == nullptr) {
		spdlog::warn("LoginScreen::playTrack: nao encontrei \"{}\" (confira assets/{})", track.assetMp3, track.assetMp3);
		return;
	}

	off_t length = AAsset_getLength(asset);
	const void* buffer = AAsset_getBuffer(asset);
	if (buffer == nullptr || length <= 0) {
		AAsset_close(asset);
		return;
	}

	// Copia pra um buffer proprio que fica vivo enquanto o stream tocar -
	// BASS le sob demanda da memoria, o AAsset original pode fechar.
	m_musicBuffer.assign((unsigned char*) buffer, (unsigned char*) buffer + length);
	AAsset_close(asset);

	m_musicStream = BASS_StreamCreateFile(TRUE, m_musicBuffer.data(), 0, m_musicBuffer.size(), BASS_SAMPLE_LOOP);
	if (m_musicStream == 0) {
		spdlog::warn("LoginScreen::playTrack: BASS_StreamCreateFile falhou pra \"{}\" (erro {})", track.assetMp3, BASS_ErrorGetCode());
		m_musicBuffer.clear();
		return;
	}

	BASS_ChannelPlay(m_musicStream, false);
	m_playing = true;
	m_playPauseButton->setCaption("||");
}

void LoginScreen::stopMusic()
{
	if (m_musicStream) {
		BASS_ChannelStop(m_musicStream);
		BASS_StreamFree(m_musicStream);
		m_musicStream = 0;
	}
	m_musicBuffer.clear();
	m_playing = false;
}

void LoginScreen::performLayout()
{
	// Layout manual (mesmo espirito do StatusHUD/SpeedometerHUD) - o
	// mockup pede posicoes bem especificas (logo+form a esquerda, player
	// de musica... por ora deixei tudo dentro do mesmo card, dá pra
	// reposicionar fino depois de ver rodando de verdade no aparelho).
	float pad = UISettings::padding();
	float w = width();
	float h = height();

	m_titleLabel->performLayout();
	m_titleLabel->setPosition(ImVec2(pad, h * 0.30f));

	m_passwordBox->setSize(ImVec2(w * 0.30f, 48.0f));
	m_passwordBox->setPosition(ImVec2(pad, h * 0.30f + 40.0f));
	m_passwordBox->performLayout();

	m_errorLabel->performLayout();
	m_errorLabel->setPosition(ImVec2(pad, h * 0.30f + 96.0f));

	m_entrarButton->setSize(ImVec2(140.0f, 44.0f));
	m_entrarButton->setPosition(ImVec2(pad, h * 0.30f + 130.0f));
	m_entrarButton->performLayout();

	m_cadastrarButton->setSize(ImVec2(140.0f, 44.0f));
	m_cadastrarButton->setPosition(ImVec2(pad + 156.0f, h * 0.30f + 130.0f));
	m_cadastrarButton->performLayout();

	m_registerInfoLabel->performLayout();
	m_registerInfoLabel->setPosition(ImVec2(pad, h * 0.30f + 40.0f));

	// Mini player de musica, canto superior esquerdo (mesma area do
	// mockup que voce mandou, "Coracao Partido / MC Ryan SP").
	float playerX = pad;
	float playerY = pad;

	m_trackTitleLabel->performLayout();
	m_trackTitleLabel->setPosition(ImVec2(playerX + 90.0f, playerY + 10.0f));

	m_trackArtistLabel->performLayout();
	m_trackArtistLabel->setPosition(ImVec2(playerX + 90.0f, playerY + 34.0f));

	m_prevButton->setSize(ImVec2(48.0f, 40.0f));
	m_prevButton->setPosition(ImVec2(playerX + 90.0f, playerY + 64.0f));
	m_prevButton->performLayout();

	m_playPauseButton->setSize(ImVec2(48.0f, 40.0f));
	m_playPauseButton->setPosition(ImVec2(playerX + 148.0f, playerY + 64.0f));
	m_playPauseButton->performLayout();

	m_nextButton->setSize(ImVec2(48.0f, 40.0f));
	m_nextButton->setPosition(ImVec2(playerX + 206.0f, playerY + 64.0f));
	m_nextButton->performLayout();

	Widget::performLayout();
}

void LoginScreen::draw(ImGuiRenderer* renderer)
{
	if (!visible()) return;

	if (m_bgTexture == nullptr) {
		m_bgTexture = LoadIconTextureFromAsset("login/login_fundo.jpg");
	}
	if (m_logoTexture == nullptr) {
		m_logoTexture = LoadIconTextureFromAsset("login/login_logo.png");
	}

	if (m_bgTexture != nullptr) {
		renderer->drawImage(absolutePosition(), absolutePosition() + size(), (ImTextureID) m_bgTexture);
	}
	else {
		// Sem o fundo carregado ainda (ou nao encontrado) - preenche solido
		// pra nao ficar transparente mostrando o jogo atras.
		renderer->drawRect(absolutePosition(), absolutePosition() + size(), ImColor(0.05f, 0.05f, 0.07f, 1.0f), true);
	}

	if (m_logoTexture != nullptr) {
		ImVec2 logoPos = absolutePosition() + ImVec2((width() - 260.0f) / 2.0f, height() * 0.10f);
		renderer->drawImage(logoPos, logoPos + ImVec2(260.0f, 114.5f), (ImTextureID) m_logoTexture);
	}

	// Capa da faixa atual
	const LoginTrack& track = kLoginTracks[m_trackIndex];
	void* cover = LoadIconTextureFromAsset(track.assetCover);
	if (cover != nullptr) {
		ImVec2 coverPos = absolutePosition() + ImVec2(UISettings::padding(), UISettings::padding());
		renderer->drawImage(coverPos, coverPos + ImVec2(72.0f, 72.0f), (ImTextureID) cover);
	}

	Widget::draw(renderer);
}
