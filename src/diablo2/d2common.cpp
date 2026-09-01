#include <diablo2/d2common.h>
#include <common/ptr_wrapper.h>

char* diablo2::d2_common::get_base() {
	static auto base = reinterpret_cast<char*>(GetModuleHandle("d2common.dll"));
	return base;
}

int32_t diablo2::d2_common::get_inventory_index(structures::unit* item, char page, BOOL lod) {
	static wrap_func_std_import<int32_t(structures::unit*, char, BOOL)> get_inventory_index(10409, get_base());
	return get_inventory_index(item, page, lod);
}

void* diablo2::d2_common::get_inventory_data(int32_t index, int32_t zero, char* data) {
	static wrap_func_std_import<void* (int32_t, int32_t, char*)> get_inventory_data(10636, get_base());
	return get_inventory_data(index, zero, data);
}

diablo2::structures::unit* diablo2::d2_common::get_item_at_cell(structures::inventory* inv, uint32_t cellx,
																uint32_t celly, uint32_t* pcellx, uint32_t* pcelly, int32_t invIndex, uint8_t page) {
	static wrap_func_std_import<structures::unit * (structures::inventory*, uint32_t, uint32_t, uint32_t*, uint32_t*,
													int32_t, uint8_t)> get_item_at_cell(10252, get_base());
	return get_item_at_cell(inv, cellx, celly, pcellx, pcelly, invIndex, page);
}

uint32_t diablo2::d2_common::can_put_into_slot(structures::inventory* inv, structures::unit* item, uint32_t x,
											   uint32_t y, uint32_t invIndex, structures::unit** lastBlockingUnit, uint32_t* lastBlockingUnitIndex, uint8_t page) {
	static wrap_func_std_import< uint32_t(structures::inventory*, structures::unit*, uint32_t, uint32_t,
										  uint32_t, structures::unit**, uint32_t*, uint8_t)> can_put_into_slot(
											  10247, get_base());
	return can_put_into_slot(inv, item, x, y, invIndex, lastBlockingUnit, lastBlockingUnitIndex, page);
}

diablo2::structures::unit* diablo2::d2_common::inv_remove_item(structures::inventory* inventory, structures::unit* item) {
	static wrap_func_std_import<structures::unit * (structures::inventory*, structures::unit*)> inv_remove_item(10243, get_base());
	return inv_remove_item(inventory, item);
}

BOOL diablo2::d2_common::inv_add_item(structures::inventory* inv, structures::unit* item, uint32_t x, uint32_t y,
									  uint32_t invIndex, BOOL isClient, uint8_t page) {
	static wrap_func_std_import<BOOL(structures::inventory*, structures::unit*, uint32_t, uint32_t,
									 uint32_t, BOOL, uint8_t)> inv_add_item(10249, get_base());

	return inv_add_item(inv, item, x, y, invIndex, isClient, page);
}

BOOL diablo2::d2_common::inv_update_item(structures::inventory* inv, structures::unit* item, BOOL isClient) {
	static wrap_func_std_import<BOOL(structures::inventory*, structures::unit*, BOOL)> inv_update_item(10242, get_base());
	return inv_update_item(inv, item, isClient);
}

BOOL diablo2::d2_common::get_free_belt_slot(structures::inventory* inv, structures::unit* item, int32_t* free_slot) {
	static wrap_func_std_import<BOOL(structures::inventory*, structures::unit*, int32_t*)> get_free_belt_slot(10269, get_base());
	return get_free_belt_slot(inv, item, free_slot);
}

BOOL diablo2::d2_common::place_item_in_belt_slot(structures::inventory* inv, structures::unit* item, int32_t slot) {
	static wrap_func_std_import<BOOL(structures::inventory*, structures::unit*, int32_t)> place_item_in_belt_slot(10266, get_base());
	return place_item_in_belt_slot(inv, item, slot);
}

diablo2::structures::unit* diablo2::d2_common::get_item_from_belt_slot(structures::inventory* inv, int32_t slot) {
	static wrap_func_std_import<structures::unit * (structures::inventory*, int32_t)> get_item_from_belt_slot(10271, get_base());
	return get_item_from_belt_slot(inv, slot);
}

void diablo2::d2_common::set_inv_page(structures::unit* item, uint8_t page) {
	static wrap_func_std_import<void(structures::unit*, uint8_t)> set_inv_page(10720, get_base());
	set_inv_page(item, page);
}

int32_t diablo2::d2_common::get_item_node_page(structures::unit* item) {
	static wrap_func_std_import<int32_t(structures::unit*)> get_item_node_page(10307, get_base());
	return get_item_node_page(item);
}

BOOL diablo2::d2_common::change_anim_mode(structures::unit* unit, uint32_t mode) {
	static wrap_func_std_import<BOOL(structures::unit*, uint32_t)> change_anim_mode(10348, get_base());
	return change_anim_mode(unit, mode);
}

void diablo2::d2_common::set_item_cmd_flag(structures::unit* item, int32_t flag, BOOL set) {
	static wrap_func_std_import<void(structures::unit*, int32_t, BOOL)> set_item_cmd_flag(10711, get_base());
	set_item_cmd_flag(item, flag, set);
}

BOOL diablo2::d2_common::add_item_to_trade_inventory(structures::inventory* inv, structures::unit* item) {
	static wrap_func_std_import<BOOL(structures::inventory*, structures::unit*)> add_item_to_trade_inventory(10283, get_base());
	return add_item_to_trade_inventory(inv, item);
}

void diablo2::d2_common::refresh_inventory(structures::unit* unit, BOOL set_flag) {
	static wrap_func_std_import<void(structures::unit*, BOOL)> refresh_inventory(10357, get_base());
	refresh_inventory(unit, set_flag);
}

diablo2::structures::items_line* diablo2::d2_common::get_item_record(uint32_t guid) {
	static wrap_func_std_import<structures::items_line * (uint32_t)> get_item_record(10600, get_base());
	return get_item_record(guid);
}

diablo2::structures::item_types_line* diablo2::d2_common::get_item_type_record(uint32_t typeId) {
	static wrap_func_fast<structures::item_types_line * (uint32_t)>get_item_type_record(0x2B1A0, get_base());
	return get_item_type_record(typeId);
}

uint32_t diablo2::d2_common::get_maximum_character_gold(structures::unit* player) {
	static wrap_func_std_import<uint32_t(structures::unit*)> get_maximum_character_gold(10439, get_base());
	return get_maximum_character_gold(player);
}

int32_t diablo2::d2_common::set_stat(structures::unit* unit, unit_stats_t stat, uint32_t value, int16_t param) {
	static wrap_func_std_import<int32_t(structures::unit*, int32_t, int32_t, int32_t)> set_stat(10517, get_base());
	return set_stat(unit, stat, value, param);
}

int32_t diablo2::d2_common::get_stat(structures::unit* unit, unit_stats_t stat, int16_t param) {
	static wrap_func_std_import<int32_t(structures::unit*, int32_t, int32_t)> get_stat(10519, get_base());
	return get_stat(unit, stat, param);
}

int32_t diablo2::d2_common::get_stat_signed(structures::unit* unit, unit_stats_t stat, int16_t param) {
	static wrap_func_std_import<int32_t(structures::unit*, int32_t, int32_t)> get_stat_signed(10520, get_base());
	return get_stat_signed(unit, stat, param);
}

int32_t diablo2::d2_common::get_base_stat(structures::unit* unit, unit_stats_t stat, int16_t param) {
	static wrap_func_std_import<int32_t(structures::unit*, int32_t, int32_t)> get_base_stat(10521, get_base());
	return get_base_stat(unit, stat, param);
}

int32_t diablo2::d2_common::_10111(int32_t* x, int32_t* y) {
	static wrap_func_std_import<int32_t(int32_t*, int32_t*)> get_unk_coords(10111, get_base());
	return get_unk_coords(x, y);
}

int32_t diablo2::d2_common::_10116(int32_t x1, int32_t y1, int32_t* x, int32_t* y) {
	static wrap_func_std_import<int32_t(int32_t, int32_t, int32_t*, int32_t*)> get_unk_coords2(10116, get_base());
	return get_unk_coords2(x1, y1, x, y);
}

diablo2::structures::room* diablo2::d2_common::get_room_from_unit(structures::unit* unit) {
	static wrap_func_std_import<structures::room * (structures::unit*)> get_room_from_unit(10342, get_base());
	return get_room_from_unit(unit);
}

int32_t diablo2::d2_common::get_unit_size_x(structures::unit* unit) {
	static wrap_func_std_import<int32_t(structures::unit*)> get_unit_size_x(10336, get_base());
	return get_unit_size_x(unit);
}

int32_t diablo2::d2_common::get_unit_size_y(structures::unit* unit) {
	static wrap_func_std_import<int32_t(structures::unit*)> get_unit_size_y(10337, get_base());
	return get_unit_size_y(unit);
}

int32_t diablo2::d2_common::get_distance_between_units(structures::unit* unit1, structures::unit* unit2) {
	static wrap_func_std_import<int32_t(structures::unit*, structures::unit*)> get_distance_between_units(10397, get_base());
	return get_distance_between_units(unit1, unit2);
}

int32_t diablo2::d2_common::get_unit_x(structures::path* path) {
	static wrap_func_std_import<int32_t(structures::path*)> get_unit_x(10162, get_base());
	return get_unit_x(path);
}

int32_t diablo2::d2_common::get_unit_y(structures::path* path) {
	static wrap_func_std_import<int32_t(structures::path*)> get_unit_y(10163, get_base());
	return get_unit_y(path);
}

int32_t diablo2::d2_common::get_unit_precise_x(structures::unit* unit) {
	static wrap_func_std_import<int32_t(structures::unit*)> get_unit_precise_x(10327, get_base());
	return get_unit_precise_x(unit);
}

int32_t diablo2::d2_common::get_unit_precise_y(structures::unit* unit) {
	static wrap_func_std_import<int32_t(structures::unit*)> get_unit_precise_y(10330, get_base());
	return get_unit_precise_y(unit);
}

diablo2::structures::unit* diablo2::d2_common::get_target_from_path(structures::path* path) {
	static wrap_func_std_import<structures::unit * (structures::path*)> get_target_from_path(10180, get_base());
	return get_target_from_path(path);
}

void diablo2::d2_common::assign_skill(structures::unit* unit, uint32_t skill_id, uint32_t level, BOOL remove,
									  const char* file, int32_t line) {
	static wrap_func_std_import<void(structures::unit*, uint32_t, uint32_t, BOOL, const char*, int32_t)> assign_skill_fn(
		10953, get_base());
	assign_skill_fn(unit, skill_id, level, remove, file, line);
}

int32_t diablo2::d2_common::get_skill_id(structures::skill* skill) {
	static wrap_func_std_import<int32_t(structures::skill*, const char*, int32_t)> get_skill_id_fn(10963, get_base());
	return get_skill_id_fn(skill, nullptr, 0);
}

int32_t diablo2::d2_common::get_skill_level(structures::unit* unit, structures::skill* skill, BOOL with_bonus) {
	static wrap_func_std_import<int32_t(structures::unit*, structures::skill*, BOOL)> get_skill_level_fn(10968, get_base());
	return get_skill_level_fn(unit, skill, with_bonus);
}

void diablo2::d2_common::refresh_passive_skills(structures::unit* unit) {
	static wrap_func_std_import<void(structures::unit*)> refresh_passive_skills_fn(10941, get_base());
	refresh_passive_skills_fn(unit);
}
