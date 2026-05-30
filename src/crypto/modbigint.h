// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-24
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

#ifndef CMEVBOT_MODBIGINT_H
#define CMEVBOT_MODBIGINT_H

#include "bigint.h"


template <int N, const UnsignedFixedWidthBigInt<N>& P>
class UnsignedModBigInt: public UnsignedFixedWidthBigInt<N> {
public:
	UnsignedModBigInt() = default;
	UnsignedModBigInt(UnsignedFixedWidthBigInt<N> v) : UnsignedFixedWidthBigInt<N>(v % P) {}
	UnsignedModBigInt(int v) : UnsignedFixedWidthBigInt<N>(v) {}

	void addRaw(const UnsignedFixedWidthBigInt<N>& other, uint64_t& carry) {
		carry = 0;
		for (int i = 0; i < N; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t sum = a + b + carry;
			carry = (sum < a || (carry && sum == a)) ? 1 : 0;

			this->data.chunks[i] = sum;
		}
	}

	void subRaw(const UnsignedFixedWidthBigInt<N>& other, uint64_t& borrow) {
		borrow = 0;
		for (int i = 0; i < N; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t diff = a - b - borrow;
			borrow = (a < b || (borrow && a == b)) ? 1 : 0;

			this->data.chunks[i] = diff;
		}
	}

	UnsignedModBigInt<N, P> operator+(const UnsignedFixedWidthBigInt<N>& other) const {
		UnsignedModBigInt<N, P> out = *this;
		uint64_t carry = 0;
		out.addRaw(other, carry);

		// Always reduce if overflow happened — safe because P < 2⁶⁴ⁿ
		if (carry != 0 || out >= P) {
			uint64_t borrow = 0;
			out.subRaw(P, borrow);
		}

		return out;
	}

	UnsignedModBigInt<N, P> operator-(const UnsignedFixedWidthBigInt<N>& other) const {
		UnsignedModBigInt<N, P> result;

		uint64_t borrow = 0;

		for (int i = 0; i < N; ++i) {
			uint64_t a = this->data.chunks[i];
			uint64_t b = other.data.chunks[i];

			uint64_t diff = a - b - borrow;
			borrow = (a < b + borrow) ? 1 : 0;

			result.data.chunks[i] = diff;
		}

		if (borrow) {
			// This means this < other, so we wrap by adding P
			uint64_t carry = 0;
			result.addRaw(P, carry);
		}

		return result;
	}

	UnsignedModBigInt<N, P> operator*(const UnsignedFixedWidthBigInt<N>& other) const {
		UnsignedFixedWidthBigInt<N*2> tmp1(*this);
		UnsignedFixedWidthBigInt<N*2> tmp2(other);
		UnsignedFixedWidthBigInt<N*2> tmpP(P);
		return UnsignedFixedWidthBigInt<N>((tmp1 * tmp2) % tmpP);
	}

	UnsignedModBigInt<N, P> operator*(int other) const {
		UnsignedFixedWidthBigInt<N+1> tmp1(*this);
		UnsignedFixedWidthBigInt<N+1> tmp2(other);
		UnsignedFixedWidthBigInt<N+1> tmpP(P);
		return UnsignedFixedWidthBigInt<N>((tmp1 * tmp2) % tmpP);
	}

	UnsignedModBigInt<N, P> operator/(const UnsignedFixedWidthBigInt<N>& other) const {
		return *this * UnsignedModBigInt<N, P>(other).inverse();
	}

	UnsignedModBigInt<N, P> modinv(const UnsignedFixedWidthBigInt<N>& m) const {
		// Ensure no constructors reduce mod P during init — so we use raw values
		UnsignedFixedWidthBigInt<N> r0 = m;
		UnsignedFixedWidthBigInt<N> r1 = *this;

		UnsignedFixedWidthBigInt<N> t0 = 0;
		UnsignedFixedWidthBigInt<N> t1 = 1;

		UnsignedFixedWidthBigInt<N>* r_old = &r0;
		UnsignedFixedWidthBigInt<N>* r = &r1;

		UnsignedFixedWidthBigInt<N>* t_old = &t0;
		UnsignedFixedWidthBigInt<N>* t = &t1;

		UnsignedFixedWidthBigInt<N>* tmp;

		while (!r->isZero()) {
			UnsignedFixedWidthBigInt<N> q = *r_old / *r;

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
		if (*r_old != UnsignedFixedWidthBigInt<N>(1))
			return 0;

		// Correct for negative result
		if (t_old->getBit(N*64 - 1)) {
			*t_old += m;
		}

		return *t_old;  // Will be implicitly reduced into mod class
	}

	UnsignedFixedWidthBigInt<N> exp(const UnsignedFixedWidthBigInt<N>& exponent) const {

		UnsignedFixedWidthBigInt<N*2> base(*this);
		UnsignedFixedWidthBigInt<N*2> result(1);
		UnsignedFixedWidthBigInt<N*2> tmpModulus(P);

		for (int i = 0; i < N * 64; i++) {
			if (exponent.getBit(i) != 0)
				result = (result * base) % tmpModulus;

			base = (base * base) % tmpModulus;
		}

		return result;
	}

	UnsignedModBigInt<N, P> inverse() const {
		return modinv(P);
	}

	UnsignedModBigInt<N, P> modsqrt() const {
		if (this->isZero())
			return 0;

		// Compute a^((p + 1) / 4) mod p
		static const UnsignedModBigInt<N, P> EXP = (P + 1) >> 2;
		UnsignedModBigInt<N, P> root = exp(EXP);

		// Verify it is a valid square root: root^2 ≡ a mod p
		if (root * root != *this) {
			return 0; // no sqrt exists
		}

		return root;
	}

private:
	// 5 base-2**52 numbers
	//uint64_t n[5];
};

#endif //CMEVBOT_MODBIGINT_H
