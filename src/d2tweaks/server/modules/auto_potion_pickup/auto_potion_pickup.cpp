#include <d2tweaks/server/modules/auto_potion_pickup/auto_potion_pickup.h>
#include <d2tweaks/server/modules/portal_busy.h>
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
	// Native belt items use INVPAGE_NULL (0xFF). Never write INVPAGE_BELT(5) onto item_data->page.
	constexpr uint8_t k_page_null = 0xFF;
	// item_move_sc.target_page==5 is only a protocol opcode meaning "belt", not an inv page.
	constexpr uint8_t k_packet_belt_opcode = 5;
	constexpr int32_t k_node_page_storage = 1;
	constexpr int32_t k_refill_tick_interval = 25;
	// Vanilla 16-slot belt (D2Common #10269):
	//   keys 1-4 drink slots 0/1/2/3 (bottom / HUD row)
	//   column c stacks upward as c, c+4, c+8, c+12
	// Do NOT treat 0/4/8/12 as the four hotkeys — that is column 0 only.

	void sync_belt_item_minimal(diablo2::structures::unit* item) {
		if (item->mode != k_item_mode_in_belt)
			diablo2::d2_common::change_anim_mode(item, k_item_mode_in_belt);
		diablo2::d2_common::set_inv_page(item, k_page_null);
	}

	void notify_client_belt_move(diablo2::structures::unit* player, uint32_t guid, int32_t slot);

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
		// rvs/rvl = 恢复活力药水（生命+法力），不是回城卷 tsc
		if (!record)
			return false;
		const auto* code = record->string_code;
		return code_eq(code, "rvs") || code_eq(code, "rvl");
	}

	bool is_hp_or_mp(const diablo2::structures::items_line* record) {
		return is_hp(record) || is_mp(record) || is_hpo(record) || is_rejuv(record);
	}

	bool is_tsc(const diablo2::structures::items_line* record) {
		return record && code_eq(record->string_code, "tsc");
	}

	bool is_perfect_gem(const diablo2::structures::items_line* record) {
		if (!record)
			return false;
		const auto* code = record->string_code;
		// gpb/gpg/gpr/gpv/gpw/gpy = 完美蓝绿红紫白黄宝石；skz = 完美骷髅
		return code_eq(code, "gpb")
			|| code_eq(code, "gpg")
			|| code_eq(code, "gpr")
			|| code_eq(code, "gpv")
			|| code_eq(code, "gpw")
			|| code_eq(code, "gpy")
			|| code_eq(code, "skz");
	}

	bool is_charm(const diablo2::structures::items_line* record) {
		if (!record)
			return false;
		const auto* code = record->string_code;
		// cm1=小 cm2=中/大(2格) cm3=特大(3格)
		return code_eq(code, "cm1")
			|| code_eq(code, "cm2")
			|| code_eq(code, "cm3");
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

		if (cfg.pickup_tsc() && is_tsc(record))
			return true;
		if (cfg.pickup_perfect_gems() && is_perfect_gem(record))
			return true;
		if (cfg.pickup_charms() && is_charm(record))
			return true;

		return false;
	}

	void notify_client_belt_move(diablo2::structures::unit* player, uint32_t guid, int32_t slot) {
		static d2_tweaks::common::item_move_sc resp;
		resp.item_guid = guid;
		resp.target_page = k_packet_belt_opcode;
		resp.tx = static_cast<uint32_t>(slot);
		resp.ty = 0;
		singleton<d2_tweaks::server::server>::instance().send_packet(
			player->player_data->net_client, &resp, sizeof resp);
	}

	// Native GetFreeBeltSlot: same-type column first (hotkey 0-3), then a new
	// empty column if the item has AutoBelt. Never mix red/blue/purple in one column.
	// PlaceItemInBeltSlot already unlinks the item from the inventory grid.
	// Do not call refresh_inventory / update_inventory_items (enlarged backpack).
	bool try_move_to_belt(diablo2::structures::unit* player, diablo2::structures::unit* item) {
		int32_t slot = -1;
		if (!diablo2::d2_common::get_free_belt_slot(player->inventory, item, &slot))
			return false;
		if (slot < 0 || slot > 15)
			return false;

		// First potion into an emptied column lands in hotkey 0-3. Vanilla 1-4
		// often ignores that direct place; potions that later stack in 4/8/12
		// and shift down work. Route the first one through the upper cell first.
		if (slot < 4 && !diablo2::d2_common::get_item_from_belt_slot(player->inventory, slot)) {
			const auto upper = slot + 4;
			if (upper <= 15
				&& !diablo2::d2_common::get_item_from_belt_slot(player->inventory, upper)
				&& diablo2::d2_common::place_item_in_belt_slot(player->inventory, item, upper)) {
				sync_belt_item_minimal(item);
				notify_client_belt_move(player, item->guid, upper);
				if (diablo2::d2_common::place_item_in_belt_slot(player->inventory, item, slot)) {
					sync_belt_item_minimal(item);
					notify_client_belt_move(player, item->guid, slot);
				}
				return true;
			}
		}

		if (!diablo2::d2_common::place_item_in_belt_slot(player->inventory, item, slot))
			return false;

		sync_belt_item_minimal(item);
		notify_client_belt_move(player, item->guid, slot);
		return true;
	}

	void refill_belt_from_inventory(diablo2::structures::game* game, diablo2::structures::unit* player) {
		if (!player->inventory || !player->player_data || !player->player_data->net_client)
			return;

		static uint32_t s_tick;
		if ((++s_tick % static_cast<uint32_t>(k_refill_tick_interval)) != 0)
			return;

		for (auto item = player->inventory->first_item; item; item = item->item_data->pt_next_item) {
			if (!item->item_data)
				continue;

			if (item->mode != k_item_mode_stored)
				continue;
			if (item->item_data->page != k_page_inventory)
				continue;
			if (diablo2::d2_common::get_item_node_page(item) != k_node_page_storage)
				continue;

			const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
			if (!is_hp_or_mp(record))
				continue;

			if (try_move_to_belt(player, item))
				return;
		}
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
	const auto room = diablo2::d2_common::get_room_from_unit(unit);
	if (d2_tweaks::server::modules::player_busy_with_portal(unit, room))
		return;

	if (cfg.auto_potion_enabled()) {
		if (room) {
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
		}
	}

	if (cfg.refill_belt())
		refill_belt_from_inventory(game, unit);
}
