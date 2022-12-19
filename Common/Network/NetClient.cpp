#include "NetClient.h"
#include "NetCommon.h"

static void* __stdcall JoinDeserializer(char* packet, int size)
{
	static base::ServerJoin join;
	if (join.ParseFromArray(packet, size)) return &join;
	return nullptr;
}
static void* __stdcall PluginDeserializer(char* packet, int size)
{
	static base::ServerPlugin plugin;
	if (plugin.ParseFromArray(packet, size)) return &plugin;
	return nullptr;
}
static void* __stdcall SetStateDeserializer(char* packet, int size)
{
	static base::ServerSetState state;
	if (state.ParseFromArray(packet, size)) return &state;
	return nullptr;
}

void NetClient::SteamNetClientConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetClient* client = (NetClient*)pInfo->m_info.m_nUserData;

	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		client->socket.networking->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		client->socket.socket = 0;
		if (client->disconnectCB) client->disconnectCB(client, client->local);
		break;
	}


	case k_ESteamNetworkingConnectionState_None:
	case k_ESteamNetworkingConnectionState_Connected:
	case k_ESteamNetworkingConnectionState_Connecting:
	default:
		break;
	}


}
JoinResult NetClient::Create(const char* ip, uint32_t port, const std::string& name, NetClient** out)
{
	JoinResult joinRes;
	joinRes.success = false;
	if (name.size() > MAX_NAME_LENGTH)
	{
		joinRes.reason = "Name to long";
		return joinRes;
	}
	if (name.size() < MIN_NAME_LENGTH)
	{
		joinRes.reason = "Name to short";
		return joinRes;
	}
	uint32_t ipAddr = ParseIP(ip);
	if (!ipAddr) {
		joinRes.reason = "Failed to Parse IP";
		return joinRes;
	}
	*out = nullptr;
	NetClient* temp = new NetClient();
	temp->socket.networking = SteamNetworkingSockets();
	
	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.SetIPv4(ipAddr, port);
	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetClientConnectionStatusChangedCallback);
	HSteamListenSocket sock = temp->socket.networking->ConnectByIPAddress(addr, 1, &opt);
	if (sock == k_HSteamListenSocket_Invalid)
	{
		delete temp;
		joinRes.reason = "Failed to Create Socket";
		return joinRes;
	}
	temp->socket.socket = sock;
	temp->socket.networking->SetConnectionUserData(sock, (int64)temp);
	



	base::ClientJoin join;
	join.set_name(name);
	std::string str = join.SerializeAsString();
	
	temp->SendData(Client_Join, str.data(), str.length(), SendFlags::Send_Reliable);

	*out = temp;
	return joinRes;
}

NetClient::~NetClient()
{
}

bool NetClient::IsConnected() const
{
	SteamNetConnectionRealTimeStatus_t stat;
	EResult res = socket.networking->GetConnectionRealTimeStatus(socket.socket, &stat, 0, NULL);
	if (res == EResult::k_EResultOK)
	{
		return stat.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected;
	}
	return false;
}

ClientConnection* NetClient::GetSelf()
{
	return local;
}

ClientConnection* NetClient::GetConnection(uint16_t id)
{
	if (id < MAX_PLAYERS)
	{
		if (socket.connected[id].isConnected != ConnectionState::Disconnected)
		{
			return &socket.connected[id];
		}
	}
	return nullptr;
}

void NetClient::SetCallback(ClientPacketFunction fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).receiver = fn;
}
void NetClient::SetDeserializer(DeserializationFunc fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).deserializer = fn;
}

void NetClient::SetJoinCallback(ClientJoinCallbackFunction fn)
{
	joinCB = fn;
}

void NetClient::SetDisconnectCallback(ClientDisconnectCallbackFunction fn)
{
	disconnectCB = fn;
}

bool NetClient::SendData(uint16_t packetID, const void* data, uint32_t size, uint32_t flags)
{
	if (tempStorage.size() < size + 10)
	{
		tempStorage.resize(size + 10);
	}
	*(uint16_t*)&tempStorage.at(0) = packetID;
	memcpy(&tempStorage.at(sizeof(packetID)), data, size);
	int64 outMsgNum = 0;
	EResult res = socket.networking->SendMessageToConnection(socket.socket, tempStorage.data(), size + sizeof(packetID), flags, &outMsgNum);
	return EResult::k_EResultOK == res;
}

bool NetClient::IsP2P() const
{
	return false;
}

void NetClient::Poll()
{
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = socket.networking->ReceiveMessagesOnConnection(socket.socket, &pIncomingMsg, 1);
		if (numMsgs == 0)
			break;
		if (numMsgs < 0) return;
		const uint32 size = pIncomingMsg->GetSize();
		if (size < sizeof(uint16_t)) {
			pIncomingMsg->Release();
			continue;
		}

		const uint16_t* data = (const uint16_t*)pIncomingMsg->GetData();
		const uint16_t packetID = *data;
		if (packetID < callbacks.size())
		{
			NetClient::CallbackInfo& info = callbacks.at(packetID);
			if (info.receiver)
			{
				const void* packet = data + 1;
				if (info.deserializer) {
					packet = info.deserializer((char*)packet, size - 2);
				}
				if (packet)
				{
					info.receiver(this, (void*)packet, size - 2);
				}
			}
		}

		pIncomingMsg->Release();
	}
}
bool NetClient::ServerJoinCallback(NetClient* c, base::ServerJoin* join, int size)
{
	
	const int32 idVal = join->data().id();
	if (idVal < MAX_PLAYERS && idVal >= 0)
	{
		ClientConnection* cl = &c->socket.connected[idVal];
		cl->isAdmin = join->data().is_admin();
		cl->id = idVal;
		cl->isConnected = ConnectionState::Connected;
		cl->name = join->data().name();
		if (join->is_local())
		{
			c->local = cl;
		}
		if (c->joinCB)
		{
			c->joinCB(c, cl);
		}
	}
	else
	{
		c->socket.networking->CloseConnection(c->socket.socket, 1, "received invalid player id", false);
	}
	return true;
}