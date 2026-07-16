#include <d2tweaks/server/modules/auto_potion_pickup/auto_potion_pickup.h>
#include <d2tweaks/server/server.h>

#include <diablo2/d2game.h>
#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>
#include <diablo2/structures/room.h>
#include <diablo2/structures/data/items_line.h>

#include <vector>

MODULE_INIT(auto_potion_pickup)

namespace {
	constexpr int32_t k_pickup_distance = 4;
	constexpr uint32_t k_item_mode_on_ground = 3;

	bool code_eq(const char* code, const char* expected) {
		return code[0] == expected[0]
			&& code[1] == expected[1]
			&& code[2] == expected[2];
	}

	bool is_heal_or_rejuv(const diablo2::structures::items_line* record) {
		if (!record)
			return false;

		const auto* code = record->string_code;

		// hp1-hp5 red potions (no mp blue potions)
		if (code[0] == 'h' && code[1] == 'p'
			&& code[2] >= '1' && code[2] <= '5') {
			return true;
		}

		// hpo = 浓缩生命药水 (legacy/unused code used by some mods)
		if (code_eq(code, "hpo"))
			return true;

		// rvs / rvl (purple rejuv)
		if (code_eq(code, "rvs") || code_eq(code, "rvl"))
			return true;

		return false;
	}

	bool is_rune(const diablo2::structures::items_line* record) {
		if (!record)
			return false;

		const auto* code = record->string_code;
		// r01-r33 (and any mod r##_ style rune code)
		return code[0] == 'r'
			&& code[1] >= '0' && code[1] <= '9'
			&& code[2] >= '0' && code[2] <= '9';
	}

	bool should_auto_pickup(const diablo2::structures::items_line* record) {
		return is_heal_or_rejuv(record) || is_rune(record);
	}
}

void d2_tweaks::server::modules::auto_potion_pickup::init() {
	singleton<server>::instance().register_tick_handler(this);
}

void d2_tweaks::server::modules::auto_potion_pickup::tick(diablo2::structures::game* game,
														  diablo2::structures::unit* unit) {
	if (!game || !unit)
		return;

	if (unit->type != diablo2::structures::unit_type_t::UNIT_TYPE_PLAYER)
		return;

	const auto room = diablo2::d2_common::get_room_from_unit(unit);
	if (!room)
		return;

	std::vector<uint32_t> guids;

	for (auto item = room->unit; item; item = item->prev_unit_in_room) {
		if (!item)
			continue;

		if (item->type != diablo2::structures::unit_type_t::UNIT_TYPE_ITEM)
			continue;

		if (item->mode != k_item_mode_on_ground)
			continue;

		const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
		if (!should_auto_pickup(record))
			continue;

		const auto distance = diablo2::d2_common::get_distance_between_units(unit, item);
		if (distance > k_pickup_distance)
			continue;

		guids.push_back(item->guid);
	}

	for (const auto guid : guids) {
		uint32_t item_carried = 0;
		diablo2::d2_game::pickup_item(game, unit, guid, &item_carried);
	}
}
