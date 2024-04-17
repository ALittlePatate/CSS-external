#include <valve-bsp-parser/bsp_parser.hpp>
#include "xorstr.hpp"
#include "Functions.h"
#include "Memory.h"
#include "Overlay.h"

LPCSTR TargetProcess = "hl2.exe";
bool ShowMenu = false;
bool Unhook = false;
bool ally_esp = false;
bool enemy_esp = true;
bool ally_box = false;
bool enemy_box = true;
bool ally_skeleton = false;
bool enemy_skeleton = true;
bool ally_esp_health_bar = false;
bool enemy_esp_health_bar = true;
bool ally_name = false;
bool enemy_name = true;
bool enemy_esp_visible = false;
bool ally_esp_visible = false;
bool crosshair = true;
bool ImGui_Initialised = false;
bool CreateConsole = false;
bool aimbot = true;
bool show_fov = true;
float aim_fov = 10.f;
bool aim_visible = true;
float smooth = 75.f;

using namespace rn;

namespace OverlayWindow {
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}

namespace DirectX9Interface {
	IDirect3D9Ex* Direct3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParams = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}

bsp_parser _bsp_parser;
void DoAimbot(Vector3 headpos, float factor, bool show_fov, bool is_visible) {
	if (factor > 16.f || (!is_visible && aim_visible))
		return;
	float fov = (factor / ((30 - aim_fov) - factor)) * 100;
	float aimX = headpos.x - Process::WindowWidth / 2;
	float aimY = headpos.y - Process::WindowHeight / 2;
	if (show_fov) {
		RGBA White = { 255, 255, 255, 255 };
		DrawCircle((int)headpos.x, (int)headpos.y, fov, &White);
	}
	if (!aimbot || ShowMenu)
		return;
	if (!GetAsyncKeyState(VK_HOME) && !GetAsyncKeyState(VK_LBUTTON))
		return;

	if (abs(aimX) < fov && abs(aimY) < fov) {
		if (smooth == 0) {
			aimX *= 4;
			aimY *= 4;
		}
		aimX *= (100 - smooth) / 100;
		aimY *= (100 - smooth) / 100;
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, aimX, aimY, 0, 0);
	}
}

void Draw() {
	RGBA Cyan = { 0, 231, 255, 255 };
	if (crosshair) DrawCircleFilled(Process::WindowWidth/2, Process::WindowHeight/2, 3, &Cyan);

	if (!Game::client || !Game::engine) {
		init_modules();
	}

	char map_name[256];
	static bool parsed_map = false;
	static std::string map_name_old = "";
	for (int j = 0; j < 256; j++) {
		char c = RPM<char>(Game::engine + MAP_NAME + j*sizeof(char));
		if (c == '\0') {
			map_name[j] = '\0';
			break;
		}
		map_name[j] = c;
	}

	if (map_name[0] == '\0')
		return;
	if (std::string(map_name) != map_name_old || !parsed_map) {
		if (parsed_map)
			_bsp_parser.unload_map();
		parsed_map = _bsp_parser.load_map(std::string(Game::path) + std::string("cstrike\\maps"), map_name);
		map_name_old = std::string(map_name);
	}

	if (!ally_esp && !enemy_esp && !aimbot && !show_fov) {
		return;
	}

	RGBA White = { 255, 255, 255, 255 };

	uintptr_t entity_list = Game::client + ENTITY_LIST;
	uintptr_t localplayer = RPM<uintptr_t>(Game::client + LOCALPLAYER);
	Vector3 pos = RPM<Vector3>(localplayer + XYZ);
	int localplayer_team = RPM<uintptr_t>(localplayer + TEAM);
	int ent_idx = 0;

	for (int i = 0; i < 32; i++) {
		uintptr_t ent = GetEntity(entity_list, i);
		if (ent == NULL || ent == localplayer) {
			continue;
		}

		Vector3 absOrigin = RPM<Vector3>(ent + XYZ);
		Vector3 w2s_absOrigin;

		if (!WorldToScreen(absOrigin, w2s_absOrigin)) {
			continue;
		}

		int ent_team = RPM<int>(ent + TEAM);
		if (ent_team == localplayer_team && !ally_esp) {
			continue;
		}

		if (RPM<BYTE>(ent + LIFESTATE) != 0 || RPM<int>(ent + DORMANT)) {
			continue;
		}

		int health = RPM<int>(ent + HEALTH);
		if (health == 0) {
			continue;
		}

		bool is_visible = true;
		if (parsed_map)
			is_visible = _bsp_parser.is_visible(vector3{pos.x, pos.y, pos.z + 66}, vector3{absOrigin.x, absOrigin.y, absOrigin.z + 66});
		
		//getting name
		DWORD list = RPM<DWORD>(Game::client + NAME_LIST) + 0x38;
		char name[256];
		for (int j = 0; j < 256; j++) {
			char c = RPM<char>(list + 0x140 * i + j*sizeof(char));
			if (c == '\0') {
				name[j] = '\0';
				break;
			}
			name[j] = c;
		}

		//esp
		DWORD bonematrix_addr = RPM<DWORD>(ent + BONEMATRIX);
		if (!bonematrix_addr) continue;

		Vector3 headpos = GetBonePos(bonematrix_addr, BoneID::PELVIS);
		headpos.z += headpos.z - absOrigin.z;

		Vector3 w2s_headpos;

		if (!WorldToScreen(headpos, w2s_headpos)) {
			continue;
		}

		Vector3 neckpos = GetBonePos(bonematrix_addr, BoneID::HEAD);
		neckpos.z += 5.f;
		neckpos.y -= 2.f;
		Vector3 w2s_neckpos;

		if (!WorldToScreen(neckpos, w2s_neckpos)) {
			continue;
		}

		if (ent_team != localplayer_team)
			DoAimbot(w2s_neckpos, sqrt(abs(w2s_headpos.y - w2s_neckpos.y)), show_fov, is_visible);

#ifdef  _DEBUG
		char ent_text[256];
		sprintf(ent_text, xorstr_("%s --> %d"), name, health);
		DrawStrokeText(30, 100 + 16*ent_idx, &White, ent_text);
		ent_idx++;
#endif //  _DEBUG
		

		RGBA color = { 255, 255, 255, 255 };
		if ((ent_team == localplayer_team && ally_box && ally_esp && ((ally_esp_visible && is_visible) || !ally_esp_visible))
			|| (ent_team != localplayer_team && enemy_box && enemy_esp && ((enemy_esp_visible && is_visible) || !enemy_esp_visible))) {
			DrawEspBox2D(w2s_absOrigin, w2s_headpos, &color, 1);
		}

		if ((ent_team == localplayer_team && ally_name && ally_esp && ((ally_esp_visible && is_visible) || !ally_esp_visible))
			|| (ent_team != localplayer_team && enemy_name && enemy_esp && ((enemy_esp_visible && is_visible) || !enemy_esp_visible))) {
			DrawNameTag(w2s_absOrigin, w2s_headpos, name);
		}

		if ((ent_team == localplayer_team && ally_esp_health_bar && ally_esp && ((ally_esp_visible && is_visible) || !ally_esp_visible))
			|| (ent_team != localplayer_team && enemy_esp_health_bar && enemy_esp && ((enemy_esp_visible && is_visible) || !enemy_esp_visible))) {
			DrawHealthBar(w2s_absOrigin, w2s_headpos, health);
		}

		if ((ent_team == localplayer_team && ally_skeleton && ally_esp && ((ally_esp_visible && is_visible) || !ally_esp_visible))
			|| (ent_team != localplayer_team && enemy_skeleton && enemy_esp && ((enemy_esp_visible && is_visible) || !enemy_esp_visible))) {
			DrawBones(bonematrix_addr, &color, 1);
		}
	}
}

void DrawMenu() {
	ImGui::SetNextWindowSize(ImVec2(500.f, 400.f));
	ImGui::Begin("CSS hax", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	ImGui::Checkbox("Crosshair", &crosshair);
	ImGui::Separator();

	ImGui::Checkbox("Ally ESP", &ally_esp);
	ImGui::SameLine();
	ImGui::Checkbox("Visible only##ally", &ally_esp_visible);
	ImGui::Checkbox("Box##ally", &ally_box);
	ImGui::Checkbox("Skeleton##ally", &ally_skeleton);
	ImGui::Checkbox("Healthbar##ally", &ally_esp_health_bar);
	ImGui::Checkbox("Name##ally", &ally_name);

	ImGui::Separator();
	ImGui::Checkbox("Enemy ESP", &enemy_esp);
	ImGui::SameLine();
	ImGui::Checkbox("Visible only##enemy", &enemy_esp_visible);
	ImGui::Checkbox("Box##enemy", &enemy_box);
	ImGui::Checkbox("Skeleton##enemy", &enemy_skeleton);
	ImGui::Checkbox("Healthbar##enemy", &enemy_esp_health_bar);
	ImGui::Checkbox("Name##enemy", &enemy_name);

	ImGui::Separator();
	ImGui::Checkbox("Aimbot", &aimbot);
	ImGui::SameLine();
	ImGui::Checkbox("Show FOV", &show_fov);
	ImGui::Checkbox("Visible only##aimbot", &aim_visible);
	ImGui::SliderFloat("FOV", &aim_fov, 1.f, 30.f, "%1.f", 1.f);
	ImGui::SliderFloat("Smoothing", &smooth, 0.f, 100.f, "%1.f", 1.f);

	ImGui::Separator();
	if (ImGui::Button("Unhook")) {
		Unhook = true;
	}

	ImGui::End();
}

void Render() {
	if (GetAsyncKeyState(VK_DELETE) & 1) Unhook = true;
	if (GetAsyncKeyState(VK_INSERT) & 1) ShowMenu = !ShowMenu;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	Draw();

	if (ShowMenu)
		DrawMenu();

	ImGui::EndFrame();

	DirectX9Interface::pDevice->SetRenderState(D3DRS_ZENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

	DirectX9Interface::pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (DirectX9Interface::pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		DirectX9Interface::pDevice->EndScene();
	}

	HRESULT result = DirectX9Interface::pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && DirectX9Interface::pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void MainLoop() {
	static RECT OldRect;
	ZeroMemory(&DirectX9Interface::Message, sizeof(MSG));

	while (DirectX9Interface::Message.message != WM_QUIT) {
		if (Unhook) {
			ImGui_ImplDX9_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			DestroyWindow(OverlayWindow::Hwnd);
			UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
			exit(0);
		}

		if (PeekMessage(&DirectX9Interface::Message, OverlayWindow::Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&DirectX9Interface::Message);
			DispatchMessage(&DirectX9Interface::Message);
		}
		HWND ForegroundWindow = GetForegroundWindow();
		if (ForegroundWindow == Process::Hwnd) {
			HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
			SetWindowPos(OverlayWindow::Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT TempRect;
		POINT TempPoint;
		ZeroMemory(&TempRect, sizeof(RECT));
		ZeroMemory(&TempPoint, sizeof(POINT));

		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = Process::Hwnd;

		POINT TempPoint2;
		GetCursorPos(&TempPoint2);
		io.MousePos.x = TempPoint2.x - TempPoint.x;
		io.MousePos.y = TempPoint2.y - TempPoint.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}

		if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
			OldRect = TempRect;
			Process::WindowWidth = TempRect.right;
			Process::WindowHeight = TempRect.bottom;
			DirectX9Interface::pParams.BackBufferWidth = Process::WindowWidth;
			DirectX9Interface::pParams.BackBufferHeight = Process::WindowHeight;
			SetWindowPos(OverlayWindow::Hwnd, (HWND)0, TempPoint.x, TempPoint.y, Process::WindowWidth, Process::WindowHeight, SWP_NOREDRAW);
			DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		}
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (DirectX9Interface::pDevice != NULL) {
		DirectX9Interface::pDevice->EndScene();
		DirectX9Interface::pDevice->Release();
	}
	if (DirectX9Interface::Direct3D9 != NULL) {
		DirectX9Interface::Direct3D9->Release();
	}
	DestroyWindow(OverlayWindow::Hwnd);
	UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
}

bool DirectXInit() {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9Interface::Direct3D9))) {
		return false;
	}

	D3DPRESENT_PARAMETERS Params = { 0 };
	Params.Windowed = TRUE;
	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Params.hDeviceWindow = OverlayWindow::Hwnd;
	Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	Params.BackBufferWidth = Process::WindowWidth;
	Params.BackBufferHeight = Process::WindowHeight;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.EnableAutoDepthStencil = TRUE;
	Params.AutoDepthStencilFormat = D3DFMT_D16;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	if (FAILED(DirectX9Interface::Direct3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, OverlayWindow::Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9Interface::pDevice))) {
		DirectX9Interface::Direct3D9->Release();
		return false;
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontDefault();

	ImGui_ImplWin32_Init(OverlayWindow::Hwnd);
	ImGui_ImplDX9_Init(DirectX9Interface::pDevice);
	DirectX9Interface::Direct3D9->Release();
	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message) {
	case WM_DESTROY:
		if (DirectX9Interface::pDevice != NULL) {
			DirectX9Interface::pDevice->EndScene();
			DirectX9Interface::pDevice->Release();
		}
		if (DirectX9Interface::Direct3D9 != NULL) {
			DirectX9Interface::Direct3D9->Release();
		}
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (DirectX9Interface::pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			DirectX9Interface::pParams.BackBufferWidth = LOWORD(lParam);
			DirectX9Interface::pParams.BackBufferHeight = HIWORD(lParam);
			//HRESULT hr = DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
			//if (hr == D3DERR_INVALIDCALL)
				//IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	OverlayWindow::WindowClass = {
		sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverlayWindow::Name, LoadIcon(nullptr, IDI_APPLICATION)
	};
	
	RegisterClassEx(&OverlayWindow::WindowClass);
	if (Process::Hwnd){
		static RECT TempRect = { NULL };
		static POINT TempPoint;
		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);
		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		Process::WindowWidth = TempRect.right;
		Process::WindowHeight = TempRect.bottom;
	}

	OverlayWindow::Hwnd = CreateWindowEx(NULL, OverlayWindow::Name, OverlayWindow::Name, WS_POPUP | WS_VISIBLE, Process::WindowLeft, Process::WindowTop, Process::WindowWidth, Process::WindowHeight, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(OverlayWindow::Hwnd, &DirectX9Interface::Margin);
	SetWindowLong(OverlayWindow::Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT);
	ShowWindow(OverlayWindow::Hwnd, SW_SHOW);
	UpdateWindow(OverlayWindow::Hwnd);
}

DWORD WINAPI ProcessCheck(LPVOID lpParameter) {
	while (true) {
		if (Process::Hwnd != NULL) {
			if (Game::PID == 0) {
				exit(0);
			}
		}
	}
}

int main() {
	if (CreateConsole == false) {
		ShowWindow(GetConsoleWindow(), SW_HIDE); 
		FreeConsole();
	}

	bool WindowFocus = false;
	Game::PID = GetProcessId(TargetProcess);
	while (WindowFocus == false) {
		DWORD ForegroundWindowProcessID;
		GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
		if (Game::PID == ForegroundWindowProcessID) {
			Process::ID = GetCurrentProcessId();
			Process::Handle = GetCurrentProcess();
			Process::Hwnd = GetForegroundWindow();

			RECT TempRect;
			GetWindowRect(Process::Hwnd, &TempRect);
			Process::WindowWidth = TempRect.right - TempRect.left;
			Process::WindowHeight = TempRect.bottom - TempRect.top;
			Process::WindowLeft = TempRect.left;
			Process::WindowRight = TempRect.right;
			Process::WindowTop = TempRect.top;
			Process::WindowBottom = TempRect.bottom;

			char TempTitle[MAX_PATH];
			GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle));
			Process::Title = TempTitle;

			char TempClassName[MAX_PATH];
			GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName));
			Process::ClassName = TempClassName;

			char TempPath[MAX_PATH];
			GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath));
			Process::Path = TempPath;

			WindowFocus = true;
		}
	}

	Game::handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, Game::PID);
	DWORD size = MAX_PATH;
	QueryFullProcessImageNameA(Game::handle, 0, Game::path, &size);
	Game::path[strlen(Game::path) - 7] = '\0';
	
	std::string s = RandomString(10);
	OverlayWindow::Name = s.c_str();
	SetupWindow();
	DirectXInit();
	//CreateThread(0, 0, ProcessCheck, 0, 0, 0); TOFIX
	while (TRUE) {
		MainLoop();
	}
}