#pragma once

#include "imgui/imgui.h"

namespace engine
{
	bool initialize();

	void run_visuals(const ImDrawList* draw_list);
}