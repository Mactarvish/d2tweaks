#pragma once

#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>
#include <diablo2/structures/room.h>

namespace d2_tweaks::server::modules {
	// Hackmap Quick Back opens a town portal then walks in. Auto pickup/belt
	// refill on the same tick cancels that walk — pause while a portal we own
	// is nearby, or while the player is mid-interact.
	inline bool player_busy_with_portal(diablo2::structures::unit* player,
										diablo2::structures::room* room,
										int32_t distance_limit = 20) {
		if (!player)
			return false;

		if (player->interaction.interacting)
			return true;

		if (!room)
			return false;

		for (auto* u = room->unit; u; u = u->prev_unit_in_room) {
			if (!u)
				continue;
			if (u->type != diablo2::structures::unit_type_t::UNIT_TYPE_OBJECT)
				continue;
			if (u->owner_type != diablo2::structures::unit_type_t::UNIT_TYPE_PLAYER)
				continue;
			if (u->owner_guid != player->guid)
				continue;

			const auto distance = diablo2::d2_common::get_distance_between_units(player, u);
			if (distance <= distance_limit)
				return true;
		}

		return false;
	}
}
