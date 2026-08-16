//
// Created by kapil on 16.02.2026.
//

#include "launcher.h"

#include <algorithm>
#include <filesystem>

#include "binary_file.h"
#include "logger.h"
#include "process.h"
#include "defines.h"
#include "input.h"
#include "installer.h"

#include "nlohmann/json.hpp"
#include "miniz.h"
#include "version.h"

// NOTE: Must declare extern manually because `imgui_impl_win32.h` is designed to be includable without <windows.h>.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using json = nlohmann::json;

launcher::launcher() : _cfg(config::load("japi/config/launcher.toml")) {}

void launcher::run() {
	WNDCLASSEX windowClass = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_CLASSDC,
		.lpfnWndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
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
		},
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

    create_d3d_device(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    mainFont = io.Fonts->AddFontFromFileTTF("japi\\JetBrainsMono-Regular.ttf", 48.0f);

    ImGui::StyleColorsLight();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(1, 1, 1, 1);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);

    JINFO("Running launcher version %s", LAUNCHER_VERSION);

    if (_cfg.get<bool>("auto_update", false)) {
        JINFO("Auto-update is enabled, checking for updates...");
        check_for_updates();
    }

    if (_cfg.get<bool>("first_launch", true)) {
        JINFO("First launch detected, setting up...");
        _cfg.set("first_launch", false);

        _cfg.set("auto_update", input::query("Do you want to enable auto-updates? (Recommended)", "Auto-Update"));

        if (input::query("Should JAPIUpdater try to cleanup old JAPI files?", "Legacy")) {
            cleanup_old_files();
        }

        check_for_updates();
    }

    launch_game();
}

void launcher::check_for_updates() {
    // Download manifest
    const auto url = "https://raw.githubusercontent.com/Kapilarny/JAPI/new_files/updates/manifest.json";
    JINFO("Downloading update manifest from: %s", url);

    std::vector<char> manifest_data;

    try {
        manifest_data = _dl.download_file(url);
    } catch (const std::exception& e) {
        JERROR("Failed to download update manifest: %s", e.what());
        return;
    }

    // Parse
    json manifest_json = json::parse(manifest_data);

    if (!manifest_json.contains("version") || !manifest_json.contains("content")) {
        throw std::runtime_error("launcher::check_for_updates - Invalid manifest format.");
    }

    if (manifest_json["version"] != 1) {
        throw std::runtime_error("launcher::check_for_updates - Unsupported manifest version: " + std::to_string(manifest_json["version"].get<int>()));
    }

    const auto& content = manifest_json["content"];
    const std::string latest_version = content["latest_version"];

    const auto installed_version = _cfg.get<std::string>("japi_version", "0.0.0");

    if (version(latest_version) > version(installed_version)) {
        JINFO("Update available: %s -> %s", installed_version.c_str(), latest_version.c_str());

        if (input::query("An update is available. Do you want to install it?", "Update")) {
            const auto& updates = content["updates"];
            const auto& latest = std::ranges::find_if(updates, [&](const json& update) {
                return update["version"] == latest_version;
            });

            if (latest == updates.end()) {
                throw std::runtime_error("launcher::check_for_updates - Update information for version " + latest_version + " not found in manifest.");
            }

            // Get the update file name
            const std::string update_file_name = (*latest)["package_name"];

            install_japi(update_file_name);
            _cfg.set("japi_version", latest_version);
        } else {
            JINFO("User chose not to update.");
        }
    } else {
        JINFO("No updates available. Installed version: %s", installed_version.c_str());
    }
}

void launcher::install_japi(const std::string& update_file_name) {
    // Download the update package
    const auto url = "https://raw.githubusercontent.com/Kapilarny/JAPI/new_files/updates/japi/" + update_file_name;
    JINFO("Downloading update package from: %s", url.c_str());

    auto data = _dl.download_file(url);
    JINFO("Downloaded %zu bytes.", data.size());

    binary_file update_file(reinterpret_cast<std::vector<uint8_t>&>(data), true);

    installer inst(update_file, _dl);
    inst.install();
}

void launcher::cleanup_old_files() {
    std::vector<std::string> old_files = {
        "d3dcompiler_47_o.dll",
        "JAPIPreload.dll",
        "dinput8.dll",
        "dinput8_o.dll",
        "JAPIInstaller.exe"
    };

    // Check if any of the old files exist
    if (!std::ranges::any_of(old_files, [](const std::string& file) { return std::filesystem::exists(file); })) {
        return;
    }

    JINFO("Detected old files...");

    // Prompt the user to delete them
    const int result = MessageBoxA(nullptr, "Detected old JAPI files. Do you want to delete them? (Recommended)", "Old Files Detected", MB_ICONQUESTION | MB_YESNO);
    if (result != IDYES) {
        JINFO("User chose not to delete old files, skipping cleanup...");
        return;
    }

    // Old japi shit
    if (std::filesystem::exists("d3dcompiler_47_o.dll")) {
        // Delete the old JAPI if it exists
        if (std::filesystem::exists("d3dcompiler_47_o.dll")) {
            std::filesystem::remove("d3dcompiler_47.dll");
            JINFO("Deleted old file: d3dcompiler_47_o.dll");
        }

        std::filesystem::rename("d3dcompiler_47_o.dll", "d3dcompiler_47.dll");
        JINFO("Restored d3dcompiler_47.dll");
    }

    // Add configs to the list of old files to delete
    old_files.emplace_back("japi/config/JAPI.cfg");
    old_files.emplace_back("japi/config/updater.cfg");

    for (const auto& file : old_files) {
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
            JINFO("Deleted old file: %s", file.c_str());
        }
    }
}

void launcher::launch_game() {
    // Get current PWD
#ifdef DEBUG_MODE
    const std::string current_path = R"(C:\Program Files (x86)\Steam\steamapps\common\JoJo's Bizarre Adventure All-Star Battle R)";
#else
    const std::string current_path = std::filesystem::current_path().string();
#endif

    const std::string game_path = current_path + R"(\japi\bin\unpacked.exe)";

    process g_process(game_path.c_str(), current_path.c_str());
    logger::despawn_console();
    do {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            // isDone = (msg.message == WM_QUIT);
        }
        // if (isDone) break;

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

        ImGui::PushFont(mainFont);
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

        g_process.restart();
        g_process.inject_dll(std::string(current_path + R"(\japi\dlls\JAPIPreload.dll)").c_str());
        g_process.resume(true);
    } while (g_process.get_exit_code() == 67);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool launcher::create_d3d_device(HWND hWnd) {
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
