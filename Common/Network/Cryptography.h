#pragma once
#include <stdint.h>
#include <stddef.h>
#include "logging.h"

#define KEY_LEN 64
#define KEY_EXP_LEN 176
#define BLOCK_LEN 16

class CryptoContext
{
public:
	CryptoContext() {};
	void InitContext(const uint8_t key[KEY_LEN]);
	void EncryptBuffer(uint8_t* buf, size_t length);	// length >= BLOCK_LEN !!!, otherwise buffer remains unencrypted
	void DecryptBuffer(uint8_t* buf, size_t length);	// length >= BLOCK_LEN !!!, otherwise buffer remains unencrypted
private:
	uint8_t key[KEY_EXP_LEN];
};



union uint128_t {
	struct {
		uint64_t low;
		uint64_t high;
	};
	uint8_t byte[16];
};
union uint256_t {
	struct {
		uint128_t low;
		uint128_t high;
	};
	uint8_t byte[32];
};
union uint512_t {
	struct {
		uint256_t low;
		uint256_t high;
	};
	uint8_t byte[64];
};
union uint1024_t {
	struct {
		uint512_t low;
		uint512_t high;
	};
	uint8_t byte[128];
};
union uint2048_t {
	struct {
		uint1024_t low;
		uint1024_t high;
	};
	uint8_t byte[256];
};
union uint4096_t {
	struct {
		uint2048_t low;
		uint2048_t high;
	};
	uint8_t byte[512];
};


void GenKeyPairU128(uint128_t* publicKey, uint128_t* privateKey);
void GenSharedSecretU128(uint128_t* sharedSecret, const uint128_t privateKey, const uint128_t publicKey);
void GenKeyPairU256(uint256_t* publicKey, uint256_t* privateKey);
void GenSharedSecretU256(uint256_t* sharedSecret, const uint256_t privateKey, const uint256_t publicKey);

void GenKeyPairU512(uint512_t* publicKey, uint512_t* privateKey);
void GenSharedSecretU512(uint512_t* sharedSecret, const uint512_t privateKey, const uint512_t publicKey);
void GenKeyPairU1024(uint1024_t* publicKey, uint1024_t* privateKey);
void GenSharedSecretU1024(uint1024_t* sharedSecret, const uint1024_t privateKey, const uint1024_t publicKey);

// STACK OVERFLOW GUARANTEED
void GenKeyPairU2048(uint2048_t* publicKey, uint2048_t* privateKey);
void GenSharedSecretU2048(uint2048_t* sharedSecret, const uint2048_t privateKey, const uint2048_t publicKey);
void GenKeyPairU4096(uint4096_t* publicKey, uint4096_t* privateKey);
void GenSharedSecretU4096(uint4096_t* sharedSecret, const uint4096_t privateKey, const uint4096_t publicKey);