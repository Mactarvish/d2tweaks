#include <common/config.h>

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

namespace {
	bool get_bool(const YAML::Node& node, const char* key, bool fallback) {
		if (!node || !node[key])
			return fallback;
		return node[key].as<bool>(fallback);
	}

	int32_t get_int(const YAML::Node& node, const char* key, int32_t fallback) {
		if (!node || !node[key])
			return fallback;
		return node[key].as<int32_t>(fallback);
	}
}

void config::load_defaults() {
	m_auto_gold_enabled = true;
	m_auto_gold_distance = 4;

	m_auto_potion_enabled = true;
	m_auto_potion_distance = 4;
	m_pickup_hp = true;
	m_pickup_mp = true;
	m_pickup_rejuv = true;
	m_pickup_hpo = true;
	m_pickup_runes = true;
	m_rune_min = 1;
	m_rune_max = 33;
	m_pickup_tsc = true;
	m_pickup_perfect_gems = true;
	m_pickup_charms = true;
	m_refill_belt = false;

	m_auto_enter_portal_enabled = true;
	m_auto_enter_portal_key = 0x08;

	m_attr_boost_enabled = false;
	m_attr_boost_multiplier = 100;
	m_attr_boost_resist_value = 0;
	m_attr_boost_armor_value = 0;
	m_attr_boost_skill_level = 0;
	m_attr_boost_hp_enabled = false;
	m_attr_boost_hp_value = 200000;
	std::strncpy(m_attr_boost_name, "DDDDDDDDD", sizeof m_attr_boost_name);
	m_attr_boost_name[sizeof m_attr_boost_name - 1] = '\0';
}

void config::load_from_yaml(const char* path) {
	try {
		const auto root = YAML::LoadFile(path);

		if (const auto gold = root["auto_gold_pickup"]) {
			m_auto_gold_enabled = get_bool(gold, "enabled", m_auto_gold_enabled);
			m_auto_gold_distance = get_int(gold, "distance", m_auto_gold_distance);
		}

		if (const auto potion = root["auto_potion_pickup"]) {
			m_auto_potion_enabled = get_bool(potion, "enabled", m_auto_potion_enabled);
			m_auto_potion_distance = get_int(potion, "distance", m_auto_potion_distance);
			m_pickup_hp = get_bool(potion, "pickup_hp", m_pickup_hp);
			m_pickup_mp = get_bool(potion, "pickup_mp", m_pickup_mp);
			m_pickup_rejuv = get_bool(potion, "pickup_rejuv", m_pickup_rejuv);
			m_pickup_hpo = get_bool(potion, "pickup_hpo", m_pickup_hpo);
			m_pickup_runes = get_bool(potion, "pickup_runes", m_pickup_runes);
			m_rune_min = get_int(potion, "rune_min", m_rune_min);
			m_rune_max = get_int(potion, "rune_max", m_rune_max);
			m_pickup_tsc = get_bool(potion, "pickup_tsc", m_pickup_tsc);
			m_pickup_perfect_gems = get_bool(potion, "pickup_perfect_gems", m_pickup_perfect_gems);
			m_pickup_charms = get_bool(potion, "pickup_charms", m_pickup_charms);
			m_refill_belt = get_bool(potion, "refill_belt", m_refill_belt);
		}

		if (const auto portal = root["auto_enter_portal"]) {
			m_auto_enter_portal_enabled = get_bool(portal, "enabled", m_auto_enter_portal_enabled);
			m_auto_enter_portal_key = get_int(portal, "key", m_auto_enter_portal_key);
		}

		if (const auto boost = root["attr_boost"]) {
			m_attr_boost_enabled = get_bool(boost, "enabled", m_attr_boost_enabled);
			m_attr_boost_multiplier = get_int(boost, "multiplier", m_attr_boost_multiplier);
			m_attr_boost_resist_value = get_int(boost, "resist_value", m_attr_boost_resist_value);
			m_attr_boost_armor_value = get_int(boost, "armor_value", m_attr_boost_armor_value);
			m_attr_boost_skill_level = get_int(boost, "skill_level", m_attr_boost_skill_level);
			m_attr_boost_hp_enabled = get_bool(boost, "hp_enabled", m_attr_boost_hp_enabled);
			m_attr_boost_hp_value = get_int(boost, "hp_value", m_attr_boost_hp_value);
			if (boost["name"]) {
				const auto name = boost["name"].as<std::string>(std::string(m_attr_boost_name));
				std::strncpy(m_attr_boost_name, name.c_str(), sizeof m_attr_boost_name);
				m_attr_boost_name[sizeof m_attr_boost_name - 1] = '\0';
			}
		}

		m_auto_gold_distance = std::max(1, m_auto_gold_distance);
		m_auto_potion_distance = std::max(1, m_auto_potion_distance);
		m_rune_min = std::clamp(m_rune_min, 1, 33);
		m_rune_max = std::clamp(m_rune_max, 1, 33);
		if (m_rune_min > m_rune_max)
			std::swap(m_rune_min, m_rune_max);
		m_attr_boost_multiplier = std::max(1, m_attr_boost_multiplier);
		m_attr_boost_resist_value = std::max(0, m_attr_boost_resist_value);
		m_attr_boost_armor_value = std::max(0, m_attr_boost_armor_value);
		m_attr_boost_skill_level = std::max(0, m_attr_boost_skill_level);
		m_attr_boost_hp_value = std::max(0, m_attr_boost_hp_value);

		spdlog::info("Loaded config from {}", path);
	} catch (const std::exception& e) {
		spdlog::error("Failed to load {}: {}", path, e.what());
	}
}

config::config(token) {
	load_defaults();

	static const char* k_paths[] = {
		"d2tweaks.yaml",
		"d2tweaks/d2tweaks.yaml",
	};

	for (const auto* path : k_paths) {
		if (std::filesystem::exists(path)) {
			load_from_yaml(path);
			return;
		}
	}

	spdlog::warn("d2tweaks.yaml not found, using defaults");
}
