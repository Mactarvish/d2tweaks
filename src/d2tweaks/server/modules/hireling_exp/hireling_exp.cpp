#include <d2tweaks/server/modules/hireling_exp/hireling_exp.h>
#include <d2tweaks/server/server.h>

#include <diablo2/d2game.h>
#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>
#include <diablo2/structures/game.h>

MODULE_INIT(hireling_exp)

namespace {
	constexpr uint32_t k_pet_type_hireling = 7;
	constexpr uint32_t k_catchup_tick_interval = 25;
	// ~1 level around clvl 17-25; later levels need more so this becomes ~1 level.
	constexpr uint32_t k_catchup_bonus = 500000;

	diablo2::structures::unit* find_hireling(diablo2::structures::game* game,
											 diablo2::structures::unit* player) {
		if (const auto merc = diablo2::d2_game::get_player_pet(game, player, k_pet_type_hireling, 0))
			return merc;

		diablo2::structures::unit* found = nullptr;
		diablo2::d2_game::iterate_unit_pets(
			game, player, [&](diablo2::structures::game*, diablo2::structures::unit*,
							  diablo2::structures::unit* pet) {
				if (!found && pet && pet->is_hireling())
					found = pet;
			});
		return found;
	}

	void give_hireling_xp(diablo2::structures::game* game,
						  diablo2::structures::unit* player,
						  diablo2::structures::unit* merc,
						  uint32_t bonus) {
		if (!game || !player || !merc || bonus == 0)
			return;

		const auto player_lvl = diablo2::d2_common::get_stat(player, diablo2::UNIT_STAT_LEVEL, 0);
		const auto merc_lvl = diablo2::d2_common::get_stat(merc, diablo2::UNIT_STAT_LEVEL, 0);
		// Native function refuses XP once merc level >= owner level.
		if (player_lvl <= 0 || merc_lvl >= player_lvl)
			return;

		diablo2::d2_game::add_experience_for_hireling(
			game, player, merc, static_cast<uint32_t>(merc_lvl), bonus);
	}

	void share_player_xp_with_hireling(diablo2::structures::game* game,
									   diablo2::structures::unit* player) {
		const auto merc = find_hireling(game, player);
		if (!merc)
			return;

		const auto player_xp = static_cast<uint32_t>(
			diablo2::d2_common::get_stat(player, diablo2::UNIT_STAT_EXPERIENCE, 0));

		static uint32_t s_guid;
		static uint32_t s_last_xp;
		static uint32_t s_catchup_tick;

		if (player->guid != s_guid) {
			s_guid = player->guid;
			s_last_xp = player_xp;
			s_catchup_tick = 0;
			return;
		}

		if (player_xp > s_last_xp) {
			give_hireling_xp(game, player, merc, player_xp - s_last_xp);
			s_last_xp = player_xp;
		} else {
			s_last_xp = player_xp;
		}

		if ((++s_catchup_tick % k_catchup_tick_interval) != 0)
			return;

		const auto player_lvl = diablo2::d2_common::get_stat(player, diablo2::UNIT_STAT_LEVEL, 0);
		const auto merc_lvl = diablo2::d2_common::get_stat(merc, diablo2::UNIT_STAT_LEVEL, 0);
		if (player_lvl > 0 && merc_lvl + 1 < player_lvl)
			give_hireling_xp(game, player, merc, k_catchup_bonus);
	}
}

void d2_tweaks::server::modules::hireling_exp::init() {
	// Disabled: over-leveling mercs produced "损坏的雇佣数据" on load.
}

void d2_tweaks::server::modules::hireling_exp::tick(diablo2::structures::game* game,
													diablo2::structures::unit* unit) {
	if (!game || !unit)
		return;
	if (unit->type != diablo2::structures::unit_type_t::UNIT_TYPE_PLAYER)
		return;

	share_player_xp_with_hireling(game, unit);
}
