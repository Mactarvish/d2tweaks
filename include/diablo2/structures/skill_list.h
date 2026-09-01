#pragma once

#include <cstdint>

#pragma pack(push, 1)

namespace diablo2 {
	namespace structures {
		struct skills_txt;

		struct skill {
			skills_txt* skills_txt;
			skill* next_skill;
			uint32_t skill_mode;
			uint32_t flags;
			uint32_t unk10[2];
			uint32_t target_info;
			uint32_t target_type;
			uint32_t target_guid;
			uint32_t par4;
			int32_t skill_level;
			uint32_t level_bonus;
			int32_t quantity;
			uint32_t owner_guid;
			int32_t charges;
		};

		struct skill_list {
			void* memory_pool;
			skill* first_skill;
			skill* left_skill;
			skill* right_skill;
			skill* used_skill;
			uint32_t unk14;
		};
	}
}

#pragma pack(pop)
