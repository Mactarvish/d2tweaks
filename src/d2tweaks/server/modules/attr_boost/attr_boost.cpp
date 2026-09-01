#include <d2tweaks/server/modules/attr_boost/attr_boost.h>
#include <d2tweaks/server/server.h>

#include <common/config.h>
#include <d2tweaks/common/attr_boost_common.h>

#include <diablo2/d2common.h>
#include <diablo2/structures/unit.h>

#include <spdlog/spdlog.h>

MODULE_INIT(attr_boost)

namespace {
	constexpr diablo2::unit_stats_t k_attrs[] = {
		diablo2::UNIT_STAT_STRENGTH,
		diablo2::UNIT_STAT_ENERGY,
		diablo2::UNIT_STAT_DEXTERITY,
		diablo2::UNIT_STAT_VITALITY,
	};

	uint32_t g_attrs_boosted_guid = 0;
	uint32_t g_boost_log_guid = 0;
}

void d2_tweaks::server::modules::attr_boost::init() {
	singleton<server>::instance().register_tick_handler(this);
}

void d2_tweaks::server::modules::attr_boost::tick(diablo2::structures::game* game,
												  diablo2::structures::unit* unit) {
	if (!game || !unit)
		return;
	if (unit->type != diablo2::structures::unit_type_t::UNIT_TYPE_PLAYER)
		return;

	const auto& cfg = singleton<config>::instance();
	if (!d2_tweaks::common::attr_boost_common::name_matches(unit, cfg.attr_boost_name()))
		return;

	const auto mult = cfg.attr_boost_multiplier();
	const auto resist = cfg.attr_boost_resist_value();
	const auto armor = cfg.attr_boost_armor_value();
	const auto skill_level = cfg.attr_boost_skill_level();
	const auto hp_value = cfg.attr_boost_hp_enabled() ? cfg.attr_boost_hp_value() : 0;
	const bool do_attrs = cfg.attr_boost_enabled() && mult > 1;
	const bool do_resists = resist > 0;
	const bool do_armor = armor > 0;
	const bool do_skills = skill_level > 0;
	const bool do_hp = hp_value > 0;

	if (do_attrs && unit->guid != g_attrs_boosted_guid) {
		const auto str = diablo2::d2_common::get_base_stat(unit, diablo2::UNIT_STAT_STRENGTH, 0);
		if (str > 0 && str < 5000) {
			for (const auto stat : k_attrs) {
				const auto cur = diablo2::d2_common::get_base_stat(unit, stat, 0);
				if (cur <= 0)
					continue;
				diablo2::d2_common::set_stat(unit, stat, static_cast<uint32_t>(cur) * static_cast<uint32_t>(mult), 0);
			}
			spdlog::info("attr_boost_server {} attrs x{}", unit->player_data->name, mult);
			g_attrs_boosted_guid = unit->guid;
		}
	}

	// 服务端：实际承伤计算用
	if (do_resists)
		d2_tweaks::common::attr_boost_common::apply_elemental_resists(
			unit, static_cast<uint32_t>(resist));
	if (do_armor)
		d2_tweaks::common::attr_boost_common::apply_armor_class(
			unit, static_cast<uint32_t>(armor));
	if (do_skills)
		d2_tweaks::common::attr_boost_common::apply_skill_levels(
			unit, static_cast<uint32_t>(skill_level));
	if (do_hp)
		d2_tweaks::common::attr_boost_common::apply_hit_points(
			unit, static_cast<uint32_t>(hp_value));

	if ((do_resists || do_armor || do_skills || do_hp) && unit->guid != g_boost_log_guid) {
		spdlog::info("attr_boost_server {} resist={} armor={} skill={} hp={} ac={}",
					 unit->player_data->name,
					 resist,
					 armor,
					 skill_level,
					 hp_value,
					 diablo2::d2_common::get_stat(unit, diablo2::UNIT_STAT_ARMORCLASS, 0));
		g_boost_log_guid = unit->guid;
	}
}
