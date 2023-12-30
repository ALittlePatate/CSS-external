#include "Imgui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

#define ABS(X) ((X < 0) ? (-X) : (X))
#define BOX_OFFSET 20.f //out of my ass

typedef struct
{
	DWORD R;
	DWORD G;
	DWORD B;
	DWORD A;
}RGBA;

std::string string_To_UTF8(const std::string& str) {
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	wchar_t* pwBuf = new wchar_t[nwLen + 1];
	ZeroMemory(pwBuf, nwLen * 2 + 2);
	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
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

void DrawStrokeText(int x, int y, RGBA* color, const char* str) {
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x - 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x + 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}

void DrawNewText(int x, int y, RGBA* color, const char* str) {
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}

void DrawRect(int x, int y, int w, int h, RGBA* color, int thickness) {
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0, thickness);
}

void DrawEspBox2D(Vector3 feet, Vector3 head, RGBA* color, int thickness) {
	float t = feet.x - head.x;
	float height = feet.y - head.y;
	float pd = feet.x + t;

	ImVec4 boxColor(color->R / 255.0f, color->G / 255.0f, color->B / 255.0f, color->A / 255.0f);

    // Calculate box position and draw the rectangle
    ImVec2 boxMin(head.x - (height/2)/2, head.y);
    ImVec2 boxMax(pd + (height/2)/2, feet.y);

	auto drawlist = ImGui::GetOverlayDrawList();
    drawlist->AddRect(boxMin, boxMax, ImColor(boxColor), 0.0f, 0, thickness);
    drawlist->AddRect(ImVec2(boxMin.x - thickness, boxMin.y - thickness), ImVec2(boxMax.x + thickness, boxMax.y + thickness), ImColor(0, 0, 0, 255), 0.0f, 0, thickness);
    drawlist->AddRect(ImVec2(boxMin.x + thickness, boxMin.y + thickness), ImVec2(boxMax.x - thickness, boxMax.y - thickness), ImColor(0, 0, 0, 255), 0.0f, 0, thickness);
}

void DrawBones(DWORD addr, RGBA* color, int thickness) {
	Vector3 lastbonepos{};
	Vector3 pelvis_bonepos{};
	Vector3 neck_bonepos{};
	for (int i = 0; i <= (int)BoneID::RIGHT_HAND; i++) {
		if (i == (int)BoneID::LEFT_FOOT || i == (int)BoneID::RIGHT_FOOT) continue; //causing weird bug
		if (i > 18 && i < 28) continue;

		Vector3 bonepos = GetBonePos(addr, (BoneID)i);
		if (i == (int)BoneID::PELVIS) {
			lastbonepos = bonepos;
			pelvis_bonepos = bonepos;
		}
		else if (i == (int)BoneID::RIGHT_HIPS || i == (int)BoneID::STOMACH) lastbonepos = pelvis_bonepos;
		else if (i == (int)BoneID::NECK) neck_bonepos = bonepos;
		else if (i == (int)BoneID::LEFT_NECK) lastbonepos = neck_bonepos;
		else if (i == (int)BoneID::RIGHT_NECK) lastbonepos = neck_bonepos;

		Vector3 w2s_bonepos;
		if (!WorldToScreen(bonepos, w2s_bonepos)) {
			continue;
		}

		Vector3 w2s_lastbonepos;
		if (!WorldToScreen(lastbonepos, w2s_lastbonepos)) {
			continue;
		}

		ImGui::GetOverlayDrawList()->AddLine(ImVec2(w2s_lastbonepos.x, w2s_lastbonepos.y), ImVec2(w2s_bonepos.x, w2s_bonepos.y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);

		lastbonepos = bonepos;
	}
}

void DrawHealthBar(Vector3 feet, Vector3 head, int health) {
	float t = feet.x - head.x;
	float pd = feet.x + t;
	float h = feet.y - head.y;

    ImVec2 boxMin(head.x - (h/2)/2, head.y);
    ImVec2 boxMax(pd + (h/2)/2, feet.y);

	const ImVec2 barMin = ImVec2(boxMin.x - 5, boxMin.y);
    const ImVec2 barMax = ImVec2{boxMin.x - 2, boxMax.y};

	const float green = health * 2.f * 2.55f;
	const float greenClamped = (std::min)(255.f, green);
	const float red = (std::min)(255.f, 510 - green);

	const float height = ((barMax.y - barMin.y) * health) / 100.f;

	auto draw_list = ImGui::GetOverlayDrawList();
	draw_list->AddRectFilled(ImVec2(barMin.x, barMin.y - 1),
										 ImVec2(barMax.x, barMax.y),
										 IM_COL32(0, 0, 0, 255));
	draw_list->AddRectFilled(
		ImVec2{barMin.x + 1, barMax.y - (std::max)(1.f, height)},
		ImVec2{barMax.x - 1, barMax.y - 1},
		IM_COL32(red, greenClamped, 0, 255));
}

void DrawNameTag(Vector3 feet, Vector3 head, char* name) {
	float t = feet.x - head.x;
	float pd = feet.x + t;

    ImVec2 boxMin(head.x, head.y);
    ImVec2 boxMax(pd, feet.y);

	const ImVec2 textSize = ImGui::CalcTextSize(name);
	const ImVec2 textPos = ImFloor(
		{(boxMin.x + boxMax.x - textSize.x) / 2.f, boxMin.y - textSize.y - 2.f});

	auto draw_list = ImGui::GetOverlayDrawList();
	draw_list->AddText(ImVec2(textPos.x + 1.f, textPos.y + 1.f), IM_COL32(0, 0, 0, 255), name);
	draw_list->AddText(
		textPos, IM_COL32(255, 255, 255, 255), name);
}

void DrawFilledRect(int x, int y, int w, int h, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0);
}

void DrawCircleFilled(int x, int y, int radius, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}

void DrawCircle(int x, int y, int radius, RGBA* color, int segments = 0) {
	ImGui::GetOverlayDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments);
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color, float thickne) {
	ImGui::GetOverlayDrawList()->AddTriangle(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickne);
}

void DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}

void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness) {
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);
}

void DrawCornerBox(int x, int y, int w, int h, int borderPx, RGBA* color) {
	DrawFilledRect(x + borderPx, y, w / 3, borderPx, color);
	DrawFilledRect(x + w - w / 3 + borderPx, y, w / 3, borderPx, color);
	DrawFilledRect(x, y, borderPx, h / 3, color);
	DrawFilledRect(x, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
	DrawFilledRect(x + borderPx, y + h + borderPx, w / 3, borderPx, color);
	DrawFilledRect(x + w - w / 3 + borderPx, y + h + borderPx, w / 3, borderPx, color);
	DrawFilledRect(x + w + borderPx, y, borderPx, h / 3, color);
	DrawFilledRect(x + w + borderPx, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
}