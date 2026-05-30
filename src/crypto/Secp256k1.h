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

#ifndef CMEVBOT_SECP256K1_H
#define CMEVBOT_SECP256K1_H

#include <utility>

#include "LongKey.h"
#include "bigint.h"
#include "modbigint.h"
#include "ethereum/ethereum.h"
#include "ds/ArrayList.h"


const extern uint256_t SECP256K1_CURVE_P;
const extern uint256_t SECP256K1_CURVE_N;

typedef UnsignedModBigInt<4, SECP256K1_CURVE_N> Secp256k1Scalar;

class Secp256k1Field: public UnsignedFixedWidthBigInt<4> {
public:
	Secp256k1Field() = default;
	Secp256k1Field(UnsignedFixedWidthBigInt<4> v) : UnsignedFixedWidthBigInt<4>(modReduce256k1(std::move(v))) {}
	Secp256k1Field(int v) : UnsignedFixedWidthBigInt<4>(v) {}

	static UnsignedFixedWidthBigInt<4> modReduce256k1(UnsignedFixedWidthBigInt<4> v);
	static UnsignedFixedWidthBigInt<4> modReduce256k1(UnsignedFixedWidthBigInt<8> v);

	void addRaw(const uint256_t& other, uint64_t& carry) {
		carry = 0;
		for (int i = 0; i < 4; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t sum = a + b + carry;
			carry = (sum < a || (carry && sum == a)) ? 1 : 0;

			this->data.chunks[i] = sum;
		}
	}

	void subRaw(const uint256_t& other, uint64_t& borrow) {
		borrow = 0;
		for (int i = 0; i < 4; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t diff = a - b - borrow;
			borrow = (a < b || (borrow && a == b)) ? 1 : 0;

			this->data.chunks[i] = diff;
		}
	}

	Secp256k1Field operator+(const uint256_t& other) const {
		Secp256k1Field out = *this;
		uint64_t carry = 0;
		out.addRaw(other, carry);

		// Always reduce if overflow happened — safe because P < 2⁶⁴ⁿ
		if (carry != 0 || out >= SECP256K1_CURVE_P) {
			uint64_t borrow = 0;
			out.subRaw(SECP256K1_CURVE_P, borrow);
		}

		return out;
	}

	Secp256k1Field operator-(const uint256_t& other) const {
		Secp256k1Field result;

		uint64_t borrow = 0;

		for (int i = 0; i < 4; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t diff = a - b - borrow;
			borrow = (a < b + borrow) ? 1 : 0;

			result.data.chunks[i] = diff;
		}

		if (borrow) {
			// This means this < other, so we wrap by adding P
			uint64_t carry = 0;
			result.addRaw(SECP256K1_CURVE_P, carry);
		}

		return result;
	}

	Secp256k1Field operator*(const uint256_t& other) const;

	Secp256k1Field operator*(int other) const;

	Secp256k1Field operator/(const uint256_t& other) const {
		/*Secp256k1Field base(other);
		Secp256k1Field result(*this);
		Secp256k1Field exponent(SECP256K1_CURVE_P - uint256_t(2));

		for (int i = 0; i < 33; i++) {
			if (exponent.getBit(i) != 0)
				result = result * base;

			base = base * base;
		}

		for (int i = 33; i < 256; i++) {
			result = result * base;
			base = base * base;
		}

		return result;*/
		return *this * Secp256k1Field(other).inverse();
	}

	Secp256k1Field modinv(const uint256_t& m) const {
		// Ensure no constructors reduce mod P during init — so we use raw values
		uint256_t r0 = m;
		uint256_t r1 = *this;

		uint256_t t0 = 0;
		uint256_t t1 = 1;

		uint256_t* r_old = &r0;
		uint256_t* r = &r1;

		uint256_t* t_old = &t0;
		uint256_t* t = &t1;

		uint256_t* tmp;

		while (!r->isZero()) {
			uint256_t q = *r_old / *r;

			*r_old -= q * *r;
			tmp = r;
			r = r_old;
			r_old = tmp;

			*t_old -= q * *t;
			tmp = t;
			t = t_old;
			t_old = tmp;
		}

		// If not invertible
		if (*r_old != uint256_t(1))
			return 0;

		// Correct for negative result
		if (t_old->getBit(4*64 - 1)) {
			*t_old += m;
		}

		return *t_old;  // Will be implicitly reduced into mod class
	}

	uint256_t exp(const uint256_t& exponent) const {
		Secp256k1Field base(*this);
		Secp256k1Field result(1);

		for (int i = 0; i < 4 * 64; i++) {
			if (exponent.getBit(i) != 0)
				result = result * base;

			base = base * base;
		}

		return result;
	}

	Secp256k1Field inverse() const {
		//return this->exp(SECP256K1_CURVE_P - uint256_t(2));
		return modinv(SECP256K1_CURVE_P);
	}

	Secp256k1Field modsqrt() const {
		if (this->isZero())
			return 0;

		// Compute a^((p + 1) / 4) mod p
		static const Secp256k1Field EXP = (SECP256K1_CURVE_P + 1) >> 2;
		Secp256k1Field root = exp(EXP);

		// Verify it is a valid square root: root^2 ≡ a mod p
		if (root * root != *this) {
			return 0; // no sqrt exists
		}

		return root;
	}
};



class Secp256k1Group {
public:
	Secp256k1Group() : x(0), y(0), infinity(true) {}
	Secp256k1Group(Secp256k1Field x, Secp256k1Field y) : x(std::move(x)), y(std::move(y)), infinity(false) {}

	Secp256k1Group operator+(const Secp256k1Group& other) const;
	Secp256k1Group operator-() const;
	Secp256k1Group operator-(const Secp256k1Group& other) const;
	Secp256k1Group operator*(const uint256_t& scalar) const;

	static Secp256k1Group generator();

public:
	Secp256k1Field x;
	Secp256k1Field y;
	bool infinity;
};


class Secp256k1PublicKey {
public:
	inline Secp256k1PublicKey() = default;
	explicit inline Secp256k1PublicKey(Secp256k1Group group) : group(std::move(group)) {}

	EthereumAddress toAddress() const;

	operator bool() const;

	const Secp256k1Group group;
};


class Secp256k1Signature {
public:
	inline Secp256k1Signature(uint256_t  r, uint256_t  s, uint8_t v)
		: r(std::move(r)), s(std::move(s)), v(v) {}

	Secp256k1PublicKey recover(const uint256_t& messageHash) const;

	bool isValid() const;

	const uint256_t r, s;
	const uint8_t v;
};


class Secp256k1PrivateKey {
public:
	explicit Secp256k1PrivateKey(uint256_t sk) : scalar(std::move(sk)) {}

	Secp256k1Signature sign(const LongKey<256>& hash, const uint256_t& k = 0) const;
	Secp256k1Signature sign(const uint8_t* message, int length, const uint256_t& k = 0) const;
	inline Secp256k1Signature sign(const ArrayList<uint8_t>& message, const uint256_t& k = 0) const {
		return sign(message.getMemory(), message.size(), k);
	}

	const Secp256k1Scalar& getScalar() const { return scalar; }

private:
	Secp256k1Scalar scalar;
};


#endif //CMEVBOT_SECP256K1_H
