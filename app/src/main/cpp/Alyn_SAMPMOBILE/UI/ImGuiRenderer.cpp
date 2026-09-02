#include <string>
#include <cmath>
#include "ImGuiRenderer.h"
#include "UISettings.h"

ImGuiRenderer::ImGuiRenderer(ImDrawList* draw_list, ImFont* font)
{
	m_drawList = draw_list;
	m_font = font;
}

void ImGuiRenderer::drawLine(const ImVec2& a, const ImVec2& b, const ImColor& color, float thickness)
{
	m_drawList->AddLine(a, b, color, thickness);
}

void ImGuiRenderer::drawRect(const ImVec2& a, const ImVec2& b, const ImColor& color, bool fill, float thickness)
{
	fill ? m_drawList->AddRectFilled(a, b, color) :
	m_drawList->AddRect(a, b, color, 0.0f, 15, thickness);
}

void ImGuiRenderer::drawRectFilledMulticolor(const ImVec2& a, const ImVec2& b, const ImColor& col_upr_left, const ImColor& col_upr_right, const ImColor& col_bot_right, const ImColor& col_bot_left)
{
	m_drawList->AddRectFilledMultiColor(a, b, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
}

void ImGuiRenderer::drawTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImColor& color, bool fill, float thickness)
{
	fill ? m_drawList->AddTriangleFilled(a, b, c, color) :
	m_drawList->AddTriangle(a, b, c, color, thickness);
}

void ImGuiRenderer::drawConvexPolyFilled(ImVec2* points, int num_points, const ImColor& color)
{
	m_drawList->AddConvexPolyFilled(points, num_points, color);
}

void ImGuiRenderer::drawText(const ImVec2& pos, const ImColor& color, const char* begin, const char* end, bool outline, float font_size, ImFont* font)
{
	ImFont* pFont = font == nullptr ? this->m_font : font;
	float sz_font = font_size == 0.0f ? pFont->FontSize : font_size;

	if (outline) {
		ImVec2 outlined = pos;
		float outlineSize = UISettings::outlineSize();

		// right
		outlined.x += outlineSize;
		m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x -= outlineSize;

		// left
		outlined.x -= outlineSize;
		m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x += outlineSize;

		// bottom
		outlined.y += outlineSize;
		m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.y -= outlineSize;

		// top
		outlined.y -= outlineSize;
		m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.y += outlineSize;

		// bottom-right
		outlined.x += outlineSize;
		outlined.y += outlineSize;
		//m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x -= outlineSize;
		outlined.y -= outlineSize;

		// bottom-left
		outlined.x -= outlineSize;
		outlined.y += outlineSize;
		//m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x += outlineSize;
		outlined.y -= outlineSize;

		// top-right
		outlined.x += outlineSize;
		outlined.y -= outlineSize;
		//m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x -= outlineSize;
		outlined.y += outlineSize;

		// top-left
		outlined.x -= outlineSize;
		outlined.y -= outlineSize;
		//m_drawList->AddText(pFont, sz_font, outlined, ImColor(0.0f, 0.0f, 0.0f), begin, end);
		outlined.x += outlineSize;
		outlined.y += outlineSize;
	}

	m_drawList->AddText(pFont, sz_font, pos, color, begin, end);
}

void ImGuiRenderer::drawText(const ImVec2& pos, const ImColor& color, const std::string& text, bool outlined, float font_size, ImFont* font)
{
	if (text.empty()) return;

	ImFont* pFont = font == nullptr ? this->m_font : font;

	float sz_font = font_size == 0.0f ? pFont->FontSize : font_size;

	const char* text_start = text.c_str();
	const char* text_cur = text.c_str();
	const char* text_end = text.c_str() + text.length();

	ImVec2 pos_cur = pos;
	ImColor color_cur = color;

	while (*text_cur) {
		if (*text_cur == '{' && ((&text_cur[7] < text_end) && text_cur[7] == '}')) {
			// print accumulated text
			if (text_cur != text_start) {
				drawText(pos_cur, color_cur, text_start, text_cur, outlined, sz_font, pFont);
				ImVec2 sz = calculateTextSize(text_start, text_cur, sz_font);
				pos_cur.x += sz.x;
			}

			// new colorcode
			ImVec4 col;
			if (processInlineHexColor(text_cur + 1, text_cur + 7, col)) {
				color_cur = col;
			}

			text_cur += 7;
			text_start = text_cur + 1;
		}
		else if (*text_cur == '\n') {
			// print accumulated text
			if (text_cur != text_start) {
				drawText(pos_cur, color_cur, text_start, text_cur, outlined, sz_font, pFont);
			}

			pos_cur.x = pos.x;
			pos_cur.y += sz_font;
			text_start = text_cur + 1;
		}
		else if (*text_cur == '\t') {
			// print accumulated text
			if (text_cur != text_start) {
				drawText(pos_cur, color_cur, text_start, text_cur, outlined, sz_font, pFont);
				ImVec2 sz = calculateTextSize(text_start, text_cur, sz_font);
				pos_cur.x += sz.x;
			}

			pos_cur.x += sz_font;
			text_start = text_cur + 1;
		}

		++text_cur;
	}

	if (text_cur != text_start) {
		drawText(pos_cur, color_cur, text_start, text_cur, outlined, sz_font, pFont);
	}
}

ImVec2 ImGuiRenderer::calculateTextSize(const std::string& text, float font_size)
{
	ImVec2 text_size = {0.0f, 0.0f};
	if (text.empty()) return text_size;

	ImVec2 cur_size = {0.0f, 0.0f};
	if (font_size == 0.0f) font_size = m_font->FontSize;

	const char* text_start = text.c_str();
	const char* text_cur = text.c_str();
	const char* text_end = text.c_str() + text.length();

	while (*text_cur) {
		if (*text_cur == '{' && ((&text_cur[7] < text_end) && text_cur[7] == '}')) {
			if (text_cur != text_start) {
				// ����� �� �����-����
				ImVec2 sz = calculateTextSize(text_start, text_cur, font_size);
				cur_size.x += sz.x;
				if (cur_size.y == 0.0f) cur_size.y = sz.y;
			}

			text_cur += 7;
			text_start = text_cur + 1;
		}
		else if (*text_cur == '\n') {
			if (text_cur != text_start) {
				// ����� �� \n
				ImVec2 sz = calculateTextSize(text_start, text_cur, font_size);
				cur_size.x += sz.x;
				if (cur_size.y == 0.0f) cur_size.y = sz.y;
			}

			// ��������� text_size
			text_size.x = ImMax(text_size.x, cur_size.x);
			cur_size.y += font_size;
			cur_size.x = 0.0f;

			text_start = text_cur + 1;
		}
		else if (*text_cur == '\t') {
			if (text_cur != text_start) {
				// ����� �� \t
				ImVec2 sz = calculateTextSize(text_start, text_cur, font_size);
				cur_size.x += sz.x;
				if (cur_size.y == 0.0f) cur_size.y = sz.y;
			}

			cur_size.x += font_size;
			text_start = text_cur + 1;
		}

		++text_cur;
	}

	if (text_cur != text_start) {
		// ����� ��� ��������������
		ImVec2 sz = calculateTextSize(text_start, text_cur, font_size);
		cur_size.x += sz.x;
		if (cur_size.y == 0.0f) cur_size.y = sz.y;
	}

	text_size = ImMax(text_size, cur_size);
	return text_size;
}

ImVec2 ImGuiRenderer::calculateTextSize(const char* begin, const char* end, float font_size)
{
	return m_font->CalcTextSizeA(font_size == 0.0f ? m_font->FontSize : font_size, FLT_MAX, 0.0f, begin, end);
}

bool ImGuiRenderer::processInlineHexColor(const char* start, const char* end, ImVec4& color)
{
	const int hexCount = (int) (end - start);
	if (hexCount == 6) {
		char hex[7];
		strncpy(hex, start, hexCount);
		hex[hexCount] = 0;

		unsigned int hexColor = 0;
		if (sscanf(hex, "%x", &hexColor) > 0) {
			color.x = static_cast<float>((hexColor & 0x00FF0000) >> 16) / 255.0f;
			color.y = static_cast<float>((hexColor & 0x0000FF00) >> 8) / 255.0f;
			color.z = static_cast<float>((hexColor & 0x000000FF)) / 255.0f;
			color.w = 1.0f;
			return true;
		}
	}

	return false;
}

void ImGuiRenderer::drawImage(const ImVec2& a, const ImVec2& b, ImTextureID texture)
{
	m_drawList->AddImage(texture, a, b);
}

static void GetHexagonPoints(const ImVec2& center, float radius, ImVec2 out[6])
{
	// Hexagono "pontudo em cima" (vertice no topo, nao lado plano),
	// comecando as -90 graus (topo) e andando no sentido horario.
	for (int i = 0; i < 6; ++i) {
		float angle = -IM_PI / 2.0f + (float) i * (IM_PI / 3.0f);
		out[i] = ImVec2(center.x + radius * cosf(angle), center.y + radius * sinf(angle));
	}
}

void ImGuiRenderer::drawHexagonFilled(const ImVec2& center, float radius, const ImColor& color)
{
	ImVec2 points[6];
	GetHexagonPoints(center, radius, points);
	drawConvexPolyFilled(points, 6, color);
}

void ImGuiRenderer::drawHexagonProgress(const ImVec2& center, float radius, float thickness, const ImColor& color, float percent)
{
	if (percent <= 0.0f) return;
	if (percent > 1.0f) percent = 1.0f;

	ImVec2 points[6];
	GetHexagonPoints(center, radius, points);

	// perimetro de um hexagono regular = 6 lados, cada lado tem o mesmo
	// comprimento que o raio (propriedade geometrica do hexagono regular).
	float sideLength = radius;
	float totalPerimeter = sideLength * 6.0f;
	float targetDistance = totalPerimeter * percent;

	float distanceSoFar = 0.0f;

	for (int i = 0; i < 6; ++i) {
		const ImVec2& p1 = points[i];
		const ImVec2& p2 = points[(i + 1) % 6];

		if (distanceSoFar >= targetDistance) break;

		float remaining = targetDistance - distanceSoFar;

		if (remaining >= sideLength) {
			// lado inteiro cabe dentro do percentual, desenha ele completo
			drawLine(p1, p2, color, thickness);
			distanceSoFar += sideLength;
		} else {
			// so uma parte desse lado cabe, desenha so ate o ponto exato
			float t = remaining / sideLength;
			ImVec2 partial = ImVec2(p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t);
			drawLine(p1, partial, color, thickness);
			distanceSoFar += remaining;
			break;
		}
	}
}

void ImGuiRenderer::drawArc(const ImVec2& center, float radius, float thickness, const ImColor& color, float angleMinDeg, float angleMaxDeg)
{
	if (angleMaxDeg <= angleMinDeg) return;

	float angleMin = angleMinDeg * (IM_PI / 180.0f);
	float angleMax = angleMaxDeg * (IM_PI / 180.0f);

	// mais segmentos pra arcos maiores, pra nao ficar poligonal
	int segments = ImClamp((int) (radius * 0.35f), 16, 64);

	m_drawList->PathArcTo(center, radius, angleMin, angleMax, segments);
	m_drawList->PathStroke(color, 0, thickness);
}

void ImGuiRenderer::pushClipRect(const ImVec2& min, const ImVec2& max, bool intersect)
{
	m_drawList->PushClipRect(min, max, intersect);
}

void ImGuiRenderer::popClipRect()
{
	m_drawList->PopClipRect();
}