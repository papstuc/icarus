#include "engine.hpp"
#include "utilities.hpp"
#include "ue4/sdk.hpp"
#include "menu.hpp"
#include "console.hpp"
#include "config.hpp"

static UAthenaGameViewportClient* game_viewport = nullptr;
static AController* player_controller = nullptr;
static AAthenaPlayerCharacter* local_player_actor = nullptr;
static ACharacter* local_player_character = nullptr;

bool engine::initialize()
{
	// gworld - 48 8B 0D ? ? ? ? 4C 8B CB 4C 8B C7 - 3, 7
	// gobjects - 89 0D ? ? ? ? 48 8B DF - 2, 6
	// gnames - 48 89 1D ? ? ? ? 8B C3 - 3, 7

	const LPVOID gworld = static_cast<LPVOID>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "48 8B 0D ? ? ? ? 4C 8B CB 4C 8B C7"), 3, 7));
	const LPVOID gobjects = static_cast<LPVOID>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "89 0D ? ? ? ? 48 8B DF"), 2, 6));
	const LPVOID gnames = static_cast<LPVOID>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "48 89 1D ? ? ? ? 8B C3"), 3, 7));

	if (!gworld || !gobjects || !gnames)
	{
		console::log(L"[-] failed to find engine pointers\n");
		return false;
	}

	UWorld::GWorld = reinterpret_cast<decltype(UWorld::GWorld)>(gworld);
	UObject::GObjects = reinterpret_cast<decltype(UObject::GObjects)>(reinterpret_cast<std::uintptr_t>(gobjects) + 16);
	FName::GNames = *reinterpret_cast<decltype(FName::GNames)*>(gnames);

	game_viewport = UObject::FindObject<UAthenaGameViewportClient>("AthenaGameViewportClient Transient.AthenaGameEngine_1.AthenaGameViewportClient_1");
	
	if (!game_viewport || !UCrewFunctions::Init() || !UKismetMathLibrary::Init() || !UKismetSystemLibrary::Init())
	{
		console::log(L"[-] failed to initialize engine\n");
		return false;
	}
	
	console::log(L"[+] engine initialized\n");
	return true;
}

static void ship_esp(const ImDrawList* draw_list, const ACharacter* entity)
{
	if (!config::context.ship_esp)
	{
		return;
	}

	ACharacter* actor = const_cast<ACharacter*>(entity);

	if (!actor)
	{
		return;
	}

	if (actor->isShip() || actor->isFarShip())
	{
		const FVector local_player_location = local_player_character->K2_GetActorLocation();
		FVector location = actor->K2_GetActorLocation();
		const float distance = local_player_location.DistTo(location) * 0.01f;

		if (distance > config::context.ship_render_distance)
		{
			return;
		}

		location.Z = 1000.f;
		FVector2D screen = { };
		if (!player_controller->ProjectWorldLocationToScreen(location, screen))
		{
			return;
		}

		char buffer[MAX_PATH] = { 0 };

		const float velocity = (actor->GetVelocity() * 0.01f).Size();

		bool ship_found = true;
		if (actor->isShip() && distance < 1726.f)
		{
			float internal_water = actor->GetInternalWater()->GetNormalizedWaterAmount() * 100.f;

			if (actor->compareName("BP_Small"))
			{
				sprintf_s(buffer, sizeof(buffer), "Sloop [%.0f%%][%.0fm][%.0fm/s]", internal_water, distance, velocity);
			}
			else if (actor->compareName("BP_Medium"))
			{
				sprintf_s(buffer, sizeof(buffer), "Brigatine [%.0f%%][%.0fm][%.0fm/s]", internal_water, distance, velocity);
			}
			else if (actor->compareName("BP_Large"))
			{
				sprintf_s(buffer, sizeof(buffer), "Galleon [%.0f%%][%.0fm][%.0fm/s]", internal_water, distance, velocity);
			}
			else if (actor->compareName("AISmall"))
			{
				sprintf_s(buffer, sizeof(buffer), "AI Sloop [%.0f%%][%.0fm][%.0fm/s]", internal_water, distance, velocity);
			}
			else if (actor->compareName("AILarge"))
			{
				sprintf_s(buffer, sizeof(buffer), "AI Galleon [%.0f%%][%.0fm][%.0fm/s]", internal_water, distance, velocity);
			}
			else
			{
				ship_found = false;
			}
		}
		else if (actor->isFarShip() && distance > 1725.f)
		{
			if (actor->compareName("BP_Small"))
			{
				sprintf_s(buffer, sizeof(buffer), "Sloop [%.0fm][%.0fm/s]", distance, velocity);
			}
			else if (actor->compareName("BP_Medium"))
			{
				sprintf_s(buffer, sizeof(buffer), "Brigatine [%.0fm][%.0fm/s]", distance, velocity);
			}
			else if (actor->compareName("BP_Large"))
			{
				sprintf_s(buffer, sizeof(buffer), "Galleon [%.0fm][%.0fm/s]", distance, velocity);
			}
			else if (actor->compareName("AISmall"))
			{
				sprintf_s(buffer, sizeof(buffer), "AI Sloop [%.0fm][%.0fm/s]", distance, velocity);
			}
			else if (actor->compareName("AILarge"))
			{
				sprintf_s(buffer, sizeof(buffer), "AI Galleon [%.0fm][%.0fm/s]", distance, velocity);
			}
			else
			{
				ship_found = false;
			}
		}

		if (ship_found)
		{
			const_cast<ImDrawList*>(draw_list)->AddText(nullptr, 20.f, ImVec2(screen.X, screen.Y), utilities::color_to_value(config::context.ship_text_color), buffer);
		}
	}
}

static void ship_hole_esp(const ImDrawList* draw_list, const ACharacter* lp)
{
	if (!config::context.draw_ship_holes)
	{
		return;
	}

	ACharacter* local_player = const_cast<ACharacter*>(lp);

	if (!local_player || local_player->IsDead())
	{
		return;
	}

	const ACharacter* ship = local_player->GetCurrentShip();

	if (!ship || !const_cast<ACharacter*>(ship)->isShip())
	{
		return;
	}

	const AHullDamage* damage = const_cast<ACharacter*>(ship)->GetHullDamage();

	if (!damage)
	{
		return;
	}

	const TArray<ACharacter*> holes = damage->ActiveHullDamageZones;

	for (std::uint32_t i = 0; i < holes.Count; i++)
	{
		const ACharacter* hole = holes[i];
		const FVector location = const_cast<ACharacter*>(hole)->K2_GetActorLocation();

		FVector2D screen = { };
		if (player_controller->ProjectWorldLocationToScreen(location, screen))
		{
			continue;
		}

		const_cast<ImDrawList*>(draw_list)->AddLine(ImVec2(screen.X - 6.f, screen.Y + 6.f), ImVec2(screen.X + 6.f, screen.Y - 6.f), utilities::color_to_value(config::context.ship_hole_color));
		const_cast<ImDrawList*>(draw_list)->AddLine(ImVec2(screen.X - 6.f, screen.Y - 6.f), ImVec2(screen.X + 6.f, screen.Y + 6.f), utilities::color_to_value(config::context.ship_hole_color));
	}
}

static void player_esp(const ImDrawList* draw_list, ACharacter* actor)
{

}

static void item_esp(const ImDrawList* draw_list, ACharacter* actor)
{

}

static void hud_indicators(const ImDrawList* draw_list, ACharacter* local_player)
{

}

static bool is_ready()
{
	if (!game_viewport)
	{
		return false;
	}

	if (!player_controller)
	{
		return false;
	}

	if (!local_player_actor)
	{
		return false;
	}

	const UWorld* world = *UWorld::GWorld;
	if (!world)
	{
		return false;
	}

	const UGameInstance* game = world->GameInstance;
	if (!game)
	{
		return false;
	}

	const ULocalPlayer* local_player = game->LocalPlayers[0];

	if (!local_player)
	{
		return false;
	}

	const APlayerController* local_controller = local_player->PlayerController;
	if (!local_controller)
	{
		return false;
	}

	const ACharacter* local_character = local_controller->Character;

	if (!local_character)
	{
		return false;
	}

	if (const_cast<ACharacter*>(local_character)->IsLoading())
	{
		return false;
	}

	return true;
}

void engine::run_visuals(const ImDrawList* draw_list)
{
	if (!game_viewport)
	{
		return;
	}

	const ULocalPlayer* local_player = game_viewport->GameInstance->LocalPlayers[0];
	if (!local_player)
	{
		return;
	}

	player_controller = local_player->PlayerController;
	if (!player_controller)
	{
		return;
	}

	local_player_actor = static_cast<AAthenaPlayerCharacter*>(player_controller->K2_GetPawn());
	if (!local_player_actor)
	{
		return;
	}

	if (!is_ready())
	{
		return;
	}

	local_player_character = player_controller->Character;

	if (!local_player_character)
	{
		return;
	}

	const TArray<ULevel*> levels = game_viewport->World->Levels;

	for (std::uint32_t i = 0; i < levels.Count; i++)
	{
		if (!levels[i])
		{
			continue;
		}

		const TArray<ACharacter*> actors = levels[i]->AActors;
		for (std::uint32_t j = 0; j < actors.Count; j++)
		{
			const ACharacter* actor = actors[j];
			if (!actor || actor == local_player_character)
			{
				continue;
			}

			ship_esp(draw_list, actor);
			//player_esp(draw_list);
			//item_esp(draw_list);
		}
	}

	ship_hole_esp(draw_list, local_player_character);
}