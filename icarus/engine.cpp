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

	void* gworld = static_cast<void*>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "48 8B 0D ? ? ? ? 4C 8B CB 4C 8B C7"), 3, 7));
	void* gobjects = static_cast<void*>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "89 0D ? ? ? ? 48 8B DF"), 2, 6));
	void* gnames = static_cast<void*>(utilities::resolve_rip(utilities::pattern_scan(nullptr, "48 89 1D ? ? ? ? 8B C3"), 3, 7));

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

static void ship_esp(ImDrawList* draw_list, ACharacter* actor)
{
	if (!config::context.ship_esp)
	{
		return;
	}

	if (!actor)
	{
		return;
	}

	if (actor->isShip() || actor->isFarShip())
	{
		FVector local_player_location = local_player_character->K2_GetActorLocation();
		FVector location = actor->K2_GetActorLocation();

		float distance = local_player_location.DistTo(location) * 0.01f;
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

		float velocity = (actor->GetVelocity() * 0.01f).Size();

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
			draw_list->AddText(nullptr, 20.f, ImVec2(screen.X, screen.Y), utilities::color_to_value(config::context.ship_text_color), buffer);
		}
	}
}

static void ship_hole_esp(ImDrawList* draw_list)
{
	if (!config::context.draw_ship_holes)
	{
		return;
	}

	AAthenaPlayerCharacter* lp = ((AAthenaPlayerCharacter*)player_controller->K2_GetPawn());

	if (!lp || lp->IsDead())
	{
		return;
	}

	ACharacter* ship = lp->GetCurrentShip();

	if (!ship || !ship->isShip())
	{
		return;
	}

	AHullDamage* damage = ship->GetHullDamage();

	if (!damage)
	{
		return;
	}

	TArray<ACharacter*> holes = damage->ActiveHullDamageZones;

	for (std::uint32_t i = 0; i < holes.Count; i++)
	{
		ACharacter* hole = holes[i];
		FVector location = hole->K2_GetActorLocation();

		FVector2D screen = { };
		if (player_controller->ProjectWorldLocationToScreen(location, screen))
		{
			continue;
		}

		draw_list->AddLine(ImVec2(screen.X - 6.f, screen.Y + 6.f), ImVec2(screen.X + 6.f, screen.Y - 6.f), utilities::color_to_value(config::context.ship_hole_color));
		draw_list->AddLine(ImVec2(screen.X - 6.f, screen.Y - 6.f), ImVec2(screen.X + 6.f, screen.Y + 6.f), utilities::color_to_value(config::context.ship_hole_color));
	}
}

static void player_esp(ImDrawList* draw_list, ACharacter* actor)
{
	if (!config::context.player_esp)
	{
		return;
	}

	if (!actor || !actor->isPlayer() || actor->IsDead())
	{
		return;
	}

	if (UCrewFunctions::AreCharactersInSameCrew(actor, local_player_actor) && !config::context.teammate_esp)
	{
		return;
	}

	FVector local_player_location = ((AAthenaPlayerCharacter*)player_controller->K2_GetPawn())->K2_GetActorLocation();
	FVector location = actor->K2_GetActorLocation();

	float distance = local_player_location.DistTo(location) * 0.01f;
	if (distance > config::context.player_render_distance)
	{
		return;
	}

	FVector origin, extent = { };
	actor->GetActorBounds(true, origin, extent);

	FVector2D head = { };
	if (!player_controller->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z + extent.Z }, head))
	{
		return;
	}

	FVector2D foot = { };
	if (!player_controller->ProjectWorldLocationToScreen({ location.X, location.Y, location.Z - extent.Z }, foot))
	{
		return;
	}

	float height = std::abs(foot.Y - head.Y);
	float width = height * 0.4f;

	if (config::context.draw_box)
	{
		draw_list->AddRect({ head.X - width * 0.5f, head.Y }, { head.X + width * 0.5f, foot.Y }, utilities::color_to_value(config::context.box_color), 0.f, 15, 1.5f);
	}
}

static void item_esp(ImDrawList* draw_list, ACharacter* actor)
{

}

static void hud_indicators(ImDrawList* draw_list)
{
	if (!config::context.hud_indicators)
	{
		return;
	}

	AAthenaPlayerCharacter* local_player = ((AAthenaPlayerCharacter*)player_controller->K2_GetPawn());

	if (!local_player || !local_player->IsDead())
	{
		return;
	}

	ACharacter* ship = local_player->GetCurrentShip();

	if (!ship)
	{
		return;
	}

	float velocity = (ship->GetVelocity() * 0.01f).Size();
	std::uint32_t holes = ship->GetHullDamage()->ActiveHullDamageZones.Count;
	float internal_water = ship->GetInternalWater()->GetNormalizedWaterAmount() * 100.f;

	ImVec4 color = { 255.f, 0.f, 0.f, 255.f };
	FVector2D screen = { 40, 100 };
	char buffer[MAX_PATH];

	if (config::context.hud_velocity)
	{
		sprintf_s(buffer, "Velocity: %.0fm/s", velocity);
		draw_list->AddText(nullptr, 15.f, ImVec2(screen.X, screen.Y), ImGui::ColorConvertFloat4ToU32(color), buffer);
		screen.Y += 20.f;
	}

	if (config::context.hud_holes)
	{
		sprintf_s(buffer, "Holes: %u", holes);
		draw_list->AddText(nullptr, 15.f, ImVec2(screen.X, screen.Y), ImGui::ColorConvertFloat4ToU32(color), buffer);
		screen.Y += 20.f;
	}

	if (config::context.hud_water)
	{
		sprintf_s(buffer, "Water: %.0f%%", internal_water);
		draw_list->AddText(nullptr, 15.f, ImVec2(screen.X, screen.Y), ImGui::ColorConvertFloat4ToU32(color), buffer);
	}
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

	UWorld* world = *UWorld::GWorld;
	if (!world)
	{
		return false;
	}

	UGameInstance* game = world->GameInstance;
	if (!game)
	{
		return false;
	}

	ULocalPlayer* local_player = game->LocalPlayers[0];

	if (!local_player)
	{
		return false;
	}

	APlayerController* local_controller = local_player->PlayerController;
	if (!local_controller)
	{
		return false;
	}

	ACharacter* local_character = local_controller->Character;

	if (!local_character)
	{
		return false;
	}

	if (local_character->IsLoading())
	{
		return false;
	}

	return true;
}

void engine::run_visuals(ImDrawList* draw_list)
{
	if (!game_viewport)
	{
		return;
	}

	ULocalPlayer* local_player = game_viewport->GameInstance->LocalPlayers[0];
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

	TArray<ULevel*> levels = game_viewport->World->Levels;
	for (std::uint32_t i = 0; i < levels.Count; i++)
	{
		if (!levels[i])
		{
			continue;
		}

		TArray<ACharacter*> actors = levels[i]->AActors;
		for (std::uint32_t j = 0; j < actors.Count; j++)
		{
			ACharacter* actor = actors[j];
			if (!actor || actor == local_player_character)
			{
				continue;
			}

			ship_esp(draw_list, actor);
			player_esp(draw_list, actor);
			//item_esp(draw_list);
		}
	}

	ship_hole_esp(draw_list);
	hud_indicators(draw_list);
}