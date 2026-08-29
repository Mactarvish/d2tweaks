#include <d2tweaks/client/modules/item_move/item_move.h>

#include <d2tweaks/client/client.h>

#include <d2tweaks/common/protocol.h>

#include <common/config.h>
#include <diablo2/d2client.h>
#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>
#include <diablo2/structures/item_data.h>
#include <diablo2/structures/data/items_line.h>
#include <diablo2/structures/inventory.h>
#include <common/hooking.h>

MODULE_INIT(item_move)

namespace {
	struct pending_belt_move {
		uint32_t guid;
		int32_t slot;
		uint32_t tries;
	};

	pending_belt_move g_pending_belt[8] = {};
	int32_t g_pending_belt_count = 0;

	void sync_belt_item_minimal(diablo2::structures::unit* item) {
		if (item->mode != 2)
			diablo2::d2_common::change_anim_mode(item, 2);
		diablo2::d2_common::set_inv_page(item, 0xFF);
	}

	void queue_belt_move(uint32_t guid, int32_t slot) {
		for (int32_t i = 0; i < g_pending_belt_count; ++i) {
			if (g_pending_belt[i].guid == guid) {
				g_pending_belt[i].slot = slot;
				g_pending_belt[i].tries = 0;
				return;
			}
		}
		if (g_pending_belt_count >= 8)
			return;
		g_pending_belt[g_pending_belt_count++] = {guid, slot, 0};
	}

	void play_belt_sound(diablo2::structures::unit* item) {
		const auto itemRecord = diablo2::d2_common::get_item_record(item->data_record_index);
		if (itemRecord != nullptr)
			diablo2::d2_client::play_sound(itemRecord->drop_sound, nullptr, 0, 0, 0);
		else
			diablo2::d2_client::play_sound(4, nullptr, 0, 0, 0);
	}

	bool is_belt_potion(diablo2::structures::unit* item) {
		if (!item)
			return false;
		const auto record = diablo2::d2_common::get_item_record(item->data_record_index);
		if (!record)
			return false;
		const auto* code = record->string_code;
		if (code[0] == 'h' && code[1] == 'p' && code[2] >= '1' && code[2] <= '5')
			return true;
		if (code[0] == 'm' && code[1] == 'p' && code[2] >= '1' && code[2] <= '5')
			return true;
		if (code[0] == 'h' && code[1] == 'p' && code[2] == 'o')
			return true;
		if (code[0] == 'r' && code[1] == 'v' && (code[2] == 's' || code[2] == 'l'))
			return true;
		return false;
	}

	void send_use_item(uint32_t guid) {
#pragma pack(push, 1)
		struct use_item_pkt {
			uint8_t header;
			uint32_t item_guid;
			uint32_t unk1;
			uint32_t unk2;
		};
#pragma pack(pop)
		use_item_pkt packet{};
		packet.header = 0x26;
		packet.item_guid = guid;
		diablo2::d2_client::send_to_server(&packet, sizeof packet);
	}

	// true = done (or dropped); false = retry later (slot still occupied / item missing).
	bool apply_belt_move(diablo2::structures::unit* player, diablo2::structures::unit* item, int32_t slot) {
		if (!player || !player->inventory || slot < 0 || slot > 15)
			return true;

		const auto at_slot = diablo2::d2_common::get_item_from_belt_slot(player->inventory, slot);
		if (at_slot == item) {
			sync_belt_item_minimal(item);
			return true;
		}

		// After the last potion in a column is drunk, the client slot can keep a
		// stale pointer (mode != INBELT). That blocks the first refill into 0-3.
		if (at_slot != nullptr && at_slot->mode == 2)
			return false;

		if (!diablo2::d2_common::place_item_in_belt_slot(player->inventory, item, slot))
			return false;

		sync_belt_item_minimal(item);
		play_belt_sound(item);
		return true;
	}

	// Keys 1-4 only look at hotkey cells 0-3. After a column is emptied, the
	// first refill lands in that cell but vanilla often fails to use it; later
	// potions stack in 4/8/12 and shift down, which is why those work.
	// Drink the first potion in the column by GUID so we do not depend on that.
	void handle_belt_hotkeys() {
		if (diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_CHAT)
			|| diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_NPCMENU)
			|| diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_TRADE)
			|| diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_NPCSHOP))
			return;

		const auto player = diablo2::d2_client::get_local_player();
		if (!player || !player->inventory)
			return;

		static auto& instance = singleton<d2_tweaks::client::client>::instance();
		static bool was_down[4] = {};

		for (int32_t col = 0; col < 4; ++col) {
			const bool down = (GetAsyncKeyState(static_cast<int>('1' + col)) & 0x8000) != 0;
			const bool pressed = down && !was_down[col];
			was_down[col] = down;
			if (!pressed)
				continue;

			diablo2::structures::unit* drink = nullptr;
			for (int32_t row = 0; row < 4; ++row) {
				const auto item = diablo2::d2_common::get_item_from_belt_slot(
					player->inventory, col + row * 4);
				if (item && is_belt_potion(item)) {
					drink = item;
					break;
				}
			}

			if (!drink) {
				for (int32_t i = 0; i < g_pending_belt_count; ++i) {
					if ((g_pending_belt[i].slot % 4) != col)
						continue;
					drink = instance.get_client_unit(0x04, g_pending_belt[i].guid);
					if (drink)
						break;
					send_use_item(g_pending_belt[i].guid);
					drink = nullptr;
					break;
				}
			}

			if (!drink)
				continue;

			sync_belt_item_minimal(drink);
			send_use_item(drink->guid);
		}
	}

	void sync_hotkey_modes(diablo2::structures::unit* player) {
		if (!player || !player->inventory)
			return;

		for (int32_t col = 0; col < 4; ++col) {
			const auto item = diablo2::d2_common::get_item_from_belt_slot(player->inventory, col);
			if (!item)
				continue;
			if (item->mode != 2 || !item->item_data || item->item_data->page != 0xFF)
				sync_belt_item_minimal(item);
		}
	}

	void flush_pending_belt_moves() {
		static auto& instance = singleton<d2_tweaks::client::client>::instance();
		const auto player = diablo2::d2_client::get_local_player();
		if (!player || !player->inventory)
			return;

		int32_t write = 0;
		for (int32_t i = 0; i < g_pending_belt_count; ++i) {
			auto& pending = g_pending_belt[i];
			const auto item = instance.get_client_unit(0x04, pending.guid);
			if (item && apply_belt_move(player, item, pending.slot))
				continue;

			if (++pending.tries > 80)
				continue;

			g_pending_belt[write++] = pending;
		}
		g_pending_belt_count = write;
	}
}

int32_t(__fastcall* g_item_click_original)(diablo2::structures::unit* playerUnit, diablo2::structures::inventory* inventory, int mouse_x, int mouse_y, uint8_t flag, void* a6, unsigned int page);

char get_target_page(char currentPage) {
	if (currentPage == 0) { //item is in inventory
		if (diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_STASH))
			return 4;

		if (diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_CUBE))
			return 3;
	}

	return 0;
}

void request_item_move(diablo2::structures::unit* item, char targetPage) {
	static d2_tweaks::common::item_move_cs packet;

	packet.item_guid = item->guid;
	packet.target_page = targetPage;

	diablo2::d2_client::send_to_server(&packet, sizeof packet);
}

int32_t __fastcall item_click(diablo2::structures::unit* owner, diablo2::structures::inventory* inventory, int mouse_x, int mouse_y, uint8_t flag, char* a6, unsigned int page) {
	const auto player = diablo2::d2_client::get_local_player();

	if (owner->guid != player->guid)
		return g_item_click_original(owner, inventory, mouse_x, mouse_y, flag, a6, page);

	if ((static_cast<uint16_t>(GetAsyncKeyState(VK_CONTROL)) >> 8 & 0x80u) == 0)
		return g_item_click_original(owner, inventory, mouse_x, mouse_y, flag, a6, page);

	if (!diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_CUBE) &&
		!diablo2::d2_client::get_ui_window_state(diablo2::UI_WINDOW_STASH)) {
		return g_item_click_original(owner, inventory, mouse_x, mouse_y, flag, a6, page);
	}

	//code below taken from IDA directly, so that's why it is so ugly
	const auto coefx1 = *(reinterpret_cast<uint32_t*>(a6) + 1);
	const auto coefx2 = static_cast<unsigned int>(a6[20]);

	const auto coefy1 = *(reinterpret_cast<uint32_t*>(a6) + 3);
	const auto coefy2 = static_cast<unsigned int>(a6[21]);

	const auto itemx = (mouse_x - coefx1) / coefx2;
	const auto itemy = (mouse_y - coefy1) / coefy2;

	diablo2::structures::unit* cubeItem = nullptr;

	uint32_t px, py;

	const auto currentInventoryIndex = diablo2::d2_common::get_inventory_index(player, page, diablo2::d2_client::is_lod());
	const auto clickedItem = diablo2::d2_common::get_item_at_cell(player->inventory, itemx, itemy, &px, &py, currentInventoryIndex, page);

	for (auto item = player->inventory->first_item; item != nullptr; item = item->item_data->pt_next_item) {
		auto record = diablo2::d2_common::get_item_record(item->data_record_index);

		if (record->string_code[0] == 'b' &&
			record->string_code[1] == 'o' &&
			record->string_code[2] == 'x') { //Cube
			cubeItem = item;
			break;
		}
	}

	if (clickedItem == nullptr) {
		return g_item_click_original(owner, inventory, mouse_x, mouse_y, flag, a6, page);
	}

	const auto targetPage = get_target_page(page);

	if (targetPage == 0x03 && clickedItem == cubeItem)
		return g_item_click_original(owner, inventory, mouse_x, mouse_y, flag, a6, page);

	request_item_move(clickedItem, targetPage);
	return 0;
}

void d2_tweaks::client::modules::item_move::init() {
	hooking::hook(diablo2::d2_client::get_base() + 0x475C0, item_click, reinterpret_cast<void**>(&g_item_click_original));
	singleton<client>::instance().register_packet_handler(common::MESSAGE_TYPE_ITEM_MOVE, this);
	singleton<client>::instance().register_tick_handler(this);
}

void d2_tweaks::client::modules::item_move::tick() {
	const auto& cfg = singleton<config>::instance();
	if (!cfg.refill_belt() && !cfg.auto_potion_enabled())
		return;

	handle_belt_hotkeys();
	flush_pending_belt_moves();

	static uint32_t s_tick;
	if ((++s_tick % 4) != 0)
		return;

	sync_hotkey_modes(diablo2::d2_client::get_local_player());
}

void d2_tweaks::client::modules::item_move::handle_packet(common::packet_header* packet) {
	static auto& instance = singleton<client>::instance();

	const auto itemMove = static_cast<common::item_move_sc*>(packet);
	const auto item = instance.get_client_unit(0x04, itemMove->item_guid); //0x03 -> 0x04 - item

	const auto player = diablo2::d2_client::get_local_player();
	if (!player || !player->inventory)
		return;

	// target_page==5: server placed this item into belt slot tx.
	// If the hotkey cell is still occupied (drink packet not applied yet), retry next ticks.
	if (itemMove->target_page == 5) {
		const auto slot = static_cast<int32_t>(itemMove->tx);
		if (!item || !apply_belt_move(player, item, slot))
			queue_belt_move(itemMove->item_guid, slot);
		return;
	}

	if (item == nullptr)
		return;

	//Last parameter is some boolean
	const auto inventoryIndex = diablo2::d2_common::get_inventory_index(player, itemMove->target_page, diablo2::d2_client::is_lod());

	item->item_data->page = itemMove->target_page;

	diablo2::d2_common::inv_add_item(player->inventory, item, itemMove->tx, itemMove->ty, inventoryIndex, true, item->item_data->page);
	diablo2::d2_common::inv_update_item(player->inventory, item, true);

	const auto itemRecord = diablo2::d2_common::get_item_record(item->data_record_index);

	if (itemRecord != nullptr)
		diablo2::d2_client::play_sound(itemRecord->drop_sound, nullptr, 0, 0, 0);
	else
		diablo2::d2_client::play_sound(4, nullptr, 0, 0, 0);
}
