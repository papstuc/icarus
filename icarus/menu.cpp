#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <dxgi.h>

#include "imgui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "menu.hpp"
#include "engine.hpp"
#include "config.hpp"
#include "console.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

static std::vector<config::config_t> configs = { };
static const char* current_name = "Select config";
static std::int32_t current_item = 0;

void menu::initialize(HWND window, ID3D11Device* device, ID3D11DeviceContext* context)
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, context);
	ImGui_ImplDX11_CreateDeviceObjects();
}

void menu::render(ID3D11DeviceContext* context, ID3D11RenderTargetView* render_view)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
	if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground))
	{
		engine::run_visuals(ImGui::GetWindowDrawList());
		ImGui::End();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(800.f, 400.f));
	if (config::menu_open && ImGui::Begin("icarus - SoT", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		if (ImGui::BeginTabBar("##features"))
		{
			if (ImGui::BeginTabItem("Player ESP"))
			{
				ImGui::Checkbox("Players", &config::context.player_esp);
				ImGui::Checkbox("Teammates", &config::context.teammate_esp);
				ImGui::Checkbox("Box", &config::context.draw_box);
				ImGui::Checkbox("Name", &config::context.draw_name);
				ImGui::Checkbox("Health", &config::context.draw_health);
				ImGui::Checkbox("Health bar", &config::context.draw_health_bar);
				ImGui::Checkbox("Draw distance", &config::context.draw_distance);
				ImGui::SliderFloat("Render distance", &config::context.player_render_distance, 50.f, 1500.f, "%.2f", 0);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Ship ESP"))
			{
				ImGui::Checkbox("Ships", &config::context.ship_esp);
				ImGui::Checkbox("Sunken ships", &config::context.sunken_ship_esp);
				ImGui::Checkbox("Ship holes", &config::context.draw_ship_holes);
				ImGui::ColorEdit4("Hole color", config::context.ship_hole_color);
				ImGui::ColorEdit4("Text color", config::context.ship_text_color);
				ImGui::SliderFloat("Render distance", &config::context.ship_render_distance, 50.f, 2500.f, "%.2f", 0);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Item ESP"))
			{
				ImGui::Checkbox("Items", &config::context.draw_health);
				ImGui::Checkbox("Mermaids", &config::context.mermaids);
				ImGui::Checkbox("Lost cargo", &config::context.lost_cargo);
				ImGui::SliderFloat("Render distance", &config::context.ship_render_distance, 50.f, 2500.f, "%.2f", 0);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Configuration"))
			{
				if (ImGui::BeginCombo("##Configurations", current_name))
				{
					for (std::size_t i = 0; i < configs.size(); i++)
					{
						if (ImGui::Selectable(configs[i].name))
						{
							current_name = configs[i].name;
						}
					}
					ImGui::EndCombo();
				}
				if (ImGui::Button("Update", ImVec2(80, 20)))
				{
					menu::update_configs();
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
	
	ImGui::PopStyleVar();

	ImGui::Render();

	context->OMSetRenderTargets(1, &render_view, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

LRESULT menu::handle_window_procedure(HWND window, std::uint32_t message, WPARAM wparam, LPARAM lparam)
{
	return ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);
}

std::int32_t menu::get_mouse_cursor()
{
	return ImGui::GetMouseCursor();
}

void menu::release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext();
}

bool menu::update_configs()
{
	configs.clear();
	return config::get_configs(configs);
}
