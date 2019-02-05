#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::ToyVpn::implementation
{
	MainPage::MainPage()
	{
		InitializeComponent();
	}

	void MainPage::ClickHandler(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&)
	{
		Windows::System::Launcher::LaunchUriAsync({ L"ms-settings:network-vpn" });
	}
}
