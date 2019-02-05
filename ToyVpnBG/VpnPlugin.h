#pragma once

namespace winrt::ToyVpnBG::implementation
{
	enum HandshakeState
	{
		Waiting,
		Received,
		Canceled
	};

	struct VpnPlugin : implements<VpnPlugin, winrt::Windows::Networking::Vpn::IVpnPlugIn>
	{
		VpnPlugin() = default;

		void Connect(Windows::Networking::Vpn::VpnChannel const& channel);
		void Disconnect(Windows::Networking::Vpn::VpnChannel const& channel);
		void GetKeepAlivePayload(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBuffer& keepAlivePacket);
		void Encapsulate(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBufferList const& packets, Windows::Networking::Vpn::VpnPacketBufferList const& encapulatedPackets);
		void Decapsulate(Windows::Networking::Vpn::VpnChannel const& channel, Windows::Networking::Vpn::VpnPacketBuffer const& encapBuffer, Windows::Networking::Vpn::VpnPacketBufferList const& decapsulatedPackets, Windows::Networking::Vpn::VpnPacketBufferList const& controlPacketsToSend);

	private:
		void Handshake(Windows::Networking::Sockets::DatagramSocket const& tunnel, std::atomic<HandshakeState> const& handshakeState);
		void Configure(Windows::Networking::Vpn::VpnChannel const& channel, hstring const& parametershs);
	};

	struct VpnPluginContext : implements<VpnPluginContext, winrt::Windows::Foundation::IInspectable>
	{
		friend struct VpnPlugin;

		VpnPluginContext() = default;

	private:
		std::atomic<HandshakeState> handshakeState{ Waiting };
	};
}
