#include <Windows.h>
#include <cstdint>

#include <vector>

namespace config
{
	constexpr std::uint32_t magic = 0xDEADBEEF;

	typedef struct _context_t
	{
		bool player_esp = false;
		bool teammate_esp = false;
		bool draw_box = false;
		float box_color[4] = { 1.f, 0.f, 0.f, 1.f };
		bool draw_name = false;
		float name_color[4] = { 1.f, 0.f, 0.f, 1.f };
		bool draw_health = false;
		bool draw_health_bar = false;
		bool draw_distance = false;
		float player_render_distance = 500.f;

		bool ship_esp = false;
		bool sunken_ship_esp = false;
		float ship_text_color[4] = { 1.f, 0.f, 0.f, 1.f};
		bool draw_ship_holes = false;
		float ship_hole_color[4] = { 1.f, 0.f, 0.f, 1.f };
		float ship_render_distance = 500.f;

		bool item_esp = false;
		bool mermaids = false;
		bool lost_cargo = false;
		float item_render_distance = 500.f;

		bool hud_indicators = false;
	} context_t;

	typedef struct _config_t
	{
		char name[MAX_PATH];
		context_t ctx;
	} config_t;

	extern context_t context;
	extern bool menu_open;

	bool load_config(const char* config_name);
	bool save_config(const char* config_name);
	bool get_configs(std::vector<config::config_t>& configs);
}