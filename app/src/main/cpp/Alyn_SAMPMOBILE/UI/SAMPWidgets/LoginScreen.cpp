#include <algorithm>
#include <android/asset_manager.h>
#include <RenderWare/rw.h>

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

// Azul de destaque do app (mesmo tom usado no contorno do Button padrao -
// UI/Widgets/Button.cpp - pra manter a identidade visual consistente).
static const ImColor kAccentColor(0x32, 0x91, 0xF5);

// ============================ helpers de textura ============================
//
// O RenderWare deste client (GameSA/include/RenderWare/rwcore.h, struct
// RwRaster) arredonda a textura pra potencia de 2 ao carregar um PNG/JPG
// NPOT (ver RwImageFindRasterFormat, chamado por AssetImageLoader.cpp) e so
// preenche o canto superior-esquerdo com o conteudo real:
//   raster->width/height       = tamanho da textura (arredondado, ex 1024x512)
//   raster->originalWidth/Height = tamanho real da imagem (ex 753x331)
// drawImage() com UV padrao (0,0)-(1,1) cobre a textura INTEIRA, entao a
// imagem real acaba espremida/cortada num canto - esse e' o motivo da logo
// (e do fundo, e das capas) aparecerem cortadas/na posicao errada. As
// funcoes abaixo calculam o recorte de UV certo pra cada caso de uso.

static ImVec2 RealTextureSize(void* texture)
{
	if (texture == nullptr) return ImVec2(0.0f, 0.0f);
	RwRaster* raster = (RwRaster*) texture;
	float w = raster->originalWidth > 0 ? (float) raster->originalWidth : (float) raster->width;
	float h = raster->originalHeight > 0 ? (float) raster->originalHeight : (float) raster->height;
	return ImVec2(w, h);
}

static ImVec2 FullContentUVMax(void* texture)
{
	if (texture == nullptr) return ImVec2(1.0f, 1.0f);
	RwRaster* raster = (RwRaster*) texture;
	if (raster->width <= 0 || raster->height <= 0) return ImVec2(1.0f, 1.0f);
	ImVec2 real = RealTextureSize(texture);
	return ImVec2(real.x / (float) raster->width, real.y / (float) raster->height);
}

// "Cover": preenche o retangulo alvo cortando o excesso, sem distorcer -
// usado pro fundo (cobre a tela toda) e pra capa do album (cobre o quadrado).
static void DrawImageCover(ImGuiRenderer* renderer, void* texture, const ImVec2& pos, const ImVec2& targetSize)
{
	if (texture == nullptr) return;

	ImVec2 real = RealTextureSize(texture);
	if (real.x <= 0.0f || real.y <= 0.0f || targetSize.x <= 0.0f || targetSize.y <= 0.0f) return;

	ImVec2 fullUv = FullContentUVMax(texture);

	float srcAspect = real.x / real.y;
	float dstAspect = targetSize.x / targetSize.y;

	ImVec2 uvMin(0.0f, 0.0f);
	ImVec2 uvMax = fullUv;

	if (srcAspect > dstAspect) {
		// fonte mais larga que o alvo - corta as laterais, mantem a altura toda
		float visibleFrac = dstAspect / srcAspect;
		float cut = (fullUv.x - fullUv.x * visibleFrac) * 0.5f;
		uvMin.x = cut;
		uvMax.x = fullUv.x - cut;
	}
	else {
		// fonte mais alta (ou igual) que o alvo - corta cima/baixo
		float visibleFrac = srcAspect / dstAspect;
		float cut = (fullUv.y - fullUv.y * visibleFrac) * 0.5f;
		uvMin.y = cut;
		uvMax.y = fullUv.y - cut;
	}

	renderer->drawImage(pos, pos + targetSize, (ImTextureID) texture, uvMin, uvMax);
}

// "Contain": encaixa dentro de uma caixa maxW x maxH preservando o aspecto,
// SEM cortar - usado pra logo (nao pode perder pedaco), centralizada em
// torno de "center".
static void DrawImageContain(ImGuiRenderer* renderer, void* texture, const ImVec2& center, float maxW, float maxH)
{
	if (texture == nullptr) return;

	ImVec2 real = RealTextureSize(texture);
	if (real.x <= 0.0f || real.y <= 0.0f) return;

	float scale = std::min(maxW / real.x, maxH / real.y);
	ImVec2 drawSize(real.x * scale, real.y * scale);
	ImVec2 pos = center - drawSize * 0.5f;

	renderer->drawImage(pos, pos + drawSize, (ImTextureID) texture, ImVec2(0.0f, 0.0f), FullContentUVMax(texture));
}

// Icone simples de usuario (cabeca + ombros) - desenhado com as primitivas
// que ja existem no renderer, sem depender de nenhum asset novo.
static void DrawUserIcon(ImGuiRenderer* renderer, const ImVec2& center, float size, const ImColor& color)
{
	renderer->drawCircleFilled(ImVec2(center.x, center.y - size * 0.30f), size * 0.32f, color);

	ImVec2 a(center.x - size * 0.55f, center.y + size * 0.55f);
	ImVec2 b(center.x + size * 0.55f, center.y + size * 0.55f);
	ImVec2 c(center.x, center.y - size * 0.05f);
	renderer->drawTriangle(a, b, c, color, true);
}

// Icone simples de cadeado (corpo + arco do gancho, usando drawArc que ja
// existe no renderer pro velocimetro).
static void DrawLockIcon(ImGuiRenderer* renderer, const ImVec2& center, float size, const ImColor& color)
{
	float bodyW = size * 1.1f;
	float bodyH = size * 0.85f;
	ImVec2 bodyMin(center.x - bodyW * 0.5f, center.y - bodyH * 0.15f);
	ImVec2 bodyMax(center.x + bodyW * 0.5f, center.y + bodyH * 0.85f);
	renderer->drawRoundedRectFilled(bodyMin, bodyMax, size * 0.15f, color);

	renderer->drawArc(ImVec2(center.x, center.y - bodyH * 0.15f), size * 0.42f, size * 0.16f, color, 180.0f, 360.0f);
}

// ============================== PasswordBox ==============================

PasswordBox::PasswordBox()
{
	m_maskLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() * 0.62f);
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
			height() * 0.85f, // abre espaco pro icone de cadeado a esquerda
			(height() - m_maskLabel->height()) / 2
	));
}

void PasswordBox::draw(ImGuiRenderer* renderer)
{
	float rounding = height() * 0.35f;
	renderer->drawRoundedRectFilled(absolutePosition(), absolutePosition() + size(),
			rounding, ImColor(1.0f, 1.0f, 1.0f, 0.08f));
	renderer->drawRect(
			absolutePosition() + ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			(absolutePosition() + size()) - ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			ImColor(1.0f, 1.0f, 1.0f, 0.20f), false, UISettings::outlineSize());

	DrawLockIcon(renderer, absolutePosition() + ImVec2(height() * 0.5f, height() * 0.5f), height() * 0.22f,
			ImColor(1.0f, 1.0f, 1.0f, 0.55f));

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

// ============================== ReadOnlyField ==============================

ReadOnlyField::ReadOnlyField(const std::string& text)
{
	m_label = new Label(text, ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() * 0.62f);
	this->addChild(m_label);
}

void ReadOnlyField::setText(const std::string& text)
{
	m_label->setText(text);
}

void ReadOnlyField::performLayout()
{
	m_label->performLayout();
	m_label->setPosition(ImVec2(
			height() * 0.85f,
			(height() - m_label->height()) / 2
	));
}

void ReadOnlyField::draw(ImGuiRenderer* renderer)
{
	float rounding = height() * 0.35f;
	renderer->drawRoundedRectFilled(absolutePosition(), absolutePosition() + size(),
			rounding, ImColor(1.0f, 1.0f, 1.0f, 0.08f));
	renderer->drawRect(
			absolutePosition() + ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			(absolutePosition() + size()) - ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
			ImColor(1.0f, 1.0f, 1.0f, 0.20f), false, UISettings::outlineSize());

	DrawUserIcon(renderer, absolutePosition() + ImVec2(height() * 0.5f, height() * 0.5f), height() * 0.22f,
			ImColor(1.0f, 1.0f, 1.0f, 0.55f));

	Widget::draw(renderer);
}

// ============================== PillButton ==============================

PillButton::PillButton(const std::string& caption, bool primary)
	: Button(caption)
	, m_primary(primary)
{
	setCaptionColor(primary ? ImColor(1.0f, 1.0f, 1.0f) : ImColor(0.90f, 0.90f, 0.90f));
}

void PillButton::draw(ImGuiRenderer* renderer)
{
	float rounding = height() * 0.5f;

	if (m_primary) {
		renderer->drawRoundedRectFilled(absolutePosition(), absolutePosition() + size(),
				rounding, focused() ? ImColor(0x28, 0x74, 0xC7) : kAccentColor);
	}
	else {
		renderer->drawRoundedRectFilled(absolutePosition(), absolutePosition() + size(),
				rounding, ImColor(1.0f, 1.0f, 1.0f, focused() ? 0.20f : 0.10f));
		renderer->drawRect(
				absolutePosition() + ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
				(absolutePosition() + size()) - ImVec2(UISettings::outlineSize(), UISettings::outlineSize()),
				ImColor(1.0f, 1.0f, 1.0f, 0.35f), false, UISettings::outlineSize());
	}

	// Widget::draw (nao Button::draw) - desenha so os filhos (o Label da
	// legenda), sem cair no retangulo azul-contornado generico do Button.
	Widget::draw(renderer);
}

// ============================== PlayerIconButton ==============================

PlayerIconButton::PlayerIconButton(PlayerIconKind kind)
	: Button(" ")
	, m_kind(kind)
{
}

void PlayerIconButton::draw(ImGuiRenderer* renderer)
{
	ImVec2 c = absolutePosition() + size() * 0.5f;
	float r = std::min(size().x, size().y) * 0.5f;
	bool isPlayPause = (m_kind == PlayerIconKind::Play || m_kind == PlayerIconKind::Pause);

	if (isPlayPause) {
		renderer->drawCircleFilled(c, r, focused() ? ImColor(0x28, 0x74, 0xC7) : kAccentColor);
	}
	// anterior/proximo ficam sem nenhuma caixa/circulo de fundo - so o
	// icone, igual a referencia (a caixa contornada e' o visual "feio" que
	// estava sendo reciclado do Button padrao).

	const ImColor iconColor(1.0f, 1.0f, 1.0f, isPlayPause ? 1.0f : 0.85f);
	float s = r * (isPlayPause ? 0.5f : 0.62f);

	switch (m_kind) {
		case PlayerIconKind::Play:
			renderer->drawTriangle(
					ImVec2(c.x - s * 0.55f, c.y - s),
					ImVec2(c.x - s * 0.55f, c.y + s),
					ImVec2(c.x + s * 0.85f, c.y),
					iconColor, true);
			break;

		case PlayerIconKind::Pause: {
			float barW = s * 0.42f;
			renderer->drawRect(ImVec2(c.x - s * 0.65f, c.y - s), ImVec2(c.x - s * 0.65f + barW, c.y + s), iconColor, true);
			renderer->drawRect(ImVec2(c.x + s * 0.23f, c.y - s), ImVec2(c.x + s * 0.23f + barW, c.y + s), iconColor, true);
			break;
		}

		case PlayerIconKind::Prev:
			renderer->drawTriangle(ImVec2(c.x + s * 0.05f, c.y - s * 0.85f), ImVec2(c.x + s * 0.05f, c.y + s * 0.85f), ImVec2(c.x - s * 0.75f, c.y), iconColor, true);
			renderer->drawTriangle(ImVec2(c.x + s * 0.85f, c.y - s * 0.85f), ImVec2(c.x + s * 0.85f, c.y + s * 0.85f), ImVec2(c.x + s * 0.05f, c.y), iconColor, true);
			break;

		case PlayerIconKind::Next:
			renderer->drawTriangle(ImVec2(c.x - s * 0.85f, c.y - s * 0.85f), ImVec2(c.x - s * 0.85f, c.y + s * 0.85f), ImVec2(c.x - s * 0.05f, c.y), iconColor, true);
			renderer->drawTriangle(ImVec2(c.x - s * 0.05f, c.y - s * 0.85f), ImVec2(c.x - s * 0.05f, c.y + s * 0.85f), ImVec2(c.x + s * 0.75f, c.y), iconColor, true);
			break;
	}

	Widget::draw(renderer);
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
	m_welcomeLabel = new Label("Bem-vindo, jogador caro!", ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() * 0.9f);
	this->addChild(m_welcomeLabel);

	m_subtitleLabel = new Label("Para acessar o servidor, faca seu login abaixo", ImColor(0.75f, 0.75f, 0.78f), false, UISettings::fontSize() * 0.55f);
	this->addChild(m_subtitleLabel);

	m_usernameField = new ReadOnlyField(" ");
	this->addChild(m_usernameField);

	m_passwordBox = new PasswordBox();
	this->addChild(m_passwordBox);

	m_errorLabel = new Label(" ", ImColor(0.906f, 0.298f, 0.235f)); // vermelho, mesma familia do ToastCategory_Error
	this->addChild(m_errorLabel);

	m_entrarButton = new PillButton("Entrar", true);
	m_entrarButton->setCallback([this]() { onEntrarClicked(); });
	this->addChild(m_entrarButton);

	m_cadastrarButton = new PillButton("Cadastrar-se", false);
	m_cadastrarButton->setCallback([this]() { onCadastrarClicked(); });
	this->addChild(m_cadastrarButton);

	m_registerInfoLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f));
	this->addChild(m_registerInfoLabel);

	m_trackTitleLabel = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() * 0.6f);
	this->addChild(m_trackTitleLabel);

	m_trackArtistLabel = new Label(" ", ImColor(0.7f, 0.7f, 0.7f), false, UISettings::fontSize() * 0.5f);
	this->addChild(m_trackArtistLabel);

	m_playPauseButton = new PlayerIconButton(PlayerIconKind::Play);
	m_playPauseButton->setCallback([this]() { onPlayPauseClicked(); });
	this->addChild(m_playPauseButton);

	m_nextButton = new PlayerIconButton(PlayerIconKind::Next);
	m_nextButton->setCallback([this]() { onNextClicked(); });
	this->addChild(m_nextButton);

	m_prevButton = new PlayerIconButton(PlayerIconKind::Prev);
	m_prevButton->setCallback([this]() { onPrevClicked(); });
	this->addChild(m_prevButton);

	m_musicProgressBar = new ProgressBar(ImColor(1.0f, 1.0f, 1.0f, 0.20f), kAccentColor);
	this->addChild(m_musicProgressBar);
}

void LoginScreen::show(LoginScreenMode mode)
{
	m_mode = mode;
	m_errorMessage.clear();
	m_errorLabel->setText(" ");
	m_passwordBox->clear();

	m_usernameField->setText(Settings::nick());

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
		m_playPauseButton->setKind(PlayerIconKind::Play);
	}
	else {
		BASS_ChannelPlay(m_musicStream, false);
		m_playPauseButton->setKind(PlayerIconKind::Pause);
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
	m_playPauseButton->setKind(PlayerIconKind::Pause);
	m_musicProgressBar->setValue(0.0f);
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
	m_musicProgressBar->setValue(0.0f);
}

void LoginScreen::performLayout()
{
	// Layout manual (mesmo espirito do StatusHUD/SpeedometerHUD) - card de
	// login centralizado na tela, mini player de musica no canto superior
	// esquerdo (mesma posicao da referencia "Coracao Partido / MC Ryan SP").
	float pad = UISettings::padding();
	float w = width();
	float h = height();

	// -------- Logo (desenhada direto em draw(), so calculamos aqui pra
	// saber onde o bloco de texto/campos comeca embaixo dela) --------
	float logoMaxW = std::min(320.0f, w * 0.24f);
	float logoMaxH = logoMaxW * 0.6f;
	float logoCenterY = h * 0.145f;

	// -------- Bloco central (welcome + subtitulo + campos + botoes) --------
	float fieldW = std::min(420.0f, w * 0.30f);
	float fieldH = 52.0f;
	float cardX = (w - fieldW) / 2.0f;
	float blockY = logoCenterY + logoMaxH * 0.5f + 34.0f;

	m_welcomeLabel->performLayout();
	m_welcomeLabel->setPosition(ImVec2((w - m_welcomeLabel->width()) / 2.0f, blockY));

	m_subtitleLabel->performLayout();
	m_subtitleLabel->setPosition(ImVec2((w - m_subtitleLabel->width()) / 2.0f, blockY + m_welcomeLabel->height() + 6.0f));

	float fieldsY = blockY + m_welcomeLabel->height() + m_subtitleLabel->height() + 26.0f;

	m_usernameField->setSize(ImVec2(fieldW, fieldH));
	m_usernameField->setPosition(ImVec2(cardX, fieldsY));
	m_usernameField->performLayout();

	m_passwordBox->setSize(ImVec2(fieldW, fieldH));
	m_passwordBox->setPosition(ImVec2(cardX, fieldsY + fieldH + 14.0f));
	m_passwordBox->performLayout();

	m_registerInfoLabel->performLayout();
	m_registerInfoLabel->setPosition(ImVec2(cardX, fieldsY));

	float buttonsY = fieldsY + fieldH + 14.0f + fieldH + 22.0f;
	float buttonH = 46.0f;
	float buttonGap = 12.0f;
	float buttonW = (fieldW - buttonGap) / 2.0f;

	// setFixedSize (nao so setSize!) e' o que garante que o botao
	// realmente fica com esse tamanho - Button::performLayout() sempre
	// recalcula o tamanho a partir do texto da legenda (ver Button.cpp),
	// entao um setSize() sozinho e' sobrescrito na hora seguinte. Fixando
	// min=max=tamanho, o setSize() interno do Button fica preso nesse
	// valor (Widget::setSize ja faz esse clamp). Era esse o motivo dos
	// botoes saindo do tamanho/posicao esperados.
	m_entrarButton->setFixedSize(ImVec2(buttonW, buttonH));
	m_entrarButton->setPosition(ImVec2(cardX, buttonsY));
	m_entrarButton->performLayout();

	m_cadastrarButton->setFixedSize(ImVec2(buttonW, buttonH));
	m_cadastrarButton->setPosition(ImVec2(cardX + buttonW + buttonGap, buttonsY));
	m_cadastrarButton->performLayout();

	m_errorLabel->performLayout();
	m_errorLabel->setPosition(ImVec2(cardX, buttonsY + buttonH + 12.0f));

	// -------- Mini player de musica, canto superior esquerdo --------
	float playerX = pad;
	float playerY = pad;
	float coverSize = 64.0f;
	float textX = playerX + coverSize + 12.0f;

	m_trackTitleLabel->performLayout();
	m_trackTitleLabel->setPosition(ImVec2(textX, playerY));

	m_trackArtistLabel->performLayout();
	m_trackArtistLabel->setPosition(ImVec2(textX, playerY + m_trackTitleLabel->height() + 2.0f));

	float controlsY = playerY + m_trackTitleLabel->height() + m_trackArtistLabel->height() + 10.0f;

	m_prevButton->setFixedSize(ImVec2(30.0f, 30.0f));
	m_prevButton->setPosition(ImVec2(textX, controlsY + 5.0f));
	m_prevButton->performLayout();

	m_playPauseButton->setFixedSize(ImVec2(40.0f, 40.0f));
	m_playPauseButton->setPosition(ImVec2(textX + 38.0f, controlsY));
	m_playPauseButton->performLayout();

	m_nextButton->setFixedSize(ImVec2(30.0f, 30.0f));
	m_nextButton->setPosition(ImVec2(textX + 38.0f + 48.0f, controlsY + 5.0f));
	m_nextButton->performLayout();

	float progressY = controlsY + 40.0f + 10.0f;
	m_musicProgressBar->setSize(ImVec2(220.0f, 4.0f));
	m_musicProgressBar->setPosition(ImVec2(textX, progressY));

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
		DrawImageCover(renderer, m_bgTexture, absolutePosition(), size());
		// Escurece por cima pra garantir contraste do texto/campos, igual
		// a referencia (a foto sozinha, sem escurecer, deixa o texto branco
		// dificil de ler dependendo da area).
		renderer->drawRect(absolutePosition(), absolutePosition() + size(), ImColor(0.0f, 0.0f, 0.0f, 0.40f), true);
	}
	else {
		// Sem o fundo carregado ainda (ou nao encontrado) - preenche solido
		// pra nao ficar transparente mostrando o jogo atras.
		renderer->drawRect(absolutePosition(), absolutePosition() + size(), ImColor(0.05f, 0.05f, 0.07f, 1.0f), true);
	}

	if (m_logoTexture != nullptr) {
		float logoMaxW = std::min(320.0f, width() * 0.24f);
		float logoMaxH = logoMaxW * 0.6f;
		ImVec2 logoCenter = absolutePosition() + ImVec2(width() * 0.5f, height() * 0.145f);
		DrawImageContain(renderer, m_logoTexture, logoCenter, logoMaxW, logoMaxH);
	}

	// Capa da faixa atual (recorte "cover" - preenche o quadrado 64x64 sem
	// distorcer, cortando o excesso ao inves de espremer a imagem toda).
	const LoginTrack& track = kLoginTracks[m_trackIndex];
	void* cover = LoadIconTextureFromAsset(track.assetCover);
	if (cover != nullptr) {
		ImVec2 coverPos = absolutePosition() + ImVec2(UISettings::padding(), UISettings::padding());
		DrawImageCover(renderer, cover, coverPos, ImVec2(64.0f, 64.0f));
	}

	// Atualiza o preenchimento da barra de progresso com a posicao real da
	// faixa antes do Widget::draw() cascatear pros filhos (a ProgressBar
	// so sabe desenhar o valor que ja estiver setado nela via setValue()).
	if (m_musicStream != 0) {
		uint64_t posBytes = BASS_ChannelGetPosition ? BASS_ChannelGetPosition(m_musicStream, BASS_POS_BYTE) : 0;
		uint64_t lenBytes = BASS_ChannelGetLength ? BASS_ChannelGetLength(m_musicStream, BASS_POS_BYTE) : 0;
		float percent = (lenBytes > 0) ? (float) posBytes / (float) lenBytes : 0.0f;
		if (percent < 0.0f) percent = 0.0f;
		if (percent > 1.0f) percent = 1.0f;
		m_musicProgressBar->setValue(percent);
	}

	Widget::draw(renderer);
}
