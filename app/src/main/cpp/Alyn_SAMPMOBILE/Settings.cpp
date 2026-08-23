#include "Settings.h"
#include "Client.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <sys/stat.h>

bool Settings::m_initialized = false;

// server
char Settings::m_host[64] = "127.0.0.1";
int Settings::m_port = 7777;
char Settings::m_pass[64] = "";

// settings
char Settings::m_nick[30] = "samp_player";
int Settings::m_sampversion = 0;
bool Settings::m_newinterface = false;
bool Settings::m_systemkeyboard = false;
bool Settings::m_timestamp = false;
bool Settings::m_fastconnect = false;
bool Settings::m_voice = false;
bool Settings::m_displayfps = false;
int Settings::m_fpslimit = 60;
int Settings::m_chatstrings = 5;
float Settings::m_fontsize = 26.0f;
ImVec2 Settings::m_chatpos = ImVec2(100.0f, 10.0f);
ImVec2 Settings::m_chatsize = ImVec2(400.0f, 150.0f);

void Settings::initialize()
{
	spdlog::info("Initializing settings...");

	if (m_initialized) {
		return;
	}

	if (Client::gameDir() == nullptr) {
		spdlog::error("Failed to initialize settings. (game storage null)");
		return;
	}

	char settings_json_path[0xFF];
	sprintf(settings_json_path, "%sSAMP/settings.json", Client::gameDir());

	FILE* settings_json_file = fopen(settings_json_path, "rb");
	if (settings_json_file == nullptr) {
		// Java normally creates this file when the user presses Connect.
		// Keep safe defaults if it is genuinely unavailable, but make the
		// failure explicit instead of pretending that the selected server was
		// loaded.
		spdlog::error("Failed to initialize settings. (settings file null): {}", settings_json_path);
		return;
	}

	char buffer[0xFFFF];
	rapidjson::FileReadStream fileReadStream(settings_json_file, buffer, sizeof(buffer));

	rapidjson::Document settings_json;
	settings_json.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(fileReadStream);
	if (settings_json.HasParseError() || !settings_json.IsObject() ||
		!settings_json.HasMember("client") || !settings_json["client"].IsObject()) {
		spdlog::error("Failed to initialize settings. (invalid JSON)");
		fclose(settings_json_file);
		return;
	}

	// server
	auto& client = settings_json["client"];
	if (!client.HasMember("server") || !client["server"].IsObject() ||
		!client.HasMember("settings") || !client["settings"].IsObject()) {
		spdlog::error("Failed to initialize settings. (missing client sections)");
		fclose(settings_json_file);
		return;
	}
	auto& server = client["server"];
	auto& settings = client["settings"];
	if (!server.HasMember("host") || !server["host"].IsString() ||
		!server.HasMember("port") || !server["port"].IsInt() ||
		!server.HasMember("password") || !server["password"].IsString() ||
		!settings.HasMember("nick_name") || !settings["nick_name"].IsString()) {
		spdlog::error("Failed to initialize settings. (missing server settings)");
		fclose(settings_json_file);
		return;
	}
	snprintf(m_host, sizeof(m_host), "%s", server["host"].GetString());
	m_port = server["port"].GetInt();
	snprintf(m_pass, sizeof(m_pass), "%s", server["password"].GetString());

	// settings
	snprintf(m_nick, sizeof(m_nick), "%s", settings["nick_name"].GetString());
	m_sampversion = settings.HasMember("samp_version") && settings["samp_version"].IsInt() ? settings["samp_version"].GetInt() : 0;
	// m_newinterface = settings_json["client"]["settings"]["new_interface"].GetBool();
	m_systemkeyboard = settings_json["client"]["settings"]["system_keyboard"].GetBool();
	m_timestamp = settings_json["client"]["settings"]["timestamp"].GetBool();
	m_fastconnect = settings_json["client"]["settings"]["fast_connect"].GetBool();
	m_voice = settings_json["client"]["settings"]["voice_chat"].GetBool();
	m_displayfps = settings_json["client"]["settings"]["display_fps"].GetBool();
	m_fpslimit = settings_json["client"]["settings"]["fps_limit"].GetInt();
	m_chatstrings = settings_json["client"]["settings"]["chat_strings"].GetInt();
	m_fontsize = settings_json["client"]["settings"]["font_size"].GetFloat();
	m_chatpos.x = settings_json["client"]["settings"]["chat_posx"].GetInt();
	m_chatpos.y = settings_json["client"]["settings"]["chat_posy"].GetInt();
	m_chatsize.x = settings_json["client"]["settings"]["chat_sizex"].GetInt();
	m_chatsize.y = settings_json["client"]["settings"]["chat_sizey"].GetInt();

	spdlog::info("host {}", m_host);
	spdlog::info("port {}", m_port);
	spdlog::info("password {}", m_pass);

	spdlog::info("nick_name {}", m_nick);
	spdlog::info("samp_version {}", m_sampversion);
	spdlog::info("new_interface {}", m_newinterface);
	spdlog::info("system_keyboard {}", m_systemkeyboard);
	spdlog::info("timestamp {}", m_timestamp);
	spdlog::info("fast_connect {}", m_fastconnect);
	spdlog::info("voice_chat {}", m_voice);
	spdlog::info("display_fps {}", m_displayfps);
	spdlog::info("fps_limit {}", m_fpslimit);
	spdlog::info("chat_strings {}", m_chatstrings);
	spdlog::info("font_size {}", m_fontsize);
	spdlog::info("chat_pos {} {}", m_chatpos.x, m_chatpos.y);
	spdlog::info("chat_size {} {}", m_chatsize.x, m_chatsize.y);

	fclose(settings_json_file);

	m_initialized = true;
}
