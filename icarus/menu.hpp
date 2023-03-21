#pragma once

#include <d3d11.h>
#include <cstdint>

#include "ue4/UE4.h"
#include "imgui/imgui.h"

namespace menu
{
	void initialize(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);

	void render(ID3D11DeviceContext* context, ID3D11RenderTargetView* render_view);
	LRESULT handle_window_procedure(HWND window, std::uint32_t message, WPARAM wparam, LPARAM lparam);
	std::int32_t get_mouse_cursor();

	void release();

	bool update_configs();
}