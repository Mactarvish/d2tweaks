#include <d2tweaks/server/modules/auto_potion_pickup/auto_potion_pickup.h>
#include <d2tweaks/server/server.h>

#include <d2tweaks/common/protocol.h>
#include <common/config.h>

#include <diablo2/d2game.h>
#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>
#include <diablo2/structures/game.h>
#include <diablo2/structures/room.h>
#include <diablo2/structures/inventory.h>
#include <diablo2/structures/item_data.h>
#include <diablo2/structures/player_data.h>
#include <diablo2/structures/data/items_line.h>

#include <vector>

MODULE_INIT(auto_potion_pickup)

namespace {
	constexpr uint32_t k_item_mode_stored = 0;
	constexpr uint32_t k_item_mode_in_belt = 2;
	constexpr uint32_t k_item_mode_on_ground = 3;
	constexpr uint8_t k_page_inventory = 0;
	constexpr uint8_t k_packet_page_belt = 5; // protocol flag only
	constexpr int32_t k_node_page_storage = 1;
	constexpr int32_t k_node_page_belt = 2;

	bool code_eq(const char* code, const char* expected) {
		return code[0] == expected[0]
			&& code[1] == expected[1]
			&& code[2] == expected[2];
	}

	int32_t rune_number(const char* code) {
		if (code[0] != 'r'
			|| code[1] < '0' || code[1] > '9'
			|| code[2] < '0' || code[2] > '9') {
			return -1;
		}
		return (code[1] - '0') * 10 + (code[2] - '0');
	}

	bool is_hp(const diablo2::structures::items_line* record) {
		if (!record)
			return false;
		const auto* code = record->string_code;
		return code[0] == 'h' && code[1] == 'p'
			&& code[2] >= '1' && code[2] <= '5';
	}

	bool is_mp(const diablo2::structures::items_line* record) {
		if (!record)
			return false;
		const auto* code = record->string_code;
		return code[0] == 'm' && code[1] == 'p'
			&& code[2] >= '1' && code[2] <= '5';
	}

	bool is_hpo(const diablo2::structures::items_line* record) {
		return record && code_eq(record->string_code, "hpo");
	}

	bool is_rejuv(const diablo2::structures::items_line* record) {
		if (!record)
			return false;
		const auto* code = record->string_code;
		return code_eq(code, "rvs") || code_eq(code, "rvl");
	}

	bool is_hp_or_mp(const diablo2::structures::items_line* record) {
		return is_hp(record) || is_mp(record) || is_hpo(record);
	}

	bool should_auto_pickup(const diablo2::structures::items_line* record, const config& cfg) {
		if (!record)
			return false;

		if (cfg.pickup_hp() && is_hp(record))
			return true;
		if (cfg.pickup_mp() && is_mp(record))
			return true;
		if (cfg.pickup_hpo() && is_hpo(record))
			return true;
		if (cfg.pickup_rejuv() && is_rejuv(record))
			return true;

		if (cfg.pickup_runes()) {
			const auto n = rune_number(record->string_code);
			if (n >= cfg.rune_min() && n <= cfg.rune_max())
				return true;
		}

		return false;
	}

	bool find_inventory_space(diablo2::structures::game* game,
							  diablo2::structures::unit* player,
							  diablo2::structures::unit* item,
							  uint32_t& x, uint32_t& y) {
		const auto inventory_index = diablo2::d2_common::get_inventory_index(
			player, k_page_inventory, game->item_format == 101);

		char data[0x18];
		diablo2::d2_common::get_inventory_data(inventory_index, 0, data);
		const auto mx = static_cast<uint32_t>(data[0]);
		const auto my = static_cast<uint32_t>(data[1]);

		for (x = 0; x < mx; x++) {
			for (y = 0; y < my; y++) {
				diablo2::structures::unit* blocking = nullptr;
				uint32_t blocking_index = 0;
				if (diablo2::d2_common::can_put_into_slot(
						player->inventory, item, x, y, inventory_index,
						&blocking, &blocking_index, k_page_inventory)) {
					return true;
				}
			}
		}
		return false;
	}

	void send_item_move(diablo2::structures::unit* player,
						uint32_t guid, uint8_t page, uint32_t tx, uint32_t ty) {
		static d2_tweaks::common::item_move_sc resp;
		resp.item_guid = guid;
		resp.target_page = page;
		resp.tx = tx;
		resp.ty = ty;
		singleton<d2_tweaks::server::server>::instance().send_packet(
			player->player_data->net_client, &resp, sizeof resp);
	}

	bool place_into_inventory(diablo2::structures::game* game,
							  diablo2::structures::unit* player,
							  diablo2::structures::unit* item) {
		uint32_t x = 0, y = 0;
		if (!find_inventory_space(game, player, item, x, y))
			return false;

		const auto inventory_index = diablo2::d2_common::get_inventory_index(
			player, k_page_inventory, game->item_format == 101);

		diablo2::d2_common::set_inv_page(item, k_page_inventory);
		item->mode = k_item_mode_stored;
		item->item_data->page = k_page_inventory;

		if (!diablo2::d2_common::inv_add_item(
				player->inventory, item, x, y, inventory_index, false, k_page_inventory)) {
			return false;
		}

		diablo2::d2_common::inv_update_item(player->inventory, item, false);
		send_item_move(player, item->guid, k_page_inventory, x, y);
		return true;
	}

	bool try_move_to_belt(diablo2::structures::unit* player, diablo2::structures::unit* item) {
		int32_t slot = -1;
		if (!diablo2::d2_common::get_free_belt_slot(player->inventory, item, &slot))
			return false;

		// PlaceItemInBeltSlot already removes the item from its previous grid.
		if (!diablo2::d2_common::place_item_in_belt_slot(player->inventory, item, slot))
			return false;

		item->mode = k_item_mode_in_belt;

		// Belt items should not keep INVPAGE_BELT(5); native code leaves page alone for belt grid.
		if (item->item_data->page == k_packet_page_belt)
			diablo2::d2_common::set_inv_page(item, k_page_inventory);

		send_item_move(player, item->guid, k_packet_page_belt,
					   static_cast<uint32_t>(slot), 0);
		return true;
	}

	// Recover potions stuck by earlier buggy refill (page==5 / wrong node page).
	void recover_stuck_potions(diablo2::structures::game* game, diablo2::structures::unit* player) {
		std::vector<diablo2::structures::unit*> stuck;

		for (auto item = player->inventory->first_item; item; item = item->item_data->pt_next_item) {
			if (!item->item_data)
				continue;

			const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
			if (!is_hp_or_mp(record))
				continue;

			const auto node_page = diablo2::d2_common::get_item_node_page(item);
			const auto bad_page = item->item_data->page == k_packet_page_belt;
			const auto stranded = item->mode != k_item_mode_in_belt
				&& item->mode != k_item_mode_stored
				&& item->mode != k_item_mode_on_ground;
			const auto page5_or_orphaned = bad_page
				|| (item->mode == k_item_mode_stored && node_page != k_node_page_storage && node_page != k_node_page_belt);

			if (bad_page || stranded || page5_or_orphaned)
				stuck.push_back(item);
		}

		for (auto* item : stuck) {
			if (try_move_to_belt(player, item))
				continue;

			place_into_inventory(game, player, item);
		}
	}

	void refill_belt_from_inventory(diablo2::structures::game* game, diablo2::structures::unit* player) {
		if (!player->inventory || !player->player_data || !player->player_data->net_client)
			return;

		recover_stuck_potions(game, player);

		std::vector<diablo2::structures::unit*> potions;

		for (auto item = player->inventory->first_item; item; item = item->item_data->pt_next_item) {
			if (!item->item_data)
				continue;

			// Only backpack-stored potions (not already on belt).
			if (item->mode != k_item_mode_stored)
				continue;
			if (item->item_data->page != k_page_inventory)
				continue;
			if (diablo2::d2_common::get_item_node_page(item) != k_node_page_storage)
				continue;

			const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
			if (!is_hp_or_mp(record))
				continue;

			potions.push_back(item);
		}

		bool moved_any = false;
		for (auto* item : potions) {
			if (try_move_to_belt(player, item))
				moved_any = true;
		}

		if (moved_any)
			diablo2::d2_game::update_inventory_items(game, player);
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

	const auto& cfg = singleton<config>::instance();
	if (!cfg.auto_potion_enabled())
		return;

	const auto room = diablo2::d2_common::get_room_from_unit(unit);
	if (!room)
		return;

	std::vector<uint32_t> guids;
	const auto distance_limit = cfg.auto_potion_distance();

	for (auto item = room->unit; item; item = item->prev_unit_in_room) {
		if (!item)
			continue;

		if (item->type != diablo2::structures::unit_type_t::UNIT_TYPE_ITEM)
			continue;

		if (item->mode != k_item_mode_on_ground)
			continue;

		const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
		if (!should_auto_pickup(record, cfg))
			continue;

		const auto distance = diablo2::d2_common::get_distance_between_units(unit, item);
		if (distance > distance_limit)
			continue;

		guids.push_back(item->guid);
	}

	for (const auto guid : guids) {
		uint32_t item_carried = 0;
		diablo2::d2_game::pickup_item(game, unit, guid, &item_carried);
	}

	if (cfg.refill_belt())
		refill_belt_from_inventory(game, unit);
}
