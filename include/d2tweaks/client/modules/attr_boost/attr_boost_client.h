#pragma once

#include <d2tweaks/client/modules/client_module.h>

namespace d2_tweaks {
	namespace client {
		namespace modules {
			class attr_boost_client final : public client_module {
			public:
				void init() override;
				void tick() override;
			};
		}
	}
}
