#include "window.h"

#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_impl_win32.h"
#include "resource.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Window
{
	std::string caption_ = "test";
	unsigned int width_ = 1280U;
	unsigned int height_ = 720U;
	HWND hwnd_ = 0;
	HINSTANCE hinstance_ = 0;

	void Initalize(void)
	{
		{// ハンドル取得
			hinstance_ = GetModuleHandleA(nullptr);
		}

		{// ウィンドウクラスの登録
			WNDCLASSEX  wc;
			memset(&wc, 0, sizeof(wc));
			wc.cbSize = sizeof(wc);
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = WndProc;
			wc.hInstance = hinstance_;
			wc.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
			wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
			wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
			wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
			wc.lpszClassName = caption_.c_str();
			wc.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);
			RegisterClassExA(&wc);
		}

		{// ウィンドウの大きさの再調整、中央配置計算
			RECT rc;
			rc.left = 0;
			rc.top = 0;
			rc.right = static_cast<long>(width_);
			rc.bottom = static_cast<long>(height_);

			AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, true, 0);

			int w = rc.right - rc.left;
			int h = rc.bottom - rc.top;

			RECT rc_desk;
			GetWindowRect(GetDesktopWindow(), &rc_desk);

			int x = rc_desk.right / 2 - w / 2;
			int y = rc_desk.bottom / 2 - h / 2;

			{// ウィンドウの生成、表示
				hwnd_ = CreateWindowExA(0, caption_.c_str(), caption_.c_str(), WS_OVERLAPPEDWINDOW,
					x, y, w, h, 0, 0, hinstance_, 0);

				ShowWindow(hwnd_, SW_SHOW);
			}
		}
	}

	bool Run(void)
	{
		return MessageLoop();
	}

	void Finalize(void)
	{
	}

	const unsigned int & width(void)
	{
		return width_;
	}

	const unsigned int & height(void)
	{
		return height_;
	}
	HWND & hwnd(void)
	{
		return hwnd_;
	}
	bool MessageLoop(void)
	{
		MSG msg = { 0 };
		memset(&msg, 0, sizeof(msg));

		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			if (msg.message == WM_QUIT) return false;
		}
		return true;
	}
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				ImGui_ImplDX11_InvalidateDeviceObjects();
				/*CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();*/
				ImGui_ImplDX11_CreateDeviceObjects();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_COMMAND:
			{
				int menuID = LOWORD(wParam);
				switch(menuID)
				{
				case ID_TEST_A:
					DestroyWindow(hWnd);
					break;
	}
				}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}