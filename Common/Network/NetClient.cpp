#include "NetClient.h"

static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetClient* client = (NetClient*)pInfo->m_info.m_nUserData;

}
ConnectionError NetClient::Create(const char* ip, uint32_t port, NetClient** out)
{
	uint32_t ipAddr = 0;
	{
		int ipLen = strnlen(ip, 100);
		int curIdx = 0;
		for (int i = 0; i < ipLen; i++)
		{
			if (curIdx > 4) return SOCKET_ERROR;

			if (i == 0)
			{
				const int val = atoi(ip + i);
				ipAddr |= val << (8 * curIdx);
				curIdx++;
			}
			else if (ip[i] == '.')
			{
				const int val = atoi(ip + i + 1);
				ipAddr |= val << (8 * curIdx);
				curIdx++;
			}
		}
	}

	*out = nullptr;
	NetClient* temp = new NetClient();
	temp->socket.networking = SteamNetworkingSockets();

	
	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.SetIPv4(ipAddr, port);
	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
	HSteamListenSocket sock = temp->socket.networking->CreateListenSocketIP(addr, 1, &opt);
	if (sock == k_HSteamListenSocket_Invalid)
	{
		delete temp;
		return SOCKET_ERROR;
	}
	temp->socket.socket = sock;
	temp->socket.networking->SetConnectionUserData(sock, (int64)temp);



	*out = temp;
	return ConnectionError::OK;
}

NetClient::~NetClient()
{
}

bool NetClient::IsConnected() const
{
	return false;
}

Connection* NetClient::GetSelf()
{
	return nullptr;
}

Connection* NetClient::GetConnection(uint16_t id)
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
	return false;
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