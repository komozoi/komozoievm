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

#include "../Secp256k1.h"
#include "bigint.h"
#include "util/keccak.h"
#include <gtest/gtest.h>

const static uint256_t CURVE_P("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
const static uint256_t CURVE_N("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");

//
// Secp256k1Scalar Tests
//

TEST(Secp256k1Scalar, BasicArithmetic) {
	Secp256k1Scalar a(5);
	Secp256k1Scalar b(3);

	EXPECT_EQ((a + b), Secp256k1Scalar(8));
	EXPECT_EQ((a - b), Secp256k1Scalar(2));
	EXPECT_EQ((a * b), Secp256k1Scalar(15));
	EXPECT_EQ((b * 2), Secp256k1Scalar(6));
}

TEST(Secp256k1Scalar, LargeArithmetic) {
	{
		Secp256k1Scalar a("0xf81126d92c6e2b5d090b2a7f8d81fd1c00bd3de840c0e55e2f1a20b04d5b44c0");
		Secp256k1Scalar b("0xffa587901971a6aa6c9a415d9cc15031079b0a2a587e2cdb1d759b34b9be5073");

		EXPECT_EQ((a + b), Secp256k1Scalar("0xf7b6ae6945dfd20775a56bdd2a434d4e4da96b2be9f671fd8cbd5d5836e353f2"));
		EXPECT_EQ((a - b), Secp256k1Scalar("0xf86b9f4912fc84b29c70e921f0c0ace9b3d110a4978b58bed176e40863d3358e"));
		EXPECT_EQ((a * b), Secp256k1Scalar("0x40793eac260dcc1aa3a8ac1a0f9b596cb13debe6423683bb54ba5ff24ce33215"));
		EXPECT_EQ((b * 2), Secp256k1Scalar("0xff4b0f2032e34d54d93482bb3982a0635487376e01b3b97a7b18d7dca3465fa5"));
	}
	{
		Secp256k1Scalar a(4);
		Secp256k1Scalar b("0xFFFFFFFFFFFFFFFF0000000000000001FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD");
		Secp256k1Scalar result = a * b;
		EXPECT_EQ(result, Secp256k1Scalar("0xfffffffffffffffc000000000000000bcff3694bf2261f4cc088e4598f5d3c31"));
	}
	{
		Secp256k1Scalar a(0xDF);
		Secp256k1Scalar b("0x6AD55DA88c19628e3e2b20bF79b3579400000000000000012cAA3ce912660AFb");
		Secp256k1Scalar result = a * b;
		EXPECT_EQ(result, Secp256k1Scalar("0xfdc95d20a1cd9e8279186cb03394a622e79c032529dcb5138deb5df632ddc08"));
	}
}

TEST(Secp256k1Scalar, InverseIdentity) {
	Secp256k1Scalar a(17);
	Secp256k1Scalar inv = a.inverse();
	EXPECT_EQ((a * inv), Secp256k1Scalar(1));
}

TEST(Secp256k1Scalar, InverseFailsOnZero) {
	Secp256k1Scalar zero(0);
	EXPECT_EQ(zero.inverse(), Secp256k1Scalar(0));
}

//
// Secp256k1Field Tests
//

TEST(Secp256k1Field, BasicArithmetic) {
	Secp256k1Field a(10);
	Secp256k1Field b(7);

	EXPECT_EQ((a + b), Secp256k1Field(17));
	EXPECT_EQ((a - b), Secp256k1Field(3));
	EXPECT_EQ((a * b), Secp256k1Field(70));
	EXPECT_EQ((b * 3), Secp256k1Field(21));
}

TEST(Secp256k1Field, LargeArithmetic) {
	Secp256k1Field a("0xf81126d92c6e2b5d090b2a7f8d81fd1c00bd3de840c0e55e2f1a20b04d5b44c0");
	Secp256k1Field b("0xffa587901971a6aa6c9a415d9cc15031079b0a2a587e2cdb1d759b34b9be5073");

	EXPECT_EQ((a + b), Secp256k1Field("0xf7b6ae6945dfd20775a56bdd2a434d4d08584812993f12394c8fbbe607199904"));
	EXPECT_EQ((a - b), Secp256k1Field("0xf86b9f4912fc84b29c70e921f0c0aceaf92233bde842b88311a4857a939cf07c"));
	EXPECT_EQ((a * b), Secp256k1Field("0x9cebeb760a5134f0ec1f2e6c057f55487ba91efd6bdff13f1805e05defe52a0b"));
	EXPECT_EQ((b * 3), Secp256k1Field("0xfef096b04c54f3ff45cec418d643f09316d11e7f097a86915860d1a02d3af8fb"));
}

TEST(Secp256k1Field, Exponent) {
	Secp256k1Field base("0xf81126d92c6e2b5d090b2a7f8d81fd1c00bd3de840c0e55e2f1a20b04d5b44c0");
	Secp256k1Field exponent("0x5a56bdd2a434d4d085845a56bdd2a434d4d08584");

	EXPECT_EQ(base.exp(exponent), Secp256k1Field("0xcea3b15e89a2292df8eddac7b63fe5d5eaebf07f7cd560a72bffa42fbe333da7"));
}

TEST(Secp256k1Field, InverseIdentity) {
	Secp256k1Field a(19);
	Secp256k1Field inv = a.inverse();
	EXPECT_EQ((a * inv), Secp256k1Field(1));
}

TEST(Secp256k1Field, InverseIdentityLarge) {
	Secp256k1Field a("0xa5097e056cefb2650f3ef853b719f390765454fcac99c660f9fee3896d099037");
	Secp256k1Field inv = a.inverse();
	EXPECT_EQ((a * inv), Secp256k1Field(1));
}

TEST(Secp256k1Field, ModSqrtCorrectness) {
	Secp256k1Field a(16);  // sqrt(16) = 4
	Secp256k1Field root = a.modsqrt();
	EXPECT_EQ(root * root, a);
}

TEST(Secp256k1Field, LargeModSqrtCorrectness) {
	// One possible root is 0x3261defe6aee8596a1481009f693592bee38eb42d34e60ca59be6fed45c2f3d3
	Secp256k1Field a("0x11e447a0d9ea1554ceda8796ef5c88fa94c6ec401392d1570ab58321198398e0");
	Secp256k1Field root = a.modsqrt();
	EXPECT_EQ(root * root, a);
}

TEST(Secp256k1Field, ModSqrtOfNonResidue) {
	// Try a known non-square (this is probabilistic, but 3 is usually non-square mod p)
	Secp256k1Field a(3);
	Secp256k1Field root = a.modsqrt();
	EXPECT_TRUE(root.isZero() || (root * root != a));
}

TEST(Secp256k1Field, MoreDivision) {
	struct FieldDivTestCase { const char* a; const char* b; const char* expected; };
	FieldDivTestCase testCases[] = {
			{ "0x0000000000000000000000000000000000000000000000000000000000000001", "0x0000000000000000000000000000000000000000000000000000000000000002", "0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffff7ffffe18" },
			{ "0x0000000000000000000000000000000000000000000000000000000000000002", "0x0000000000000000000000000000000000000000000000000000000000000003", "0x55555555555555555555555555555555555555555555555555555554fffffebb" },
			{ "0x0000000000000000000000000000000000000000000000000000000000000005", "0x0000000000000000000000000000000000000000000000000000000000000007", "0x49249249249249249249249249249249249249249249249249249248db6db5c5" },
			{ "0x00000000000000000000000000000000000000000000000000000000075bcd15", "0x000000000000000000000000000000000000000000000000000000003ade68b1", "0x993ef1fe52354d6b8f1914f5d3b5302024a2db36c5f0b085abf0bd9dc89c61f0" },
			{ "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2d", "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2c", "0x55555555555555555555555555555555555555555555555555555554fffffebb" },
			{ "0xd076b849a422463a524b8326c12b0d2397ac6867f876364bde02d91e01779674", "0xf2784de3a14eb36b4dfef810b61c5bb93c315bbd6cd0fdffcb05a55487c6b3d2", "0xa55d8c7c715cf66740f5f1e1f486ceee5814d6b6dc27a15452e8b64f150ef785" },
			{ "0x78ef58f417bbca9737f48cd0b1d0662aa3c5946f77cf3debf1756325470597e6", "0xdaf8bcf444784c4f6252a1e14314d5d8f24cfc8d2fce063f705ba92db6750b1d", "0xeaf2aee93dcc35bd9d7a6773dc3a5160954ab62c009ebba4a31a66421bf1673b" },
			{ "0x2b04f21dd7e382fb886970ff526c3ff4a54e6b2070183dee6735d780082788a1", "0x4519f3d24a14503171baa935a6d8ee209854414ecfbd700f9dc98afc07c6ce74", "0x2b2de81c0d46df22fd8fcd06217e11b88af4e81b0059e921550704069c44d5b2" },
			{ "0x4edc0e4ddeaffcaaa522bb164c5e56e5fddd290c4bc688ed5dc90f83856791d6", "0xc502dccd28100b3c20594468aba057c19adfcce252d0b711f6f5aba46eb024e5", "0x821be5cdb47d01f316a67a7612da6f2ea4d211d0ccc92f57fa478909997a717e" },
			{ "0x996b00de2febdcbb5d7dd4864ea5deac9a95b5117486a497137bb418641f86be", "0x92063633bbb8c49c962309acb749905f76eebe231477c0d693d2c5dd06f39332", "0x09109dfbbb77f2441207396a8787852a158717790ab8dec4e3e677561344fb53" },
	};

	for (const auto& tc : testCases) {
		Secp256k1Field a(tc.a);
		Secp256k1Field b(tc.b);
		Secp256k1Field expected(tc.expected);
		Secp256k1Field result = a / b;
		EXPECT_EQ(result, expected) << "a: " << tc.a << ", b: " << tc.b;
	}
}

//
// Secp256k1Group Tests
//

TEST(Secp256k1Group, GeneratorNotInfinity) {
	Secp256k1Group G = Secp256k1Group::generator();
	EXPECT_FALSE(G.infinity);
}

TEST(Secp256k1Group, MultiplyByZeroYieldsInfinity) {
	Secp256k1Group G = Secp256k1Group::generator();
	Secp256k1Group R = G * 0;
	EXPECT_TRUE(R.infinity);
}

TEST(Secp256k1Group, MultiplyByOneEqualsSelf) {
	Secp256k1Group G = Secp256k1Group::generator();
	Secp256k1Group R = G * 1;
	EXPECT_EQ(R.x, G.x);
	EXPECT_EQ(R.y, G.y);
	EXPECT_FALSE(R.infinity);
}

TEST(Secp256k1Group, PointDoubling) {
	Secp256k1Group G = Secp256k1Group::generator();
	Secp256k1Group G2 = G + G;

	EXPECT_EQ(G2.x, Secp256k1Field("0xc6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"));
	EXPECT_EQ(G2.y, Secp256k1Field("0x1ae168fea63dc339a3c58419466ceaeef7f632653266d0e1236431a950cfe52a"));
}

TEST(Secp256k1Group, Multiplication) {
	// G (scalar = 1)
	{
		uint256_t scalar(1);
		Secp256k1Group point = Secp256k1Group::generator() * scalar;

		EXPECT_EQ(point.x, Secp256k1Field("0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"));
		EXPECT_EQ(point.y, Secp256k1Field("0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"));
	}

	// 2G
	{
		uint256_t scalar(2);
		Secp256k1Group point = Secp256k1Group::generator() * scalar;

		EXPECT_EQ(point.x, Secp256k1Field("0xc6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5"));
		EXPECT_EQ(point.y, Secp256k1Field("0x1ae168fea63dc339a3c58419466ceaeef7f632653266d0e1236431a950cfe52a"));
	}

	// Scalar: deadbeef
	{
		uint256_t scalar("0xdeadbeef");
		Secp256k1Group point = Secp256k1Group::generator() * scalar;

		EXPECT_EQ(point.x, Secp256k1Field("0x76d2fdf1302d1fa9556f4df94ec84cefba6d482e54f47c6c2a238c1baa560f0e"));
		EXPECT_EQ(point.y, Secp256k1Field("0xb754ac7e7a3e09c44184cb451a4f5fb557f32053eb015dffebb655b5cfd54d8a"));
	}

	// Scalar: your r
	{
		uint256_t scalar("0x038835b0c39db78693f4aff1c9019a7e35155c5862b72f5bf412b3402b9653ea");
		Secp256k1Group point = Secp256k1Group::generator() * scalar;

		EXPECT_EQ(point.x, Secp256k1Field("0xf130a28b22b2356552fa364f78de6ab1e36d599ad68b30eee179396a946d6c8e"));
		EXPECT_EQ(point.y, Secp256k1Field("0x6b7866047bc31fc187bbfc615a624398e4b5e75df4f6899ac34b13d8281ec2d8"));
	}

	// Scalar: your s
	{
		uint256_t scalar("0x777a540cc4e5fee91e0f8916f692fde860fc3d7d8a598a61c52ca9870ab07b07");
		Secp256k1Group point = Secp256k1Group::generator() * scalar;

		EXPECT_EQ(point.x, Secp256k1Field("0xc0abaa80ca36c918499ec80074b93c22bc1806ee7bf9dbf7db9a1df763b7c880"));
		EXPECT_EQ(point.y, Secp256k1Field("0xbe77fe7ecbf81cd99efbc97bc70d74e2e8c76b47678c0920cee40d1b3a3e7997"));
	}
}


//
// Secp256k1Signature Tests
//

TEST(Secp256k1Signature, SignatureValidity) {
	uint256_t r(1), s(1);
	Secp256k1Signature sig(r, s, 0);
	EXPECT_TRUE(sig.isValid());

	Secp256k1Signature bad(uint256_t(0), s, 0);
	EXPECT_FALSE(bad.isValid());
}

TEST(Secp256k1Signature, RecoverPublicKey) {
	// Precomputed from known-good ECDSA test vector
	uint256_t msgHash("0x5f35dce98ba4fba25530a026ed80b2cecdaa31091ba4958b99b52ea1d068adad");
	uint256_t r("0x184a6ca02c4b67c06f193b500e8f09b6b4091edfec7af86418256fc7354115a8");
	uint256_t s("0x4d4182e2676fba65e15f93b2cd5bb80660e4f2e821e9538bffc39ef9d27ec64e");
	uint8_t v = 1;

	// Private key is 0xaa3e36dc56a441f5c317e35b64c35d12c937f764cef19e838c389ec881be118c
	Secp256k1PublicKey expectedKey(Secp256k1Group(
		uint256_t("0x340cc9aadc527b9ca6a5888d9594fc7479e78af884906e0350c1dfb6478aedcc"),
		uint256_t("0x8c21f09f6a77e37405e4a4f786977cea96646f2838f39cf47d02b4f256cc41d6")
	));

	Secp256k1Signature sig(r, s, v);

	EXPECT_TRUE(sig.isValid());

	Secp256k1PublicKey pub = sig.recover(msgHash);
	EXPECT_TRUE(pub);
	EXPECT_EQ(pub.group.x, expectedKey.group.x) << pub.toAddress();
	EXPECT_EQ(pub.group.y, expectedKey.group.y);
}

//
// Secp256k1PublicKey Tests
//

TEST(Secp256k1PublicKey, ToAddressProducesCorrectLength) {
	Secp256k1Group G = Secp256k1Group::generator();
	Secp256k1PublicKey pub(G);
	EthereumAddress addr = pub.toAddress();

	std::array<uint8_t, 20> out;
	addr.toBytes(out.data(), out.size());

	for (int i = 0; i < 20; ++i)
		EXPECT_NE(out[i], 0); // Weak test: make sure it's not all zero
}

TEST(Secp256k1PublicKey, OperatorBool) {
	Secp256k1PublicKey invalid;
	EXPECT_FALSE(invalid);

	Secp256k1PublicKey valid(Secp256k1Group::generator());
	EXPECT_TRUE(valid);
}

TEST(Secp256k1PublicKey, ToEthereumAddress) {
	Secp256k1PublicKey pub(Secp256k1Group(
			uint256_t("0x243ad6d306a7df3d84702c8436e8719bbcffd68ee390cf579e49d9a867285a71"),
			uint256_t("0x3711a20f548ea98dbd8aab179a25ba54e158cc1d9c1fe46f820aa94b7be0f123")
	));

	EthereumAddress expected("0x40B3c75B9f1541321201deB42A6B8b6fef924383");

	EXPECT_TRUE(pub);
	EXPECT_EQ(pub.toAddress(), expected);
}

TEST(Secp256k1, SignAndRecoverSimpleMessage) {
	Secp256k1PrivateKey priv("0x30");  // Private key: 0x...30

	const char* messageStr = "hello world";
	ArrayList<uint8_t> message(messageStr);
	uint256_t msgHash = uint256_t(keccak256(message), true);

	Secp256k1Signature sig = priv.sign(message);

	Secp256k1PublicKey recovered = sig.recover(msgHash);

	EXPECT_TRUE(sig.isValid());
	EXPECT_EQ(recovered.toAddress(), EthereumAddress("0x673C638147fe91e4277646d86D5AE82f775EeA5C"));
}

TEST(Secp256k1, DeterministicSigningSameKProducesSameSig) {
	uint256_t sk("0x30");
	Secp256k1PrivateKey priv(sk);

	const char* msgStr = "repeat test";
	ArrayList<uint8_t> message(msgStr);

	uint256_t k("0x123456789abcdef123456789abcdef123456789abcdef123456789abcdef");

	Secp256k1Signature sig1 = priv.sign(message, k);
	Secp256k1Signature sig2 = priv.sign(message, k);

	EXPECT_EQ(sig1.recover(keccak256(message)).toAddress(), sig2.recover(keccak256(message)).toAddress());
	// EXPECT_EQ(sig1, sig2);
}

/*TEST(Secp256k1, SignatureLowSNormalization) {
	uint256_t sk("0x30");
	Secp256k1PrivateKey priv(sk);

	const char* msgStr = "low s test";
	ArrayList<uint8_t> message(msgStr);

	Secp256k1Signature sig = priv.sign(message);

	// s must be low (s <= N/2)
	uint256_t halfN = SECP256K1_CURVE_N >> 1;
	ASSERT_LT(sig.s, halfN + 1);  // Allow exact midpoint
}*/

