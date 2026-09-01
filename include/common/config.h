#pragma once

#include <fw/singleton.h>
#include <cstdint>

class config : public singleton<config> {
	bool m_auto_gold_enabled = true;
	int32_t m_auto_gold_distance = 4;

	bool m_auto_potion_enabled = true;
	int32_t m_auto_potion_distance = 4;
	bool m_pickup_hp = true;
	bool m_pickup_mp = true;
	bool m_pickup_rejuv = true;
	bool m_pickup_hpo = true;
	bool m_pickup_runes = true;
	int32_t m_rune_min = 1;
	int32_t m_rune_max = 33;
	bool m_pickup_tsc = true;
	bool m_pickup_perfect_gems = true;
	bool m_pickup_charms = true;
	bool m_refill_belt = false;

	bool m_auto_enter_portal_enabled = true;
	int32_t m_auto_enter_portal_key = 0x08; // VK_BACK

	bool m_attr_boost_enabled = false;
	int32_t m_attr_boost_multiplier = 100;
	int32_t m_attr_boost_resist_value = 0; // 0 = off; else set 四防+上限
	int32_t m_attr_boost_armor_value = 0; // 0 = off; else set 防御(护甲)
	int32_t m_attr_boost_skill_level = 0; // 0 = off; else set 已学技能等级
	bool m_attr_boost_hp_enabled = false;
	int32_t m_attr_boost_hp_value = 200000; // 面板生命；hp_enabled 为 true 时生效
	char m_attr_boost_name[16] = "DDDDDDDDD";

	void load_defaults();
	void load_from_yaml(const char* path);

public:
	explicit config(token);

	bool auto_gold_enabled() const { return m_auto_gold_enabled; }
	int32_t auto_gold_distance() const { return m_auto_gold_distance; }

	bool auto_potion_enabled() const { return m_auto_potion_enabled; }
	int32_t auto_potion_distance() const { return m_auto_potion_distance; }
	bool pickup_hp() const { return m_pickup_hp; }
	bool pickup_mp() const { return m_pickup_mp; }
	bool pickup_rejuv() const { return m_pickup_rejuv; }
	bool pickup_hpo() const { return m_pickup_hpo; }
	bool pickup_runes() const { return m_pickup_runes; }
	int32_t rune_min() const { return m_rune_min; }
	int32_t rune_max() const { return m_rune_max; }
	bool pickup_tsc() const { return m_pickup_tsc; }
	bool pickup_perfect_gems() const { return m_pickup_perfect_gems; }
	bool pickup_charms() const { return m_pickup_charms; }
	bool refill_belt() const { return m_refill_belt; }

	bool auto_enter_portal_enabled() const { return m_auto_enter_portal_enabled; }
	int32_t auto_enter_portal_key() const { return m_auto_enter_portal_key; }

	bool attr_boost_enabled() const { return m_attr_boost_enabled; }
	int32_t attr_boost_multiplier() const { return m_attr_boost_multiplier; }
	int32_t attr_boost_resist_value() const { return m_attr_boost_resist_value; }
	int32_t attr_boost_armor_value() const { return m_attr_boost_armor_value; }
	int32_t attr_boost_skill_level() const { return m_attr_boost_skill_level; }
	bool attr_boost_hp_enabled() const { return m_attr_boost_hp_enabled; }
	int32_t attr_boost_hp_value() const { return m_attr_boost_hp_value; }
	const char* attr_boost_name() const { return m_attr_boost_name; }
};
