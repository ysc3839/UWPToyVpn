#include "pch.h"
#include "VpnPlugin.h"
#include "../strutil.h"

using namespace winrt;
using namespace Windows::Networking;
using namespace Windows::Networking::Vpn;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

using namespace std::chrono_literals;

namespace winrt::ToyVpnBG::implementation
{
	constexpr int MAX_HANDSHAKE_ATTEMPTS = 50;
	constexpr auto IDLE_INTERVAL = 100ms;

	constexpr int MAX_PACKET_SIZE = UINT16_MAX;

	void VpnPlugin::Connect(VpnChannel const& channel)
	{
		try
		{
			auto context = channel.PlugInContext().as<VpnPluginContext>();
			if (!context)
			{
				context = make<VpnPluginContext>().as<VpnPluginContext>();
				channel.PlugInContext(context.as<IInspectable>());
			}

			DatagramSocket tunnel;
			channel.AssociateTransport(tunnel, nullptr);

			hstring parametershs;
			winrt::event_token token = tunnel.MessageReceived([&](DatagramSocket const& /*sender*/, DatagramSocketMessageReceivedEventArgs const& args)
			{
				DataReader reader{ args.GetDataReader() };
				if (reader.UnconsumedBufferLength() > 0 && reader.ReadByte() == 0)
				{
					parametershs = reader.ReadString(reader.UnconsumedBufferLength());

					tunnel.MessageReceived(token);
					context->handshakeState.store(HandshakeState::Received);
				}
			});

			HostName serverHostname{ nullptr };
			try
			{
				serverHostname = channel.Configuration().ServerHostNameList().GetAt(0);
			}
			catch (hresult_error const&)
			{
				throw hresult_invalid_argument{ L"Invalid server host name" };
			}

			std::wstring port, secret;

			Windows::Data::Xml::Dom::XmlDocument doc;
			doc.LoadXml(channel.Configuration().CustomField());
			auto root = doc.FirstChild();
			if (root.NodeName() == L"toyvpn-config")
			{
				for (auto node : root.ChildNodes())
				{
					if (node.NodeName() == L"port")
						port = node.InnerText();
					else if (node.NodeName() == L"secret")
						secret = node.InnerText();
				}
			}

			tunnel.ConnectAsync(serverHostname, port).get();

			Handshake(tunnel, context->handshakeState, secret);

			Configure(channel, parametershs);
		}
		catch (hresult_error const& ex)
		{
			channel.TerminateConnection(ex.message());
		}
	}

	void VpnPlugin::Disconnect(VpnChannel const& channel)
	{
		try
		{
			auto context = channel.PlugInContext().as<VpnPluginContext>();
			if (!context)
			{
			}
			else
			{
				context->handshakeState.store(HandshakeState::Canceled);
				channel.Stop();
			}
		}
		catch (winrt::hresult_error const&)
		{
		}
		channel.PlugInContext(nullptr);
	}

	void VpnPlugin::GetKeepAlivePayload(VpnChannel const& /*channel*/, VpnPacketBuffer& /*keepAlivePacket*/)
	{
	}

	void VpnPlugin::Encapsulate(VpnChannel const& /*channel*/, VpnPacketBufferList const& packets, VpnPacketBufferList const& encapulatedPackets)
	{
		while (packets.Size() > 0)
		{
			encapulatedPackets.Append(packets.RemoveAtBegin());
		}
	}

	void VpnPlugin::Decapsulate(VpnChannel const& channel, VpnPacketBuffer const& encapBuffer, VpnPacketBufferList const& decapsulatedPackets, VpnPacketBufferList const& /*controlPacketsToSend*/)
	{
		auto buf = channel.GetVpnReceivePacketBuffer();
		auto len = encapBuffer.Buffer().Length();
		if (len > buf.Buffer().Capacity())
		{
			return;
		}

		memcpy(buf.Buffer().data(), encapBuffer.Buffer().data(), len);
		buf.Buffer().Length(len);
		decapsulatedPackets.Append(buf);
	}

	void VpnPlugin::Handshake(DatagramSocket const& tunnel, std::atomic<HandshakeState> const& handshakeState, std::wstring const& secret)
	{
		for (int i = 0; i < 3; ++i)
		{
			DataWriter writer{ tunnel.OutputStream() };
			writer.UnicodeEncoding(UnicodeEncoding::Utf8);
			writer.WriteByte(0);
			writer.WriteString(secret);
			writer.StoreAsync().get();
			writer.DetachStream();
		}

		for (int i = 0; i < MAX_HANDSHAKE_ATTEMPTS; ++i)
		{
			std::this_thread::sleep_for(IDLE_INTERVAL);

			switch (handshakeState.load())
			{
			case HandshakeState::Received:
				return;
			case HandshakeState::Canceled:
				throw hresult_canceled{};
			}
		}

		throw hresult_error{ E_FAIL, L"Operation timed out" };
	}

	void VpnPlugin::Configure(Windows::Networking::Vpn::VpnChannel const & channel, hstring const& parametershs)
	{
		std::wstring_view parameters{ parametershs };
		rtrimwsv(parameters);

		uint32_t mtuSize{};
		std::vector<HostName> IPv4AddrList;
		VpnRouteAssignment route;
		auto IPv4Routes = route.Ipv4InclusionRoutes();
		std::vector<HostName> dnsServerList;

		for (auto const& parameter : splitwsv(parameters))
		{
			auto fields = splitws(parameter, L',');
			try
			{
				switch (fields[0].at(0))
				{
				case 'm':
					mtuSize = stoul(fields.at(1));
					break;
				case 'a':
					IPv4AddrList.emplace_back(fields.at(1));
					break;
				case 'r':
					IPv4Routes.Append({ {fields.at(1)}, static_cast<uint8_t>(stoul(fields.at(2))) });
					break;
				case 'd':
					dnsServerList.emplace_back(fields.at(1));
					break;
				}
			}
			catch (std::out_of_range const&)
			{
				throw hresult_invalid_argument{ L"Bad parameter: " + parameter };
			}
		}

		VpnDomainNameAssignment assignment;
		assignment.DomainNameList().Append({ L".", VpnDomainNameType::Suffix, dnsServerList, nullptr });

		channel.StartExistingTransports(
			IPv4AddrList,
			nullptr,
			nullptr,
			route,
			assignment,
			mtuSize,
			MAX_PACKET_SIZE,
			false
		);
	}
}
