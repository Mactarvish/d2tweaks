#pragma once

#include <cstring>

#include <diablo2/d2common.h>
#include <diablo2/structures/player_data.h>
#include <diablo2/structures/skill_list.h>
#include <diablo2/structures/unit.h>

namespace d2_tweaks::common::attr_boost_common {
	inline bool name_matches(diablo2::structures::unit* unit, const char* want) {
		if (!unit || !unit->player_data || !want || !want[0])
			return false;
		return _stricmp(unit->player_data->name, want) == 0;
	}

	inline void apply_elemental_resists(diablo2::structures::unit* unit, uint32_t value) {
		if (!unit)
			return;

		constexpr diablo2::unit_stats_t resists[] = {
			diablo2::UNIT_STAT_FIRERESIST,
			diablo2::UNIT_STAT_LIGHTRESIST,
			diablo2::UNIT_STAT_COLDRESIST,
			diablo2::UNIT_STAT_POISONRESIST,
		};
		constexpr diablo2::unit_stats_t max_resists[] = {
			diablo2::UNIT_STAT_MAXFIRERESIST,
			diablo2::UNIT_STAT_MAXLIGHTRESIST,
			diablo2::UNIT_STAT_MAXCOLDRESIST,
			diablo2::UNIT_STAT_MAXPOISONRESIST,
		};

		for (const auto stat : resists)
			diablo2::d2_common::set_stat(unit, stat, value, 0);
		for (const auto stat : max_resists)
			diablo2::d2_common::set_stat(unit, stat, value, 0);
	}

	inline void apply_armor_class(diablo2::structures::unit* unit, uint32_t value) {
		if (!unit)
			return;
		diablo2::d2_common::set_stat(unit, diablo2::UNIT_STAT_ARMORCLASS, value, 0);
	}

	inline void apply_hit_points(diablo2::structures::unit* unit, uint32_t display_hp) {
		if (!unit || display_hp <= 0)
			return;

		// 游戏内生命以 256 为精度存储；面板显示值 = internal / 256
		const uint32_t internal = display_hp * 256u;
		diablo2::d2_common::set_stat(unit, diablo2::UNIT_STAT_MAXHP, internal, 0);
		diablo2::d2_common::set_stat(unit, diablo2::UNIT_STAT_HITPOINTS, internal, 0);
	}

	inline void apply_skill_levels(diablo2::structures::unit* unit, uint32_t level) {
		if (!unit || !unit->skills || level <= 0)
			return;

		auto* list = reinterpret_cast<diablo2::structures::skill_list*>(unit->skills);
		if (!list || !list->first_skill)
			return;

		constexpr uint32_t native_owner = 0xFFFFFFFF;
		bool changed = false;

		for (auto* sk = list->first_skill; sk; sk = sk->next_skill) {
			if (sk->owner_guid != native_owner)
				continue;

			const auto id = diablo2::d2_common::get_skill_id(sk);
			if (id <= 0)
				continue;

			if (static_cast<uint32_t>(sk->skill_level) == level)
				continue;

			diablo2::d2_common::assign_skill(unit, static_cast<uint32_t>(id), level, FALSE, nullptr, 0);
			changed = true;
		}

		if (changed)
			diablo2::d2_common::refresh_passive_skills(unit);
	}
}
