#include <d2tweaks/client/modules/attr_boost/attr_boost_client.h>
#include <d2tweaks/client/client.h>

#include <common/config.h>
#include <d2tweaks/common/attr_boost_common.h>

#include <diablo2/d2client.h>
#include <diablo2/structures/unit.h>

#include <spdlog/spdlog.h>

MODULE_INIT(attr_boost_client)

namespace {
	uint32_t g_logged_guid = 0;
}

void d2_tweaks::client::modules::attr_boost_client::init() {
	singleton<client>::instance().register_tick_handler(this);
}

void d2_tweaks::client::modules::attr_boost_client::tick() {
	const auto& cfg = singleton<config>::instance();
	const auto resist = cfg.attr_boost_resist_value();
	const auto armor = cfg.attr_boost_armor_value();
	const auto skill_level = cfg.attr_boost_skill_level();
	const auto hp_value = cfg.attr_boost_hp_enabled() ? cfg.attr_boost_hp_value() : 0;
	if (resist <= 0 && armor <= 0 && skill_level <= 0 && hp_value <= 0)
		return;

	const auto player = diablo2::d2_client::get_local_player();
	if (!player)
		return;
	if (!d2_tweaks::common::attr_boost_common::name_matches(player, cfg.attr_boost_name()))
		return;

	if (resist > 0)
		d2_tweaks::common::attr_boost_common::apply_elemental_resists(
			player, static_cast<uint32_t>(resist));
	if (armor > 0)
		d2_tweaks::common::attr_boost_common::apply_armor_class(
			player, static_cast<uint32_t>(armor));
	if (skill_level > 0)
		d2_tweaks::common::attr_boost_common::apply_skill_levels(
			player, static_cast<uint32_t>(skill_level));
	if (hp_value > 0)
		d2_tweaks::common::attr_boost_common::apply_hit_points(
			player, static_cast<uint32_t>(hp_value));

	if (player->guid != g_logged_guid) {
		spdlog::info("attr_boost_client {} resist={} armor={} skill={} hp={} ac={}",
					  player->player_data->name,
					  resist,
					  armor,
					  skill_level,
					  hp_value,
					  diablo2::d2_common::get_stat(player, diablo2::UNIT_STAT_ARMORCLASS, 0));
		g_logged_guid = player->guid;
	}
}
