#include <d2tweaks/client/modules/auto_enter_portal/auto_enter_portal.h>

#include <d2tweaks/client/client.h>
#include <common/config.h>

#include <spdlog/spdlog.h>

MODULE_INIT(auto_enter_portal)

void d2_tweaks::client::modules::auto_enter_portal::init() {
	// Disabled: previous implementation targeted wrong objects (e.g. txt=78)
	// and spam-sent run/interact packets, causing assertion failures in town.
	if (singleton<config>::instance().auto_enter_portal_enabled()) {
		spdlog::warn("auto_enter_portal: config enabled but feature is retired (unsafe)");
	}
}

void d2_tweaks::client::modules::auto_enter_portal::tick() {
}
