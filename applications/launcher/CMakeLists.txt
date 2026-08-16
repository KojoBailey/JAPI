project(JAPILauncher CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static")

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/process.h
        src/process.cpp
        src/defines.h
        src/launcher.cpp
        src/launcher.h
        src/input.cpp
        src/installer.cpp
        src/installer.h

        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_impl_win32.cpp
        ${IMGUI_DIR}/imgui_impl_dx11.cpp
)


target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/external
	${CMAKE_SOURCE_DIR}/libs
)
target_link_libraries(${PROJECT_NAME}
	JSIGN JLOG JDOWNLOAD JConfig miniz
	d3d11 dxgi d3dcompiler dwmapi
)
