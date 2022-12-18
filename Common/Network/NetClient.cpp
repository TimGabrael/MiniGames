#include "NetClient.h"
#include "NetCommon.h"


static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetClient* client = (NetClient*)pInfo->m_info.m_nUserData;

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
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
	HSteamListenSocket sock = temp->socket.networking->ConnectByIPAddress(addr, 1, &opt);
	if (sock == k_HSteamListenSocket_Invalid)
	{
		delete temp;
		joinRes.reason = "Failed to Create Socket";
		return joinRes;
	}
	temp->socket.socket = sock;
	temp->socket.networking->SetConnectionUserData(sock, (int64)temp);

	SteamNetConnectionRealTimeStatus_t stat;
	EResult res = temp->socket.networking->GetConnectionRealTimeStatus(temp->socket.socket, &stat, 0, NULL);

	while (res && stat.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connecting)
	{
		temp->Poll();
		res = temp->socket.networking->GetConnectionRealTimeStatus(temp->socket.socket, &stat, 0, NULL);
	}

	
	
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
	return nullptr;
}

ClientConnection* NetClient::GetConnection(uint16_t id)
{
	return nullptr;
}

void NetClient::SetCallback(ClientPacketFunction fn, uint16_t packetID)
{
}

void NetClient::SetJoinCallback(ClientJoinCallbackFunction fn)
{
}

void NetClient::SetDisconnectcallback(ClientDisconnectCallbackFunction fn)
{
}

bool NetClient::SendData(const void* data, uint32_t size, uint32_t flags)
{
	int64 outMsgNum = 0;
	EResult res = socket.networking->SendMessageToConnection(socket.socket, data, size, flags, &outMsgNum);
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


		pIncomingMsg->Release();
	}
}

void NetClient::RunCallbacks()
{
	SteamNetworkingSockets()->RunCallbacks();
}