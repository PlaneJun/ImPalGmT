#include "draw.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <windows.h>

draw* draw::s_instance_ = NULL;

draw* draw::get_instance()
{
	if (s_instance_ == NULL)
		s_instance_ = new draw();
	return s_instance_;
}

std::string draw::string_To_UTF8(const std::string& str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	wchar_t* pwBuf = new wchar_t[nwLen + 1];
	ZeroMemory(pwBuf, nwLen * 2 + 2);

	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = (int)(::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL));

	char* pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	std::string retStr(pBuf);

	delete[]pwBuf;
	delete[]pBuf;

	pwBuf = NULL;
	pBuf = NULL;

	return retStr;
}

void draw::draw_string_center(float x, float y, uint32_t color, const char* message, ...)
{
	char output[1024] = {};
	va_list args;
	va_start(args, message);
	vsprintf_s(output, message, args);
	va_end(args);

	std::string utf_8_1 = std::string(output);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);

	auto size = ImGui::CalcTextSize(utf_8_2.c_str());
	auto coord = ImVec2(x - size.x / 2, y);
	auto coord2 = ImVec2(coord.x + 1, coord.y + 1);
	auto coord_out = ImVec2{ coord.x - 1, coord.y - 1 };

	auto DrawList = ImGui::GetForegroundDrawList();

	DrawList->AddText(coord2, IM_COL32(0, 0, 0, 200), utf_8_2.c_str());
	DrawList->AddText(coord_out, IM_COL32(0, 0, 0, 200), utf_8_2.c_str());
	DrawList->AddText(coord, color, utf_8_2.c_str());
}
void draw::draw_string(float x, float y, uint32_t color, const char* message, ...)
{
	char output[1024] = {};
	va_list args;
	va_start(args, message);
	vsprintf_s(output, message, args);
	va_end(args);

	std::string utf_8_1 = std::string(output);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);

	auto coord = ImVec2(x, y);
	auto coord2 = ImVec2(coord.x + 1, coord.y + 1);
	auto coord_out = ImVec2{ coord.x - 1, coord.y - 1 };

	auto DrawList = ImGui::GetForegroundDrawList();

	DrawList->AddText(coord2, IM_COL32(0, 0, 0, 200), utf_8_2.c_str());
	DrawList->AddText(coord_out, IM_COL32(0, 0, 0, 200), utf_8_2.c_str());
	DrawList->AddText(coord, color, utf_8_2.c_str());
}
void draw::draw_line(float x1, float y1, float x2, float y2, uint32_t clr, float thickness)
{
	auto DrawList = ImGui::GetForegroundDrawList();

	DrawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), clr, thickness);
}
void draw::draw_rect(float x, float y, float w, float h, uint32_t clr, float width)
{
	auto DrawList = ImGui::GetForegroundDrawList();
	ImVec2 min = ImVec2(x, y);
	ImVec2 max = ImVec2(x + w, y + h);
	DrawList->AddRect(min, max, clr, 0.0F, -1, width);
}
bool draw::draw_filled_rect(const char* name, int w, int h, uint32_t color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(name);
	ImVec2 pos = window->DC.CursorPos;
	auto DrawList = ImGui::GetForegroundDrawList();

	const ImRect total_bb({ pos.x,pos.y },  ImVec2(pos.x+w, pos.y+h));
	//const ImRect frame_bb(total_bb.Min + ImVec2(0, label_size.y + 10), total_bb.Max);

	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	DrawList->AddRectFilled(total_bb.Min, total_bb.Max, color);
	return true;
}
void draw::draw_triangle_filled(float x, float y, float size, uint32_t col)
{
	auto DrawList = ImGui::GetForegroundDrawList();
	ImVec2 p1 = ImVec2(x, y);
	ImVec2 p2 = ImVec2(x + size, y);
	ImVec2 p3 = ImVec2(x + size / 2, y + size);
	DrawList->AddTriangleFilled(p1, p2, p3, col);
}
void draw::draw_circle(float x, float y, float radius, uint32_t clr, float numseg, float thickness)
{
	ImGuiIO& io = ImGui::GetIO();
	ImTextureID tex_id = io.Fonts->TexID;
	auto DrawList = ImGui::GetForegroundDrawList();
	DrawList->AddCircle(ImVec2(x, y), radius, clr, numseg, thickness);
}




