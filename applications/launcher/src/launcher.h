//
// Created by kapil on 16.02.2026.
//

#ifndef JAPI_LAUNCHER_H
#define JAPI_LAUNCHER_H

#include <windows.h>
#include <d3d11.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <atomic>
#include <ctime>
#include <string>
#include <thread>

#include "config.h"
#include "downloader.h"

class launcher {
public:
    launcher();

    void run();

private:
	enum class quit_status {
		QUIT,
		CONTINUE,
	};

	bool is_minimized{false};

	void init_gui();
	quit_status update_gui();
	void render_ui();
	void destroy_gui();

    void check_for_updates();
    void install_japi(const std::string& update_file_name);
    void cleanup_old_files();
    void launch_game(bool should_launch_modded);

	bool create_d3d_device(HWND hWnd);
	void resize_render_target(UINT new_width, UINT new_height);

    config _cfg;
    downloader _dl;

	ID3D11Device* d3dDevice;
	ID3D11DeviceContext* d3dDeviceContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* mainRenderTargetView;

	ImFont* mainFont;

	std::jthread game_thread;
	std::atomic<bool> is_game_running{false};
};

#endif //JAPI_LAUNCHER_H
