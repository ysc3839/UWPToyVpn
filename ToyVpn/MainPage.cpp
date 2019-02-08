#include "pch.h"
#include "MainPage.h"
#include "../strutil.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Networking::Vpn;

namespace winrt::ToyVpn::implementation
{
	constexpr auto vpnProfileName = L"ToyVpnProfile";

	struct VpnPlugInProfileWithName : VpnPlugInProfile
	{
		VpnPlugInProfileWithName(param::hstring const& value)
		{
			ProfileName(value);
		}
	};

	MainPage::MainPage()
	{
		InitializeComponent();
		LoadSettings();
	}

	void MainPage::LaunchVPNSettings(IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&)
	{
		Windows::System::Launcher::LaunchUriAsync({ L"ms-settings:network-vpn" });
	}

	IAsyncAction MainPage::SaveVPN(IInspectable const&, Windows::UI::Xaml::RoutedEventArgs const&)
	{
		SaveSettings();

		Windows::Data::Xml::Dom::XmlDocument doc;
		auto elemConfig = doc.CreateElement(L"toyvpn-config");
		doc.AppendChild(elemConfig);

		auto elemPort = doc.CreateElement(L"port");
		elemPort.InnerText(inputPort().Text());
		elemConfig.AppendChild(elemPort);

		auto elemSecret = doc.CreateElement(L"secret");
		elemSecret.InnerText(inputSecret().Text());
		elemConfig.AppendChild(elemSecret);

		VpnPlugInProfile profile;
		profile.AlwaysOn(chkAlwaysOn().IsChecked().Value());
		profile.CustomConfiguration(doc.GetXml());
		profile.ProfileName(vpnProfileName);
		profile.RememberCredentials(false);
		profile.RequireVpnClientAppUI(chkShowInSettings().IsChecked().Value());
		profile.VpnPluginPackageFamilyName(Windows::ApplicationModel::Package::Current().Id().FamilyName());

		std::wstring_view server{ inputServer().Text() };
		profile.ServerUris().Append({ server._Starts_with(L"http://") ? server : L"http://" + server });

		co_await VpnManagementAgent().UpdateProfileFromObjectAsync(profile);
	}

	IAsyncAction MainPage::ConnectVPN(IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args)
	{
		co_await SaveVPN(sender, args);
		co_await VpnManagementAgent().ConnectProfileAsync(VpnPlugInProfileWithName(vpnProfileName));
	}

	IAsyncAction MainPage::DisconnectVPN(IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args)
	{
		co_await VpnManagementAgent().DisconnectProfileAsync(VpnPlugInProfileWithName(vpnProfileName));
	}

	void MainPage::LoadSettings()
	{
		auto roamingSettingsValues = Windows::Storage::ApplicationData::Current().RoamingSettings().Values();
		inputServer().Text(unbox_value_or<hstring>(roamingSettingsValues.Lookup(L"server"), {}));
		inputPort().Text(unbox_value_or<hstring>(roamingSettingsValues.Lookup(L"port"), {}));
		inputSecret().Text(unbox_value_or<hstring>(roamingSettingsValues.Lookup(L"secret"), {}));
		chkAlwaysOn().IsChecked(unbox_value_or<bool>(roamingSettingsValues.Lookup(L"alwaysOn"), false));
		chkShowInSettings().IsChecked(unbox_value_or<bool>(roamingSettingsValues.Lookup(L"showInSettings"), true));
	}

	void MainPage::SaveSettings()
	{
		auto roamingSettingsValues = Windows::Storage::ApplicationData::Current().RoamingSettings().Values();
		roamingSettingsValues.Insert(L"server", box_value(inputServer().Text()));
		roamingSettingsValues.Insert(L"port", box_value(inputPort().Text()));
		roamingSettingsValues.Insert(L"secret", box_value(inputSecret().Text()));
		roamingSettingsValues.Insert(L"alwaysOn", box_value(chkAlwaysOn().IsChecked()));
		roamingSettingsValues.Insert(L"showInSettings", box_value(chkShowInSettings().IsChecked()));
	}
}
