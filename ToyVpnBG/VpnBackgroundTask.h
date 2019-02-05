#pragma once

#include "VpnBackgroundTask.g.h"

namespace winrt::ToyVpnBG::implementation
{
	struct VpnBackgroundTask : VpnBackgroundTaskT<VpnBackgroundTask>
	{
		VpnBackgroundTask() = default;

		void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance const& taskInstance);
	};
}

namespace winrt::ToyVpnBG::factory_implementation
{
	struct VpnBackgroundTask : VpnBackgroundTaskT<VpnBackgroundTask, implementation::VpnBackgroundTask>
	{
	};
}
