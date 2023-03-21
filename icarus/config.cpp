#include <filesystem>
#include <iostream>
#include <fstream>
#include <iterator>
#include "config.hpp"

config::context_t config::context = { };

bool config::menu_open = true;

static bool get_sot_path(std::filesystem::path& path)
{
	wchar_t module_path[MAX_PATH];
	if (!GetModuleFileName(GetModuleHandle(0), module_path, MAX_PATH))
	{
		return false;
	}

	path = std::filesystem::path(module_path).parent_path().parent_path().parent_path();

	return std::filesystem::exists(path);
}

static bool read_config(const std::filesystem::path path, config::config_t& config)
{
	if (!std::filesystem::exists(path) || path.extension() != ".cfg")
	{
		return false;
	}

	std::ifstream input(path, std::ios::binary);

	std::uint32_t magic_number = 0;
	input.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));

	if (magic_number != config::magic)
	{
		input.close();
		return false;
	}

	input.read(reinterpret_cast<char*>(&config), sizeof(config));
	input.close();


	return true;
}

static bool write_config(const std::filesystem::path path, config::config_t& config)
{
	std::ofstream output(path, std::ios::binary);
	output.write(reinterpret_cast<const char*>(&config::magic), sizeof(config::magic));
	output.write(reinterpret_cast<const char*>(&config), sizeof(config));
	output.close();

	return true;
}

bool config::load_config(const char* config_name)
{
	std::filesystem::path path;
	if (!get_sot_path(path))
	{
		return false;
	}

	path.replace_filename(config_name);
	path.replace_extension("cfg");

	config::config_t config = { };
	
	if (!read_config(path, config))
	{
		return false;
	}

	config::context = config.ctx;

	return true;
}

bool config::save_config(const char* config_name)
{
	std::filesystem::path path;
	if (!get_sot_path(path))
	{
		std::cout << "path" << std::endl;
		return false;
	}

	path.replace_filename(config_name);
	path.replace_extension("cfg");

	std::cout << path << std::endl;

	config::config_t config = { };
	config.ctx = config::context;
	strcpy_s(config.name, config_name);

	if (!write_config(path, config))
	{
		std::cout << "failed to write" << std::endl;
		return false;
	}

	return true;
}

bool config::get_configs(std::vector<config::config_t>& configs)
{
	std::filesystem::path path;
	if (!get_sot_path(path))
	{
		return false;
	}

	path = path.parent_path();

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path))
	{
		config::config_t config = { };

		if (entry.is_directory())
		{
			continue;
		}

		if (!read_config(entry.path(), config))
		{
			continue;
		}

		configs.push_back(config);
	}

	return true;
}