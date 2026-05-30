// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-23
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//       http://www.apache.org/licenses/LICENSE-2.0
//  
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
//

#include "Secp256k1.h"
#include "util/keccak.h"
#include "secrandom.h"

#include <memory>



// ChatGPT gave me these numbers.
// Doubting the abilities of an LLM, I removed the values and re-copied them from the Bitcoin wiki.
// Then I decided to double-check that the numbers are the ones ChatGPT expected
// The code ChatGPT originally generated was not off on any of these numbers by even a digit.
// Impressive memory, even if the thinking is retarded.
const uint256_t SECP256K1_CURVE_P("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
const uint256_t SECP256K1_CURVE_N("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
const static Secp256k1Field CURVE_GX("0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
const static Secp256k1Field CURVE_GY("0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");



Secp256k1Group Secp256k1Group::operator+(const Secp256k1Group& other) const {
	if (infinity) return other;
	if (other.infinity) return *this;

	if (x == other.x) {
		if (y != other.y) {
			return Secp256k1Group(); // P + (-P) = ∞
		}
		if (y.isZero()) {
			return Secp256k1Group(); // Tangent is vertical
		}
		// Point doubling
		Secp256k1Field s = (x * x * 3) / (y * 2);
		Secp256k1Field nx = s * s - x - x;
		Secp256k1Field ny = s * (x - nx) - y;
		return Secp256k1Group(nx, ny);
	}

	// Regular addition
	Secp256k1Field s = (other.y - y) / (other.x - x);
	Secp256k1Field nx = s * s - x - other.x;
	Secp256k1Field ny = s * (x - nx) - y;
	return Secp256k1Group(nx, ny);
}

Secp256k1Group Secp256k1Group::operator*(const uint256_t& scalar) const {
	Secp256k1Group result;        // starts as point at infinity
	Secp256k1Group addend = *this;

	for (int i = 255; i >= 0; --i) {
		result = result + result; // result *= 2

		if (scalar.getBit(i)) {
			result = result + addend;
		}
	}
	return result;
}

Secp256k1Group Secp256k1Group::generator() {
	return Secp256k1Group(CURVE_GX, CURVE_GY);
}

Secp256k1Group Secp256k1Group::operator-() const {
	if (infinity)
		return *this;
	return Secp256k1Group(x, Secp256k1Field(SECP256K1_CURVE_P - y));
}

Secp256k1Group Secp256k1Group::operator-(const Secp256k1Group &other) const {
	return *this + (-other);
}


EthereumAddress Secp256k1PublicKey::toAddress() const {
	uint8_t bytes[64];
	group.x.toBytes(&bytes[0x00], true);
	group.y.toBytes(&bytes[0x20], true);
	LongKey<256> hash = keccak256(bytes, sizeof(bytes));
	return EthereumAddress(&hash.data.rawBytes[12], 20, true);
}

Secp256k1PublicKey::operator bool() const {
	return !group.infinity;
	/*  Alternative implementation to check more stuff
	if (group.infinity) return false;

	// Optional: validate the point lies on the curve
	auto x = group.x;
	auto y = group.y;

	if ((y * y) != (x * x * x + Secp256k1Field(7))) return false;

	return true;
	 */
}


Secp256k1PublicKey Secp256k1Signature::recover(const uint256_t& msgHash) const {
	Secp256k1Field rx(r);
	if (rx >= SECP256K1_CURVE_P)
		return {}; // Optional check

	// y^2 = x^3 + 7 mod p
	Secp256k1Field y_squared = rx * rx * rx + Secp256k1Field(7);

	Secp256k1Field ry = y_squared.modsqrt();
	if (ry.isZero())
		return {}; // No square root exists

	// Adjust for correct y parity based on v (which is already 0 or 1)
	if (ry.getBit(0) == v)
		ry = Secp256k1Field(SECP256K1_CURVE_P - ry);

	Secp256k1Group R(rx, ry);

	Secp256k1Scalar rInv = Secp256k1Scalar(r).inverse();
	if (rInv.isZero()) return {};

	uint256_t z = msgHash % SECP256K1_CURVE_N;

	// All scalar mult happens on the group level
	Secp256k1Group G = Secp256k1Group::generator();
	Secp256k1Group sR = R * s;
	Secp256k1Group zG = G * (SECP256K1_CURVE_N - z);

	Secp256k1Group pub = (sR - zG) * rInv;
	if (pub.infinity) return {};

	return Secp256k1PublicKey(-pub);
}


bool Secp256k1Signature::isValid() const {
	return r > 0 && r < SECP256K1_CURVE_N &&
		   s > 0 && s < SECP256K1_CURVE_N;
}


Secp256k1Signature Secp256k1PrivateKey::sign(const LongKey<256>& hash, const uint256_t& k) const {
	uint256_t z = uint256_t(hash, true);

	// Generate random K if desired
	Secp256k1Scalar kScalar = Secp256k1Scalar(k);
	if (k.isZero())
		getSecureRandomBytes(kScalar.data.rawBytes, 32);

	Secp256k1Group R;
	Secp256k1Scalar r(0);
	while (r.isZero()) {
		R = Secp256k1Group::generator() * kScalar;
		r = R.x;

		if (r.isZero())
			kScalar = kScalar + uint256_t(1);
	}

	Secp256k1Scalar s = (r*scalar + z) / kScalar;
	if (s.isZero())
		// Retry if s == 0
		return sign(hash, uint256_t(kScalar) + 1);

	// Enforce low-s rule
	if (s.getBit(255))
		s = SECP256K1_CURVE_N - s;

	// Compute recovery id v (0 or 1)
	for (uint8_t recovery_id = 0; recovery_id < 2; ++recovery_id) {
		Secp256k1Signature sig(r, s, recovery_id);
		Secp256k1PublicKey recovered = sig.recover(z);
		if (recovered && recovered.group.x == (Secp256k1Group::generator() * scalar).x)
			return sig;
	}

	// Something must have gone very wrong.
	return Secp256k1Signature(0, 0, 0);
}

Secp256k1Signature Secp256k1PrivateKey::sign(const uint8_t* message, int n, const uint256_t& k) const {
	return sign(keccak256(message, n));
}

void modReduce256k1FromParts(uint256_t& low, uint64_t high) {
	while (high) {
		uint64_t* lowPtr = low.data.chunks;

		__uint128_t tmp = (__uint128_t)high * 0x1000003d1 + low.data.chunks[0];
		*lowPtr = (uint64_t)tmp;
		uint64_t carry = (uint64_t)(tmp >> 64);

		for (int i = 1; i < 4 && carry; i++) {
			uint64_t sum = *(++lowPtr) + carry;
			if (sum <= *lowPtr)
				carry = 1;
			else
				carry = 0;
			*lowPtr = sum;
		}

		high = carry;
	}

	// Simple subtraction modulus
	if (low >= SECP256K1_CURVE_P)
		low -= SECP256K1_CURVE_P;
}


void modReduce256k1FromParts(uint256_t& low, uint256_t& high) {

	// Compute 256-bit folding
	uint64_t carry = 0;
	for (int i = 0; i < 4; i++) {
		__uint128_t tmp = (__uint128_t)high.data.chunks[i] * 0x1000003d1 + carry + low.data.chunks[i];
		low.data.chunks[i] = (uint64_t)tmp;
		carry = (uint64_t)(tmp >> 64);
	}

	// Compute 64-bit folding
	modReduce256k1FromParts(low, carry);
}


uint256_t Secp256k1Field::modReduce256k1(UnsignedFixedWidthBigInt<8> v) {
	//return uint256_t(v % UnsignedFixedWidthBigInt<8>(SECP256K1_CURVE_P));
	// Split T into high and low 256-bit parts
	uint256_t low;
	uint256_t high;

	for (int i = 0; i < 4; ++i) {
		low.data.chunks[i] = v.data.chunks[i];
		high.data.chunks[i] = v.data.chunks[i + 4];
	}

	modReduce256k1FromParts(low, high);

	return low;
}

uint256_t Secp256k1Field::modReduce256k1(uint256_t v) {
	if (v >= SECP256K1_CURVE_P)
		v -= SECP256K1_CURVE_P;
	return v;
}

Secp256k1Field Secp256k1Field::operator*(const uint256_t& other) const  {
	Secp256k1Field accumulator(0);
	uint256_t folder(0);

	uint64_t* temp1 = accumulator.data.chunks;
	uint64_t* temp2 = &folder.data.chunks[-4];

	for (int i = 0; i < 4; ++i) {
		__uint128_t a = this->data.chunks[i];
		uint64_t carry = 0;

		for (int j = 0; j < 4 - i; ++j) {
			__uint128_t prod = a * other.data.chunks[j] + temp1[j] + carry;
			temp1[j] = (uint64_t)prod;
			carry = (uint64_t)(prod >> 64);
		}

		for (int j = 4 - i; j < 4; ++j) {
			__uint128_t prod = a * other.data.chunks[j] + temp2[j] + carry;
			temp2[j] = (uint64_t)prod;
			carry = (uint64_t)(prod >> 64);
		}

		if (carry)
			temp2[4] = carry;

		temp1++;
		temp2++;
	}

	modReduce256k1FromParts(accumulator, folder);

	return accumulator;
}

Secp256k1Field Secp256k1Field::operator*(int other) const  {
	Secp256k1Field accumulator;

	// Compute 256-bit folding
	uint64_t carry = 0;
	for (int i = 0; i < 4; i++) {
		__uint128_t tmp = (__uint128_t)this->data.chunks[i] * other + carry;
		accumulator.data.chunks[i] = (uint64_t)tmp;
		carry = (uint64_t)(tmp >> 64);
	}

	modReduce256k1FromParts(accumulator, carry);

	return accumulator;
}
