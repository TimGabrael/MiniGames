#include "Cryptography.h"
#include <string>

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
	for(size_t i = 0; i < _block_len; i++)
	{
		Cipher((state*)(buf + i * BLOCK_LEN), this->key);
	}
}
void CryptoContext::DecryptBuffer(uint8_t* buf, size_t length)
{
	const size_t _block_len = length / BLOCK_LEN;
	for (size_t i = 0; i < _block_len; i++)
	{
		InvCipher((state*)(buf + i * BLOCK_LEN), this->key);
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












void GenKeyPairU128(uint128_t* publicKey, uint128_t* privateKey)
{
	for (int i = 0; i < 16; i++) privateKey->byte[i] = rand();
	u128_ExponentiateModulateP(publicKey, u128G, *privateKey);
}

void GenSharedSecretU128(uint128_t* sharedSecret, const uint128_t privateKey, const uint128_t publicKey)
{
	u128_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}
void GenKeyPairU256(uint256_t* publicKey, uint256_t* privateKey)
{
	for (int i = 0; i < 32; i++) privateKey->byte[i] = rand();
	u256_ExponentiateModulateP(publicKey, u256G, *privateKey);
}

void GenSharedSecretU256(uint256_t* sharedSecret, const uint256_t privateKey, const uint256_t publicKey)
{
	u256_ExponentiateModulateP(sharedSecret, publicKey, privateKey);
}
