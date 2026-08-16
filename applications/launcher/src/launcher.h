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

#include <ctime>
#include <string_view>

#include "config.h"
#include "downloader.h"

class launcher {
public:
    launcher();

    void run();

private:
    void check_for_updates();
    void install_japi(const std::string& update_file_name);
    void cleanup_old_files();
    void launch_game();

	bool create_d3d_device(HWND hWnd);

    config _cfg;
    downloader _dl;

	ID3D11Device* d3dDevice;
	ID3D11DeviceContext* d3dDeviceContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* mainRenderTargetView;

	ImFont* mainFont;
};

#endif //JAPI_LAUNCHER_H
