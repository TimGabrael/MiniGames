#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Cryptography.h"

enum class NetError
{
	OK,
	E_INIT,			// failed to initialize
	E_RESOLVE,		// failed to Resolve the address
	E_CONNECT,		// failed to Connect
	E_LISTEN,		// failed to Listen
	E_READ,			// failed to Read bytes from socket
	E_HANDSHAKE,	// failed to perform handshake
	E_VERSION,		// failed Version mismatch
};

enum class PacketID
{
	HANDSHAKE = 0,
	CHECK = 1,

};

struct PacketHeader
{
	PacketHeader() { type = 0; size = 0; }
	constexpr PacketHeader(uint32_t t, uint32_t sz) : type(t), size(sz) { }
	constexpr PacketHeader(PacketID t, uint32_t sz) : type((uint32_t)t), size(sz) { }
	uint32_t type;
	uint32_t size;
};
struct Packet
{
	PacketHeader header;
	std::vector<char> body;
};

struct ValidationPacket
{
	uint8_t PacketData[64];
};

// if this is changed, KEY_LEN needs to be changed as well !
#define KEY_BITS_512
#ifdef KEY_BITS_128
typedef uint128_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU128;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU128;
#elif defined(KEY_BITS_256)
typedef uint256_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU256; 
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU256;
#elif defined(KEY_BITS_512)
typedef uint512_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU512;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU512;
#elif defined(KEY_BITS_1024)
typedef uint1024_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU1024;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU1024;
#elif defined(KEY_BITS_2048)
typedef uint2048_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU2048;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU2048;
#elif defined(KEY_BITS_4096)
typedef uint4096_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU4096;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU4096;
#endif





typedef void(*ConnectionFunc)(void* obj, struct Connection* conn);
typedef void(*PacketCallback)(void* userData, Packet* pack);

struct Connection
{
	Connection();
	Connection(uintptr_t sock);
	Connection(uintptr_t sock, ConnectionFunc socketCallback);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);

	void Init(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);



	void SetCryptoKey(const ukey_t& k);

	CryptoContext ctx;
	uintptr_t socket;
	std::thread listener;
	Packet storedPacket;
	ukey_t sharedSecret;
};

class TCPSocket
{
public:
	TCPSocket();
	~TCPSocket() { running = false; }
	NetError Connect(const char* host, const char* port);
	void Disconnect();

	void SetPacketCallback(PacketCallback cb, void* userData = nullptr);

	void SendData(PacketID id, uint32_t size, const uint8_t* data);

private:
	void SendDataUnencrypted(PacketID id, uint32_t size, const uint8_t* data);
	static void SocketListenFunc(void* obj, struct Connection* conn);
	uintptr_t sock;
	PacketCallback packCb = nullptr;
	void* uData = nullptr;
	Connection serverConn;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running = true;
};

class TCPServerSocket
{
public:
	TCPServerSocket();
	NetError Create(const char* host, const char* port);
	NetError AcceptConnection();

	void SetPacketCallback(PacketCallback cb, void* userData = nullptr);

	void RemoveAll();

	void SendData(uint32_t clientIdx, PacketID id, size_t size, const uint8_t* data);

	static void TCPServerListenToClient(void* server, Connection* conn);

private:


	void AddClient(uintptr_t connSocket, const ukey_t& key);
	void RemoveClient(uintptr_t connSocket);
	void RemoveClientAtIdx(size_t idx);

	void RemoveInListener(uintptr_t connSocket);

	PacketCallback packetCallback;
	void* userData;
	uintptr_t sock;
	std::mutex clientMut;
	std::vector<Connection> clients;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running;
};

