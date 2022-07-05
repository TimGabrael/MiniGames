#include "Cryptography.h"
#include <string>
#include <random>

template<typename T>
void PrintKey(const T& k)
{
	static constexpr uint32_t num64 = sizeof(T) / 8;
	const uint64_t* k64 = (uint64_t*)k.byte;
	for (uint32_t i = 0; i < num64; i++)
	{
		LOG("%016llX ", k64[i]);
	}
	LOG("\n");
}


#define Nb 4
#define Nk 4        // The number of 32 bit words in a key.
#define Nr 10       // The number of rounds in AES Cipher.
typedef uint8_t state[4][4];

static const uint8_t sbox[256] = {
	//0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 
};
static const uint8_t rsbox[256] = {
  0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d 
};
static const uint8_t Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 
};

static void KeyExpansion(uint8_t ctxKey[KEY_EXP_LEN], const uint8_t key[KEY_LEN])
{
	uint8_t temp[4];

	for (int i = 0; i < Nk; i++)
	{
		ctxKey[i * 4] = key[i * 4];
		ctxKey[i * 4 + 1] = key[i * 4 + 1];
		ctxKey[i * 4 + 2] = key[i * 4 + 2];
		ctxKey[i * 4 + 3] = key[i * 4 + 3];
	}
	for (int i = Nk; i < Nb * (Nr + 1); i++)
	{
		{
			const int k = (i - 1) * 4;
			temp[0] = ctxKey[k];
			temp[1] = ctxKey[k + 1];
			temp[2] = ctxKey[k + 2];
			temp[3] = ctxKey[k + 3];
		}
		if ((i % Nk) == 0)
		{
			const uint8_t u8tmp = temp[0];
			temp[0] = temp[1];
			temp[1] = temp[2];
			temp[2] = temp[3];
			temp[3] = u8tmp;
		}
		temp[0] = sbox[temp[0]];
		temp[1] = sbox[temp[1]];
		temp[2] = sbox[temp[2]];
		temp[3] = sbox[temp[3]];

		temp[0] = temp[0] ^ Rcon[i / Nk];
		const int j = i * 4;
		const int k = (i - Nk) * 4;
		ctxKey[j] = ctxKey[k] ^ temp[0];
		ctxKey[j + 1] = ctxKey[k + 1] ^ temp[0];
		ctxKey[j + 2] = ctxKey[k + 2] ^ temp[0];
		ctxKey[j + 3] = ctxKey[k + 3] ^ temp[0];
	}
}

static void AddRoundKey(uint8_t round, state* s, const uint8_t ctxKey[KEY_EXP_LEN])
{
	for (uint8_t i = 0; i < 4; i++)
	{
		for(uint8_t j = 0; j < 4; j++)
		{
			(*s)[i][j] ^= ctxKey[(round * Nb * 4) + (i * Nb) + j];
		}
	}
}
static void SubBytes(state* s)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		for (uint8_t j = 0; j < 4; j++)
		{
			(*s)[j][i] = sbox[((*s)[j][i])];
		}
	}
}
static void InvSubBytes(state* s)
{
	for (uint8_t i = 0; i < 4; ++i)
	{
		for (uint8_t j = 0; j < 4; ++j)
		{
			(*s)[j][i] = rsbox[((*s)[j][i])];
		}
	}
}
static void ShiftRows(state* s)
{
	uint8_t temp = (*s)[0][1];
	(*s)[0][1] = (*s)[1][1];
	(*s)[1][1] = (*s)[2][1];
	(*s)[2][1] = (*s)[3][1];
	(*s)[3][1] = temp;

	temp = (*s)[0][2];
	(*s)[0][2] = (*s)[2][2];
	(*s)[2][2] = temp;

	temp = (*s)[1][2];
	(*s)[1][2] = (*s)[3][2];
	(*s)[3][2] = temp;

	temp = (*s)[0][3];
	(*s)[0][3] = (*s)[3][3];
	(*s)[3][3] = (*s)[2][3];
	(*s)[2][3] = (*s)[1][3];
	(*s)[1][3] = temp;
}
static void InvShiftRows(state* s)
{
	uint8_t temp;
	temp = (*s)[3][1];
	(*s)[3][1] = (*s)[2][1];
	(*s)[2][1] = (*s)[1][1];
	(*s)[1][1] = (*s)[0][1];
	(*s)[0][1] = temp;

	temp = (*s)[0][2];
	(*s)[0][2] = (*s)[2][2];
	(*s)[2][2] = temp;

	temp = (*s)[1][2];
	(*s)[1][2] = (*s)[3][2];
	(*s)[3][2] = temp;

	temp = (*s)[0][3];
	(*s)[0][3] = (*s)[1][3];
	(*s)[1][3] = (*s)[2][3];
	(*s)[2][3] = (*s)[3][3];
	(*s)[3][3] = temp;
}
static uint8_t xtime(uint8_t x)
{
	return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}
static void MixColumns(state* s)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		uint8_t first = (*s)[i][0];
		uint8_t tmp = (*s)[i][0] ^ (*s)[i][1] ^ (*s)[i][2] ^ (*s)[i][3];

		uint8_t xtmp = (*s)[i][0] ^ (*s)[i][1];
		xtmp = xtime(xtmp);
		(*s)[i][0] ^= xtmp ^ tmp;

		xtmp = (*s)[i][1] ^ (*s)[i][2];
		xtmp = xtime(xtmp);
		(*s)[i][1] ^= xtmp ^ tmp;
		
		xtmp = (*s)[i][2] ^ (*s)[i][3];
		xtmp = xtime(xtmp);
		(*s)[i][2] ^= xtmp ^ tmp;

		xtmp = (*s)[i][3] ^ first;
		xtmp = xtime(xtmp);
		(*s)[i][3] ^= xtmp ^ tmp;
	}
}
#define Multiply(x, y)                                \
      (  ((y & 1) * x) ^                              \
      ((y>>1 & 1) * xtime(x)) ^                       \
      ((y>>2 & 1) * xtime(xtime(x))) ^                \
      ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^         \
      ((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))

static void InvMixColumns(state* s)
{
	for (int i = 0; i < 4; i++)
	{
		uint8_t a = (*s)[i][0];
		uint8_t b = (*s)[i][1];
		uint8_t c = (*s)[i][2];
		uint8_t d = (*s)[i][3];

		(*s)[i][0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
		(*s)[i][1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
		(*s)[i][2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
		(*s)[i][3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
	}
}


static void Cipher(state* s, const uint8_t ctxKey[KEY_EXP_LEN])
{
	AddRoundKey(0, s, ctxKey);
	for (uint8_t round = 1; ; round++)
	{
		SubBytes(s);
		ShiftRows(s);
		if (round == Nr) {
			break;
		}
		MixColumns(s);
		AddRoundKey(round, s, ctxKey);
	}
	AddRoundKey(Nr, s, ctxKey);
}
static void InvCipher(state* s, const uint8_t ctxKey[KEY_EXP_LEN])
{
	AddRoundKey(Nr, s, ctxKey);
	for (uint8_t round = (Nr - 1); ; round--)
	{
		InvShiftRows(s);
		InvSubBytes(s);
		AddRoundKey(round, s, ctxKey);
		if (round == 0)	{
			break;
		}
		InvMixColumns(s);
	}

}

void CryptoContext::InitContext(const uint8_t key[KEY_LEN])
{
	KeyExpansion(this->key, key);
}

void CryptoContext::EncryptBuffer(uint8_t* buf, size_t length)
{
	const size_t _block_len = length / BLOCK_LEN;
	const size_t mod = length % BLOCK_LEN;
	for(size_t i = 0; i < _block_len; i++)
	{
		Cipher((state*)(buf + i * BLOCK_LEN), this->key);
	}
	if (mod != 0)
	{
		if (length >= BLOCK_LEN)
		{
			Cipher((state*)(buf + length - BLOCK_LEN), this->key);
		}
	}
}
void CryptoContext::DecryptBuffer(uint8_t* buf, size_t length)
{
	const size_t _block_len = length / BLOCK_LEN;
	const size_t mod = length % BLOCK_LEN;
	if(mod != 0)
	{
		if (length >= BLOCK_LEN)
		{
			InvCipher((state*)(buf + length - BLOCK_LEN), this->key);
		}
	}
	for (size_t i = 0; i < _block_len; i++)
	{
		InvCipher((state*)(buf + i * BLOCK_LEN), this->key);
	}
}












void GenerateComplement(uint64_t* result, const uint64_t* input, uint64_t size64)
{
	for (uint64_t i = 0; i < size64; i++)
	{
		result[i] = ~input[i];
	}
}

// 128-BIT VERSION
/* P =  2^128-159 = 0xffffffffffffffffffffffffffffff61 (The biggest 128bit prime) */
static const uint128_t u128P = { 0xffffffffffffff61ULL, 0xffffffffffffffffULL };
static const uint128_t u128INVERT_P = { 159 };
static const uint128_t u128G = { 27 };
bool u128_IsZero(const uint128_t val)
{
	return (val.low == 0 && val.high == 0);
}
bool u128_IsOdd(const uint128_t val)
{
	return (val.low & 1);
}
void u128_LeftShift(uint128_t* val)
{
	uint64_t t = (val->low >> 63) & 1;
	val->high = (val->high << 1) | t;
	val->low = (val->low << 1);
}
void u128_RightShift(uint128_t* val)
{
	uint64_t t = (val->high & 1) << 63;
	val->high = val->high >> 1;
	val->low = (val->low >> 1) | t;
}
int u128_Compare(const uint128_t v1, const uint128_t v2)
{
	if (v1.high > v2.high) return 1;
	else if (v1.high == v2.high) {
		if (v1.low > v2.low) return 1;
		else if (v1.low == v2.low) return 0;
		else return -1;
	}
	else return -1;
}
void u128_Add(uint128_t* res, const uint128_t a, const uint128_t b)
{
	uint64_t overflow = 0;
	uint64_t low = a.low + b.low;
	if (low < a.low || low < b.low) overflow = 1;
	res->low = low;
	res->high = a.high + b.high + overflow;
}
void u128_Add_L(uint128_t* res, const uint128_t a, const uint64_t b)
{
	uint64_t overflow = 0;
	uint64_t low = a.low + b;
	if (low < a.low || low < b) overflow = 1;
	res->low = low; res->high = a.high + overflow;
}
void u128_Sub(uint128_t* res, const uint128_t a, const uint128_t b)
{
	uint128_t invert_b;
	invert_b.low = ~b.low;
	invert_b.high = ~b.high;
	u128_Add_L(&invert_b, invert_b, 1);
	u128_Add(res, a, invert_b);
}
void u128_MultiplyModulateP(uint128_t* res, uint128_t a, uint128_t b)
{
	uint128_t t;
	uint128_t double_a;
	uint128_t P_a;
	res->low = res->high = 0;
	while (!u128_IsZero(b))
	{
		if (u128_IsOdd(b)) {
			u128_Sub(&t, u128P, a);
			if (u128_Compare(*res, t) >= 0) u128_Sub(res, *res, t);
			else u128_Add(res, *res, a);
		}
		double_a = a;
		u128_LeftShift(&double_a);
		u128_Sub(&P_a, u128P, a);
		if (u128_Compare(a, P_a) >= 0) u128_Add(&a, double_a, u128INVERT_P);
		else a = double_a;
		u128_RightShift(&b);
	}
}
void u128_ExponentiateModulateP_R(uint128_t* res, const uint128_t a, const uint128_t b)
{
	uint128_t t; uint128_t half_b = b;
	if (b.high == 0 && b.low == 1) {
		*res = a; return;
	}
	u128_RightShift(&half_b);
	u128_ExponentiateModulateP_R(&t, a, half_b);
	u128_MultiplyModulateP(&t, t, t);
	if (u128_IsOdd(b)) u128_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u128_ExponentiateModulateP(uint128_t* res, uint128_t a, uint128_t b)
{
	if (u128_Compare(a, u128P) > 0) u128_Sub(&a, a, u128P);
	u128_ExponentiateModulateP_R(res, a, b);
}




// 256-BIT VERSION
/* P =  2^256 - 189 = 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff43 (The biggest 256bit prime) */
static const uint256_t u256P = { 0xffffffffffffff43ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL };
static const uint256_t u256INVERT_P = { 189 };
static const uint256_t u256G = { 27 };

bool u256_IsZero(const uint256_t val)
{
	return (u128_IsZero(val.low) && u128_IsZero(val.high));
}
bool u256_IsOdd(const uint256_t val)
{
	return (val.low.low & 1);
}
void u256_LeftShift(uint256_t* val)
{
	uint64_t t = (val->low.high >> 63) & 1;
	u128_LeftShift(&val->high);
	val->high.low |= t;
	u128_LeftShift(&val->low);
}
void u256_RightShift(uint256_t* val)
{
	uint64_t t = (val->high.low & 1) << 63;
	u128_RightShift(&val->high);
	u128_RightShift(&val->low);
	val->low.high |= t;
}
int u256_Compare(const uint256_t v1, const uint256_t v2)
{
	const int compResultHigh = u128_Compare(v1.high, v2.high);
	if (compResultHigh > 0) return 1;
	else if (compResultHigh == 0) {
		const int compResultLow = u128_Compare(v1.low, v2.low);
		if (compResultLow > 0) return 1;
		else if (compResultLow == 0) return 0;
		else return -1;
	}
	else return -1;
}
void u256_Add(uint256_t* res, const uint256_t a, const uint256_t b)
{
	uint128_t overflow = { 0, 0 };
	uint128_t low;
	u128_Add(&low, a.low, b.low);
	if (u128_Compare(low, a.low) < 0 || u128_Compare(low, b.low) < 0) overflow.low = 1;
	res->low = low;
	u128_Add(&res->high, a.high, b.high);
	u128_Add(&res->high, res->high, overflow);
}
void u256_Add_L(uint256_t* res, const uint256_t a, const uint128_t b)
{
	uint128_t overflow = { 0, 0 };
	uint128_t low;
	u128_Add(&low, a.low, b);
	if (u128_Compare(low, a.low) < 0 || u128_Compare(low, b) < 0) overflow.low = 1;

	res->low = low;
	u128_Add(&res->high, a.high, overflow);
}
void u256_Sub(uint256_t* res, const uint256_t a, const uint256_t b)
{
	uint256_t invert_b;
	invert_b.low.low = ~b.low.low;
	invert_b.low.high = ~b.low.high;
	invert_b.high.low = ~b.high.low;
	invert_b.high.high = ~b.high.high;
	u256_Add_L(&invert_b, invert_b, { 1 });
	u256_Add(res, a, invert_b);
}
void u256_MultiplyModulateP(uint256_t* res, uint256_t a, uint256_t b)
{
	uint256_t t;
	uint256_t double_a;
	uint256_t P_a;
	res->low.low = res->low.high = res->high.low = res->high.high = 0;
	while (!u256_IsZero(b))
	{
		if (u256_IsOdd(b)) {
			u256_Sub(&t, u256P, a);
			if (u256_Compare(*res, t) >= 0) u256_Sub(res, *res, t);
			else u256_Add(res, *res, a);
		}
		double_a = a;
		u256_LeftShift(&double_a);
		u256_Sub(&P_a, u256P, a);
		if (u256_Compare(a, P_a) >= 0) u256_Add(&a, double_a, u256INVERT_P);
		else a = double_a;
		u256_RightShift(&b);
	}
}
void u256_ExponentiateModulateP_R(uint256_t* res, const uint256_t a, const uint256_t b)
{
	uint256_t t; uint256_t half_b = b;
	if (b.high.high == 0 && b.high.low == 0 && b.low.high == 0 && b.low.low == 1) {
		*res = a; return;
	}
	u256_RightShift(&half_b);
	u256_ExponentiateModulateP_R(&t, a, half_b);
	u256_MultiplyModulateP(&t, t, t);
	if (u256_IsOdd(b)) u256_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u256_ExponentiateModulateP(uint256_t* res, uint256_t a, uint256_t b)
{
	if (u256_Compare(a, u256P) > 0) u256_Sub(&a, a, u256P);
	u256_ExponentiateModulateP_R(res, a, b);
}




// 512-BIT VERSION
static const uint512_t u512P =        { 0x81cde7a5ff6742e9ULL, 0xdc93b6bb9b12db12ULL, 0x145dff4ca5721cbeULL,
0x1e94b44d2bc26bc2ULL, 0x918925b0b3e4e7eaULL, 0x6b2f712e3f2742c9ULL, 0xac3a19a62b1cf8edULL, 0xe1a70311f8ac6dc8ULL };
static const uint512_t u512INVERT_P = { 0x7e32185a0098bd17ULL, 0x236c494464ed24edULL, 0xeba200b35a8de341ULL,
0xe16b4bb2d43d943dULL, 0x6e76da4f4c1b1815ULL, 0x94d08ed1c0d8bd36ULL, 0x53c5e659d4e30712ULL, 0x1e58fcee07539237ULL };
static const uint512_t u512G = { 27 };

bool u512_IsZero(const uint512_t val)
{
	return (u256_IsZero(val.low) && u256_IsZero(val.high));
}
bool u512_IsOdd(const uint512_t val)
{
	return (val.low.low.low & 1);
}
void u512_LeftShift(uint512_t* val)
{
	uint64_t t = (val->low.high.high >> 63) & 1;
	u256_LeftShift(&val->high);
	val->high.low.low |= t;
	u256_LeftShift(&val->low);
}
void u512_RightShift(uint512_t* val)
{
	uint64_t t = (val->high.low.low & 1) << 63;
	u256_RightShift(&val->high);
	u256_RightShift(&val->low);
	val->low.high.high |= t;
}
int u512_Compare(const uint512_t v1, const uint512_t v2)
{
	const int compResultHigh = u256_Compare(v1.high, v2.high);
	if (compResultHigh > 0) return 1;
	else if (compResultHigh == 0) {
		const int compResultLow = u256_Compare(v1.low, v2.low);
		if (compResultLow > 0) return 1;
		else if (compResultLow == 0) return 0;
		else return -1;
	}
	else return -1;
}
void u512_Add(uint512_t* res, const uint512_t a, const uint512_t b)
{
	uint256_t overflow = { 0 };
	uint256_t low;
	u256_Add(&low, a.low, b.low);
	if (u256_Compare(low, a.low) < 0 || u256_Compare(low, b.low) < 0) { overflow.low.low = 1; }

	res->low = low;
	u256_Add(&res->high, a.high, b.high);
	u256_Add(&res->high, res->high, overflow);
}
void u512_Add_L(uint512_t* res, const uint512_t a, const uint256_t b)
{
	uint256_t overflow = { 0 };
	uint256_t low;
	u256_Add(&low, a.low, b);
	if (u256_Compare(low, a.low) < 0 || u256_Compare(low, b) < 0) overflow.low.low = 1;

	res->low = low;
	u256_Add(&res->high, a.high, overflow);
}
void u512_Sub(uint512_t* res, const uint512_t a, const uint512_t b)
{
	uint512_t invert_b;
	GenerateComplement((uint64_t*)&invert_b.byte, (const uint64_t*)b.byte, 8);
	u512_Add_L(&invert_b, invert_b, { 1 });
	u512_Add(res, a, invert_b);
}
void u512_MultiplyModulateP(uint512_t* res, uint512_t a, uint512_t b)
{
	uint512_t t;
	uint512_t double_a;
	uint512_t P_a;
	memset(res->byte, 0, sizeof(uint512_t));
	while (!u512_IsZero(b))
	{
		if (u512_IsOdd(b)) {
			u512_Sub(&t, u512P, a);
			if (u512_Compare(*res, t) >= 0) u512_Sub(res, *res, t);
			else u512_Add(res, *res, a);
		}
		double_a = a;
		u512_LeftShift(&double_a);
		u512_Sub(&P_a, u512P, a);
		if (u512_Compare(a, P_a) >= 0) u512_Add(&a, double_a, u512INVERT_P);
		else a = double_a;
		u512_RightShift(&b);
	}
}
void u512_ExponentiateModulateP_R(uint512_t* res, const uint512_t a, const uint512_t b)
{
	static uint512_t one = { 1 };
	uint512_t t; uint512_t half_b = b; 

	if (u512_Compare(b, one) == 0) {
		*res = a; return;
	}
	u512_RightShift(&half_b);
	u512_ExponentiateModulateP_R(&t, a, half_b);
	u512_MultiplyModulateP(&t, t, t);
	if (u512_IsOdd(b)) u512_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u512_ExponentiateModulateP(uint512_t* res, uint512_t a, uint512_t b)
{
	if (u512_Compare(a, u512P) > 0) u512_Sub(&a, a, u512P);
	u512_ExponentiateModulateP_R(res, a, b);
}




// 1024-BIT VERSION
static const uint1024_t u1024P = { 0x3761772ce0ca9e77ULL, 0x785e0f4405eae608ULL, 0xe01cfabed74ed729ULL, 0x16a2b6ffde879696ULL, 0x88a9a9a349c7e5e7ULL, 0xe22b0f7d900f6dbbULL, 0x7c27ddc4bf7e2698ULL,
0x84c74121837fdfd3ULL, 0x2d1b1aa94f7ce437ULL, 0x04cb89be6a3de189ULL, 0x1b251d404f8560cdULL, 0xdc97d21dc9d2ac1bULL, 0x839bd8aaadddacdfULL, 0x8958d4e088f6d127ULL, 0x2492525604131cacULL, 0x9d0d7c2b06b29d5cULL };
static const uint1024_t u1024INVERT_P = { 0xc89e88d31f356189ULL, 0x87a1f0bbfa1519f7ULL, 0x1fe3054128b128d6ULL, 0xe95d490021786969ULL, 0x7756565cb6381a18ULL, 0x1dd4f0826ff09244ULL, 0x83d8223b4081d967ULL,
0x7b38bede7c80202cULL, 0xd2e4e556b0831bc8ULL, 0xfb34764195c21e76ULL, 0xe4dae2bfb07a9f32ULL, 0x23682de2362d53e4ULL, 0x7c64275552225320ULL, 0x76a72b1f77092ed8ULL, 0xdb6dada9fbece353ULL, 0x62f283d4f94d62a3ULL };
static const uint1024_t u1024G = { 27 };

bool u1024_IsZero(const uint1024_t val)
{
	return (u512_IsZero(val.low) && u512_IsZero(val.high));
}
bool u1024_IsOdd(const uint1024_t val)
{
	return (val.low.low.low.low & 1);
}
void u1024_LeftShift(uint1024_t* val)
{
	uint64_t t = (val->low.high.high.high >> 63) & 1;
	u512_LeftShift(&val->high);
	val->high.low.low.low |= t;
	u512_LeftShift(&val->low);
}
void u1024_RightShift(uint1024_t* val)
{
	uint64_t t = (val->high.low.low.low & 1) << 63;
	u512_RightShift(&val->high);
	u512_RightShift(&val->low);
	val->low.high.high.high |= t;
}
int u1024_Compare(const uint1024_t v1, const uint1024_t v2)
{
	const int compResultHigh = u512_Compare(v1.high, v2.high);
	if (compResultHigh > 0) return 1;
	else if (compResultHigh == 0) {
		const int compResultLow = u512_Compare(v1.low, v2.low);
		if (compResultLow > 0) return 1;
		else if (compResultLow == 0) return 0;
		else return -1;
	}
	else return -1;
}
void u1024_Add(uint1024_t* res, const uint1024_t a, const uint1024_t b)
{
	uint512_t overflow = { 0, 0 };
	uint512_t low;
	u512_Add(&low, a.low, b.low);
	if (u512_Compare(low, a.low) < 0 || u512_Compare(low, b.low) < 0) overflow.low.low.low = 1;
	res->low = low;
	u512_Add(&res->high, a.high, b.high);
	u512_Add(&res->high, res->high, overflow);
}
void u1024_Add_L(uint1024_t* res, const uint1024_t a, const uint512_t b)
{
	uint512_t overflow = { 0, 0 };
	uint512_t low;
	u512_Add(&low, a.low, b);
	if (u512_Compare(low, a.low) < 0 || u512_Compare(low, b) < 0) overflow.low.low.low = 1;

	res->low = low;
	u512_Add(&res->high, a.high, overflow);
}
void u1024_Sub(uint1024_t* res, const uint1024_t a, const uint1024_t b)
{
	uint1024_t invert_b;
	GenerateComplement((uint64_t*)invert_b.byte, (const uint64_t*)b.byte, 16);
	u1024_Add_L(&invert_b, invert_b, { 1 });
	u1024_Add(res, a, invert_b);
}
void u1024_MultiplyModulateP(uint1024_t* res, uint1024_t a, uint1024_t b)
{
	uint1024_t t;
	uint1024_t double_a;
	uint1024_t P_a;
	memset(res->byte, 0, sizeof(uint1024_t));
	while (!u1024_IsZero(b))
	{
		if (u1024_IsOdd(b)) {
			u1024_Sub(&t, u1024P, a);
			if (u1024_Compare(*res, t) >= 0) u1024_Sub(res, *res, t);
			else u1024_Add(res, *res, a);
		}
		double_a = a;
		u1024_LeftShift(&double_a);
		u1024_Sub(&P_a, u1024P, a);
		if (u1024_Compare(a, P_a) >= 0) u1024_Add(&a, double_a, u1024INVERT_P);
		else a = double_a;
		u1024_RightShift(&b);
	}
}
void u1024_ExponentiateModulateP_R(uint1024_t* res, const uint1024_t a, const uint1024_t b)
{
	static uint1024_t one = { 1 };
	uint1024_t t; uint1024_t half_b = b; 

	if (u1024_Compare(b, one) == 0) {
		*res = a; return;
	}
	u1024_RightShift(&half_b);
	u1024_ExponentiateModulateP_R(&t, a, half_b);
	u1024_MultiplyModulateP(&t, t, t);
	if (u1024_IsOdd(b)) u1024_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u1024_ExponentiateModulateP(uint1024_t* res, uint1024_t a, uint1024_t b)
{
	if (u1024_Compare(a, u1024P) > 0) u1024_Sub(&a, a, u1024P);
	u1024_ExponentiateModulateP_R(res, a, b);
}





// 2048-BIT VERSION
static const uint2048_t u2048P = { 0x5252e727f8fb4f8bULL, 0x0337999422336008ULL, 0x4a95ab581b570c02ULL, 0x9731e59922cce108ULL, 0x6b7557e98635e6f2ULL, 0x6c81780641d1be66ULL, 0x06412be3b0a6a399ULL,
0x1127415ec1bd860aULL, 0x2e25cd677d696d17ULL, 0x38da29ed1fcb30ecULL, 0x6e7618bd93f483d0ULL, 0xe95c4a6b815e80c9ULL, 0xe5e9d8798bc024edULL, 0xdc4937977646c8a5ULL, 0x5d603c761fd8b3eeULL, 0x5c2e9fde473615e2ULL,
0x2eb1fdab2c555122ULL, 0x6d792ae043b4fdccULL, 0x37b89c490ebcece4ULL, 0x80eaddebcaa38dcdULL, 0x958127ed38cd95acULL, 0x5d61b358be81f493ULL, 0xa63dfbb61898e3aeULL, 0xad5dfb45de33ad2cULL, 0x882c7c38cf1cc536ULL,
0xc6647c3929a73ca4ULL, 0x24f534ca37ff94d3ULL, 0xd8c2642da1692627ULL, 0xb92d2124c98933e1ULL, 0x91566616a643a4a7ULL, 0xc2c253d7a3604e82ULL, 0xb6dfbded19a21ef4ULL };
static const uint2048_t u2048INVERT_P = { 0xadad18d80704b075ULL, 0xfcc8666bddcc9ff7ULL, 0xb56a54a7e4a8f3fdULL, 0x68ce1a66dd331ef7ULL, 0x948aa81679ca190dULL, 0x937e87f9be2e4199ULL, 0xf9bed41c4f595c66ULL,
0xeed8bea13e4279f5ULL, 0xd1da3298829692e8ULL, 0xc725d612e034cf13ULL, 0x9189e7426c0b7c2fULL, 0x16a3b5947ea17f36ULL, 0x1a162786743fdb12ULL, 0x23b6c86889b9375aULL, 0xa29fc389e0274c11ULL, 0xa3d16021b8c9ea1dULL,
0xd14e0254d3aaaeddULL, 0x9286d51fbc4b0233ULL, 0xc84763b6f143131bULL, 0x7f152214355c7232ULL, 0x6a7ed812c7326a53ULL, 0xa29e4ca7417e0b6cULL, 0x59c20449e7671c51ULL, 0x52a204ba21cc52d3ULL, 0x77d383c730e33ac9ULL,
0x399b83c6d658c35bULL, 0xdb0acb35c8006b2cULL, 0x273d9bd25e96d9d8ULL, 0x46d2dedb3676cc1eULL, 0x6ea999e959bc5b58ULL, 0x3d3dac285c9fb17dULL, 0x49204212e65de10bULL };
static const uint2048_t u2048G = { 27 };

bool u2048_IsZero(const uint2048_t val)
{
	return (u1024_IsZero(val.low) && u1024_IsZero(val.high));
}
bool u2048_IsOdd(const uint2048_t val)
{
	return (val.low.low.low.low.low & 1);
}
void u2048_LeftShift(uint2048_t* val)
{
	uint64_t t = (val->low.high.high.high.high >> 63) & 1;
	u1024_LeftShift(&val->high);
	val->high.low.low.low.low |= t;
	u1024_LeftShift(&val->low);
}
void u2048_RightShift(uint2048_t* val)
{
	uint64_t t = (val->high.low.low.low.low & 1) << 63;
	u1024_RightShift(&val->high);
	u1024_RightShift(&val->low);
	val->low.high.high.high.high |= t;
}
int u2048_Compare(const uint2048_t v1, const uint2048_t v2)
{
	const int compResultHigh = u1024_Compare(v1.high, v2.high);
	if (compResultHigh > 0) return 1;
	else if (compResultHigh == 0) {
		const int compResultLow = u1024_Compare(v1.low, v2.low);
		if (compResultLow > 0) return 1;
		else if (compResultLow == 0) return 0;
		else return -1;
	}
	else return -1;
}
void u2048_Add(uint2048_t* res, const uint2048_t a, const uint2048_t b)
{
	uint1024_t overflow = { 0, 0 };
	uint1024_t low;
	u1024_Add(&low, a.low, b.low);
	if (u1024_Compare(low, a.low) < 0 || u1024_Compare(low, b.low) < 0) overflow.low.low.low.low = 1;
	res->low = low;
	u1024_Add(&res->high, a.high, b.high);
	u1024_Add(&res->high, res->high, overflow);
}
void u2048_Add_L(uint2048_t* res, const uint2048_t a, const uint1024_t b)
{
	uint1024_t overflow = { 0, 0 };
	uint1024_t low;
	u1024_Add(&low, a.low, b);
	if (u1024_Compare(low, a.low) < 0 || u1024_Compare(low, b) < 0) overflow.low.low.low.low = 1;

	res->low = low;
	u1024_Add(&res->high, a.high, overflow);
}
void u2048_Sub(uint2048_t* res, const uint2048_t a, const uint2048_t b)
{
	uint2048_t invert_b;
	GenerateComplement((uint64_t*)invert_b.byte, (const uint64_t*)b.byte, 32);
	u2048_Add_L(&invert_b, invert_b, { 1 });
	u2048_Add(res, a, invert_b);
}
void u2048_MultiplyModulateP(uint2048_t* res, uint2048_t a, uint2048_t b)
{
	uint2048_t t;
	uint2048_t double_a;
	uint2048_t P_a;
	memset(res->byte, 0, sizeof(uint2048_t));
	while (!u2048_IsZero(b))
	{
		if (u2048_IsOdd(b)) {
			u2048_Sub(&t, u2048P, a);
			if (u2048_Compare(*res, t) >= 0) u2048_Sub(res, *res, t);
			else u2048_Add(res, *res, a);
		}
		double_a = a;
		u2048_LeftShift(&double_a);
		u2048_Sub(&P_a, u2048P, a);
		if (u2048_Compare(a, P_a) >= 0) u2048_Add(&a, double_a, u2048INVERT_P);
		else a = double_a;
		u2048_RightShift(&b);
	}
}
void u2048_ExponentiateModulateP_R(uint2048_t* res, const uint2048_t a, const uint2048_t b)
{
	struct _funcData {
		uint2048_t t;
		uint2048_t half_b;
	};

	uint2048_t t; uint2048_t half_b = b;
	static uint2048_t one = { 1 };

	if (u2048_Compare(b, one) == 0) {
		*res = a; return;
	}
	u2048_RightShift(&half_b);
	u2048_ExponentiateModulateP_R(&t, a, half_b);
	u2048_MultiplyModulateP(&t, t, t);
	if (u2048_IsOdd(b)) u2048_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u2048_ExponentiateModulateP(uint2048_t* res, uint2048_t a, uint2048_t b)
{
	if (u2048_Compare(a, u2048P) > 0) u2048_Sub(&a, a, u2048P);
	u2048_ExponentiateModulateP_R(res, a, b);
}




// 4096-BIT VERSION
static const uint4096_t u4096P = { 0xc5658c6b87d9a301ULL, 0x1f964b8a6a3a69a1ULL, 0x3291e2370230f950ULL, 0xa072301cf497e45aULL, 0xde3faf86da3f424bULL, 0x8b29ff4e668b77caULL, 0x31ba7ebfb9bb3d14ULL,
0xfdc2b715d68475a9ULL, 0xff3905a10d5b59a8ULL, 0xbd55af0659ac1d22ULL, 0xc9ea6b824d360f7eULL, 0xfd09d4ee2e919ad6ULL, 0x1a29f199e99d3f90ULL, 0xe0b72697c4692f80ULL, 0xa404a42e03a7299aULL, 0x6fddfadfb613461aULL,
0xdc9c7b7f90f7e430ULL, 0x87f072904da4a9cdULL, 0x0aaf3f80e7dcfbe0ULL, 0x9f097645c237cb10ULL, 0x89f26fda07fa41e2ULL, 0xf48667062f50700eULL, 0xe1f07d3621fd9851ULL, 0xad540ffd47746d1dULL, 0x7d6b991dbffcd837ULL,
0x2d05df4d10b74736ULL, 0x2b29288147988ef2ULL, 0x558e498e973fbf14ULL, 0x0550abad52b62c8eULL, 0x933e2696c60d8b54ULL, 0xeb281c33fa985808ULL, 0x6307d7a7190b90d7ULL, 0x67b3a55a03369544ULL, 0xb0a7ad19aaf881d1ULL,
0xed0eaa152afa14e4ULL, 0xb325eb33117a3053ULL, 0xa06f91126943d6c0ULL, 0xbd47f5dfa27b15a3ULL, 0x9122c5798dcb8753ULL, 0xad919e1348ffcae8ULL, 0xa022b9bae9c462c8ULL, 0xcfa1cb4dd0421f23ULL, 0xf0140af3d7594cb2ULL,
0xcc48586cd296d1d0ULL, 0x5f4d80cea6e8f793ULL, 0xa172f63576198b0aULL, 0xb90ae2f8dd8ee5ffULL, 0xdf22adbe4aef8964ULL, 0xe9e4e46509b9b50eULL, 0x80ed1e21e3142544ULL, 0x79242b0baa659175ULL, 0x03ddb725cba28e6bULL,
0x81e0365cc482a1a7ULL, 0xcae91d107ab352afULL, 0x77a9bd84bc2bdd46ULL, 0x2166e03b1b64ac82ULL, 0x951fe5962896c73dULL, 0x8b40c3a8ab8f6f6cULL, 0xea8142a8f80fa684ULL, 0xb1281ebe048cd866ULL, 0xdd58ff7f247a40f1ULL,
0xd92d0e0d826d35d4ULL, 0x52f2a05cf5d6cba5ULL, 0xd52ad85d026b5257ULL };
static const uint4096_t u4096INVERT_P = { 0x3a9a739478265cffULL, 0xe069b47595c5965eULL, 0xcd6e1dc8fdcf06afULL, 0x5f8dcfe30b681ba5ULL, 0x21c0507925c0bdb4ULL, 0x74d600b199748835ULL, 0xce4581404644c2ebULL,
0x023d48ea297b8a56ULL, 0x00c6fa5ef2a4a657ULL, 0x42aa50f9a653e2ddULL, 0x3615947db2c9f081ULL, 0x02f62b11d16e6529ULL, 0xe5d60e661662c06fULL, 0x1f48d9683b96d07fULL, 0x5bfb5bd1fc58d665ULL, 0x9022052049ecb9e5ULL,
0x236384806f081bcfULL, 0x780f8d6fb25b5632ULL, 0xf550c07f1823041fULL, 0x60f689ba3dc834efULL, 0x760d9025f805be1dULL, 0x0b7998f9d0af8ff1ULL, 0x1e0f82c9de0267aeULL, 0x52abf002b88b92e2ULL, 0x829466e2400327c8ULL,
0xd2fa20b2ef48b8c9ULL, 0xd4d6d77eb867710dULL, 0xaa71b67168c040ebULL, 0xfaaf5452ad49d371ULL, 0x6cc1d96939f274abULL, 0x14d7e3cc0567a7f7ULL, 0x9cf82858e6f46f28ULL, 0x984c5aa5fcc96abbULL, 0x4f5852e655077e2eULL,
0x12f155ead505eb1bULL, 0x4cda14ccee85cfacULL, 0x5f906eed96bc293fULL, 0x42b80a205d84ea5cULL, 0x6edd3a86723478acULL, 0x526e61ecb7003517ULL, 0x5fdd4645163b9d37ULL, 0x305e34b22fbde0dcULL, 0x0febf50c28a6b34dULL,
0x33b7a7932d692e2fULL, 0xa0b27f315917086cULL, 0x5e8d09ca89e674f5ULL, 0x46f51d0722711a00ULL, 0x20dd5241b510769bULL, 0x161b1b9af6464af1ULL, 0x7f12e1de1cebdabbULL, 0x86dbd4f4559a6e8aULL, 0xfc2248da345d7194ULL,
0x7e1fc9a33b7d5e58ULL, 0x3516e2ef854cad50ULL, 0x8856427b43d422b9ULL, 0xde991fc4e49b537dULL, 0x6ae01a69d76938c2ULL, 0x74bf3c5754709093ULL, 0x157ebd5707f0597bULL, 0x4ed7e141fb732799ULL, 0x22a70080db85bf0eULL,
0x26d2f1f27d92ca2bULL, 0xad0d5fa30a29345aULL, 0x2ad527a2fd94ada8ULL };

static const uint4096_t u4096G = { 27 };

bool u4096_IsZero(const uint4096_t val)
{
	return (u2048_IsZero(val.low) && u2048_IsZero(val.high));
}
bool u4096_IsOdd(const uint4096_t val)
{
	return (val.low.low.low.low.low.low & 1);
}
void u4096_LeftShift(uint4096_t* val)
{
	uint64_t t = (val->low.high.high.high.high.high >> 63) & 1;
	u2048_LeftShift(&val->high);
	val->high.low.low.low.low.low |= t;
	u2048_LeftShift(&val->low);
}
void u4096_RightShift(uint4096_t* val)
{
	uint64_t t = (val->high.low.low.low.low.low & 1) << 63;
	u2048_RightShift(&val->high);
	u2048_RightShift(&val->low);
	val->low.high.high.high.high.high |= t;
}
int u4096_Compare(const uint4096_t v1, const uint4096_t v2)
{
	const int compResultHigh = u2048_Compare(v1.high, v2.high);
	if (compResultHigh > 0) return 1;
	else if (compResultHigh == 0) {
		const int compResultLow = u2048_Compare(v1.low, v2.low);
		if (compResultLow > 0) return 1;
		else if (compResultLow == 0) return 0;
		else return -1;
	}
	else return -1;
}
void u4096_Add(uint4096_t* res, const uint4096_t a, const uint4096_t b)
{
	uint2048_t overflow = { 0, 0 };
	uint2048_t low;
	u2048_Add(&low, a.low, b.low);
	if (u2048_Compare(low, a.low) < 0 || u2048_Compare(low, b.low) < 0) overflow.low.low.low.low.low = 1;
	res->low = low;
	u2048_Add(&res->high, a.high, b.high);
	u2048_Add(&res->high, res->high, overflow);
}
void u4096_Add_L(uint4096_t* res, const uint4096_t a, const uint2048_t b)
{
	uint2048_t overflow = { 0, 0 };
	uint2048_t low;
	u2048_Add(&low, a.low, b);
	if (u2048_Compare(low, a.low) < 0 || u2048_Compare(low, b) < 0) overflow.low.low.low.low.low = 1;

	res->low = low;
	u2048_Add(&res->high, a.high, overflow);
}
void u4096_Sub(uint4096_t* res, const uint4096_t a, const uint4096_t b)
{
	uint4096_t invert_b;
	GenerateComplement((uint64_t*)invert_b.byte, (const uint64_t*)b.byte, 64);
	u4096_Add_L(&invert_b, invert_b, { 1 });
	u4096_Add(res, a, invert_b);
}
void u4096_MultiplyModulateP(uint4096_t* res, uint4096_t a, uint4096_t b)
{
	uint4096_t t;
	uint4096_t double_a;
	uint4096_t P_a;
	memset(res->byte, 0, sizeof(uint4096_t));
	while (!u4096_IsZero(b))
	{
		if (u4096_IsOdd(b)) {
			u4096_Sub(&t, u4096P, a);
			if (u4096_Compare(*res, t) >= 0) u4096_Sub(res, *res, t);
			else u4096_Add(res, *res, a);
		}
		double_a = a;
		u4096_LeftShift(&double_a);
		u4096_Sub(&P_a, u4096P, a);
		if (u4096_Compare(a, P_a) >= 0) u4096_Add(&a, double_a, u4096INVERT_P);
		else a = double_a;
		u4096_RightShift(&b);
	}
}
void u4096_ExponentiateModulateP_R(uint4096_t* res, const uint4096_t a, const uint4096_t b)
{
	uint4096_t t; uint4096_t half_b = b; 
	static uint4096_t one = { 1 };

	if (u4096_Compare(b, one) == 0) {
		*res = a; return;
	}
	u4096_RightShift(&half_b);
	u4096_ExponentiateModulateP_R(&t, a, half_b);
	u4096_MultiplyModulateP(&t, t, t);
	if (u4096_IsOdd(b)) u4096_MultiplyModulateP(&t, t, a);
	*res = t;
}
void u4096_ExponentiateModulateP(uint4096_t* res, uint4096_t a, uint4096_t b)
{
	if (u4096_Compare(a, u4096P) > 0) u4096_Sub(&a, a, u4096P);
	u4096_ExponentiateModulateP_R(res, a, b);
}
















uint8_t GenerateRandomNumber()
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 0xFF);
	return dist(rng);
}


void GenKeyPairU128(uint128_t* publicKey, uint128_t* privateKey)
{
	for (int i = 0; i < 16; i++) privateKey->byte[i] = GenerateRandomNumber();
	u128_ExponentiateModulateP(publicKey, u128G, *privateKey);
}
void GenSharedSecretU128(uint128_t* sharedSecret, const uint128_t privateKey, const uint128_t publicKey)
{
	u128_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}

void GenKeyPairU256(uint256_t* publicKey, uint256_t* privateKey)
{
	for (int i = 0; i < 32; i++) privateKey->byte[i] = GenerateRandomNumber();
	u256_ExponentiateModulateP(publicKey, u256G, *privateKey);
}
void GenSharedSecretU256(uint256_t* sharedSecret, const uint256_t privateKey, const uint256_t publicKey)
{
	u256_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}


void GenKeyPairU512(uint512_t* publicKey, uint512_t* privateKey)
{
	for (int i = 0; i < 64; i++) privateKey->byte[i] = GenerateRandomNumber();
	u512_ExponentiateModulateP(publicKey, u512G, *privateKey);
}
void GenSharedSecretU512(uint512_t* sharedSecret, const uint512_t privateKey, const uint512_t publicKey)
{
	u512_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}
void GenKeyPairU1024(uint1024_t* publicKey, uint1024_t* privateKey)
{
	for (int i = 0; i < 128; i++) privateKey->byte[i] = GenerateRandomNumber();
	u1024_ExponentiateModulateP(publicKey, u1024G, *privateKey);
}
void GenSharedSecretU1024(uint1024_t* sharedSecret, const uint1024_t privateKey, const uint1024_t publicKey)
{
	u1024_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}



void GenKeyPairU2048(uint2048_t* publicKey, uint2048_t* privateKey)
{
	for (int i = 0; i < 256; i++) privateKey->byte[i] = GenerateRandomNumber();
	u2048_ExponentiateModulateP(publicKey, u2048G, *privateKey);
}
void GenSharedSecretU2048(uint2048_t* sharedSecret, const uint2048_t privateKey, const uint2048_t publicKey)
{
	u2048_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}

void GenKeyPairU4096(uint4096_t* publicKey, uint4096_t* privateKey)
{
	for (int i = 0; i < 512; i++) privateKey->byte[i] = GenerateRandomNumber();
	u4096_ExponentiateModulateP(publicKey, u4096G, *privateKey);
}
void GenSharedSecretU4096(uint4096_t* sharedSecret, const uint4096_t privateKey, const uint4096_t publicKey)
{
	u4096_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}

