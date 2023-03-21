#pragma once

#include "imgui/imgui.h"

namespace engine
{
	bool initialize();

	void run_visuals(ImDrawList* draw_list);
}