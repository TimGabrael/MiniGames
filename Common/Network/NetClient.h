#pragma once
#include "NetCommon.h"
#include "Network/NetworkBase.h"



struct NetClient : public NetClientInterface
{
	enum State
	{
		Disconnected,
		Connecting,
		ReceivedAnswer,
		Connected,
	};
	static NetClient* Create();

	NetResult Connect(const char* ip, uint32_t port, const std::string& name);
    void Disconnect();


	virtual ~NetClient();

	virtual bool IsConnected() const override;

	virtual ClientConnection* GetSelf() override;
	virtual ClientConnection* GetConnection(uint16_t id) override;
	virtual void SetCallback(ClientPacketFunction fn, uint16_t packetID) override;
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID) override;

	virtual void SetClientInfoCallback(ClientInfoCallbackFunction fn) override;
	virtual void SetDisconnectCallback(ClientDisconnectCallbackFunction fn) override;

	virtual bool SendDataRaw(uint16_t packetID, const void* data, uint32_t size, uint32_t flags) override;
	virtual bool SendData(uint16_t packetID, google::protobuf::Message* msg, uint32_t flags) override;

	virtual bool IsP2P() const override;


	void Poll();

	void SetLobbyDeserializers();


	NetSocketClient socket;
	ApplicationData* appData = nullptr;
	State connectionState = State::Disconnected;
private:

	static void SteamNetClientConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static bool __stdcall ServerInfoCallback(ApplicationData* app, base::ServerClientInfo* info, int size);

	ClientConnection* local = nullptr;
	ClientDisconnectCallbackFunction disconnectCB = nullptr;
	ClientInfoCallbackFunction infoCB = nullptr;
	struct CallbackInfo
	{
		DeserializationFunc deserializer = nullptr;
		ClientPacketFunction receiver = nullptr;
	};
	std::vector<CallbackInfo> callbacks;
	uint8_t tempStorage[MAX_MESSAGE_LENGTH + 20];
	NetClient() {};

};

