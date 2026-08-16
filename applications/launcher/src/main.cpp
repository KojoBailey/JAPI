//
// Created by kapil on 12.02.2026.
//

#include <windows.h>
#include <d3d11.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "binary_file.h"
#include "downloader.h"
#include "logger.h"

#include "launcher.h"

// TODO: Wrap in class rather than global statics.
static ID3D11Device* d3dDevice = nullptr;
static ID3D11DeviceContext* d3dDeviceContext = nullptr;
static IDXGISwapChain* swapChain = nullptr;
static ID3D11RenderTargetView* mainRenderTargetView = nullptr;

// Callback procedure for window events.
// NOTE: Must declare extern manually because `imgui_impl_win32.h` is designed to be includable without <windows.h>.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Returns >0 if ImGui handles the message.
	LRESULT imGuiResult = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	if (imGuiResult > 0) {
        return imGuiResult;
	}

    if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC swapChainDescription = {
		.BufferDesc = {
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		},
		.SampleDesc = {
			.Count = 1,
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.OutputWindow = hWnd,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
	};

    D3D_FEATURE_LEVEL featureLevel;
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &swapChainDescription, &swapChain,
        &d3dDevice, &featureLevel, &d3dDeviceContext);

    ID3D11Texture2D* pBackBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
    pBackBuffer->Release();

    return true;
}

int main() {
    logger::init("JAPILauncher", "japi/logs");

	WNDCLASSEX windowClass = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_CLASSDC,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
        .hIcon = nullptr,
		.hCursor = nullptr,
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = "JAPILauncher", // For Windows API bookkeeping.
		.hIconSm = nullptr
	};
    RegisterClassEx(&windowClass);
    HWND hwnd = CreateWindow(
		windowClass.lpszClassName,
		"JoJoAPI Launcher", // title
		WS_OVERLAPPEDWINDOW, // style
        100, 100, // x, y screen position
		800, 600, // width, height
		nullptr, // parent window
		nullptr, windowClass.hInstance, nullptr
	);

    CreateDeviceD3D(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImFont* bigFont = io.Fonts->AddFontFromFileTTF("japi\\JetBrainsMono-Regular.ttf", 48.0f);

    ImGui::StyleColorsLight();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(1, 1, 1, 1);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);

    bool isDone = false;
    while (not isDone) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            isDone = (msg.message == WM_QUIT);
        }
        if (isDone) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("main", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar);

        ImGui::PushFont(bigFont);
        const char* text = "JoJoAPI";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 winSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(
            (winSize.x - textSize.x) * 0.5f,
            (winSize.y - textSize.y) * 0.5f));
        ImGui::Text("%s", text);
        ImGui::PopFont();

        ImGui::End();
        ImGui::Render();

        const float clearColor[4] = { 1, 1, 1, 1 };
        d3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        d3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    try {
        launcher{}
			.run();
    } catch (const std::exception& e) {
        ERROR_AND_QUIT(e.what());
    }

    JINFO("Shutting down launcher...");
}
