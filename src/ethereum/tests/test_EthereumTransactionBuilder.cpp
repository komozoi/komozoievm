// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-06
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

#include <gtest/gtest.h>
#include "ethereum/EthereumTransactionBuilder.h"
#include "crypto/keys.h"

// Mock or real implementations of these types must exist
// If not, you'll need to mock or create stubs

TEST(EthereumTransactionBuilderTest, ConstructorInitializesFieldsCorrectly) {
	EthereumAddress dst("0x1234567890abcdef1234567890abcdef12345678");
	uint64_t nonce = 1;
	uint32_t gasLimit = 21000;

	EthereumTransactionBuilder builder(dst, nonce, gasLimit);

	EXPECT_EQ(builder.dst, dst);
	EXPECT_EQ(builder.nonce, nonce);
	EXPECT_EQ(builder.gasInfo.gasLimit, gasLimit);
	EXPECT_TRUE(builder.value.isZero());
	EXPECT_EQ(builder.calldata.size(), 0);
	EXPECT_EQ(builder.accessList.size(), 0);
}

static const uint8_t rawTxHex1[] = {
		0x02, 0xf8, 0x72, 0x01, 0x0a, 0x84, 0x3b, 0x9a, 0xca, 0x00, 0x84, 0x3f, 0x8d, 0xe4, 0x08, 0x82,
		0x53, 0x30, 0x94, 0x00, 0x00, 0xc1, 0x6a, 0x54, 0x72, 0x4c, 0xf8, 0xc7, 0x34, 0x69, 0xea, 0x6b,
		0xdd, 0x76, 0x3b, 0xfb, 0x6d, 0xab, 0xc7, 0x88, 0x02, 0xc6, 0x8a, 0xf0, 0xbb, 0x14, 0x00, 0x00,
		0x1a, 0xc0, 0x01, 0xa0, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95,
		0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b,
		0x16, 0xf8, 0x17, 0x98, 0xa0, 0x53, 0xd7, 0x7b, 0xa2, 0xc6, 0x5e, 0xc0, 0xe0, 0xd6, 0xdb, 0xb7,
		0x37, 0x59, 0x79, 0x69, 0x93, 0x4c, 0xd6, 0xa3, 0x20, 0x72, 0xa8, 0xd3, 0xca, 0xaa, 0xbd, 0x7c,
		0xb8, 0x4c, 0xe8, 0x47, 0x90
};

TEST(EthereumTransactionBuilderTest, SignAndExportPayloadStructure1) {
	EthereumAddress dst("0x0000C16a54724cf8c73469eA6bdD763bFB6DaBc7");
	EthereumTransactionBuilder builder(dst, 10, 21296);
	builder.gasInfo.maxFeePerGas = 1066263560;
	builder.gasInfo.maxPriorityFeePerGas = 1000000000;
	builder.calldata.add(0x1a);
	builder.value = "0x2c68af0bb140000";

	ArrayList<uint8_t> result = builder.signAndExport(executorKey, 1);
	ASSERT_GE(result.size(), 67);

	// We don't expect the signature to match.
	char* expected = formatBinaryDataForHexdump(rawTxHex1, sizeof(rawTxHex1) - 67, 16);

	int startPos = result.get(0);
	char* actual = formatBinaryDataForHexdump(&result.get(startPos), result.size() - 67 - startPos, 16);

	if (strcmp(expected, actual)) {
		ADD_FAILURE();
		printf("Expected:\n%s\n\nActual:\n%s\n", expected, actual);
	}

	free(expected);
	free(actual);
}

static const uint8_t rawTxHex2[] = {
		0x02, 0xf8, 0x6f, 0x01, 0x0b, 0x83, 0x98, 0x96, 0x80, 0x84, 0x3b, 0x9a, 0xca, 0x00, 0x82, 0x93,
		0xed, 0x94, 0x00, 0x00, 0xc1, 0x6a, 0x54, 0x72, 0x4c, 0xf8, 0xc7, 0x34, 0x69, 0xea, 0x6b, 0xdd,
		0x76, 0x3b, 0xfb, 0x6d, 0xab, 0xc7, 0x80, 0x86, 0x21, 0x00, 0x08, 0xec, 0x63, 0x1a, 0xc0, 0x80,
		0xa0, 0xca, 0x05, 0xdc, 0x9d, 0x55, 0x34, 0x57, 0x3d, 0x73, 0x40, 0x3a, 0xe9, 0x75, 0x41, 0x5d,
		0x1e, 0x51, 0x72, 0x86, 0x03, 0x51, 0x49, 0xa5, 0x76, 0xdb, 0x6a, 0xae, 0x46, 0x05, 0x58, 0xa7,
		0x91, 0xa0, 0x21, 0x86, 0x8e, 0x24, 0xfa, 0xe7, 0xfd, 0x80, 0x76, 0x35, 0x57, 0x5e, 0x2e, 0x3b,
		0xc5, 0xfb, 0xc3, 0x67, 0x2c, 0x88, 0xd3, 0x12, 0x83, 0xbb, 0x56, 0x43, 0xaa, 0x2f, 0xa7, 0xf9,
		0xe6, 0xf8
};

static const uint8_t tx2Calldata[] = {
		0x21, 0x00, 0x08, 0xec, 0x63, 0x1a
};

TEST(EthereumTransactionBuilderTest, SignAndExportPayloadStructure2) {
	EthereumAddress dst("0x0000C16a54724cf8c73469eA6bdD763bFB6DaBc7");
	EthereumTransactionBuilder builder(dst, 11, 37869);
	builder.gasInfo.maxFeePerGas = 1000000000;
	builder.gasInfo.maxPriorityFeePerGas = 10000000;
	builder.calldata.addMany(tx2Calldata, sizeof(tx2Calldata));

	ArrayList<uint8_t> result = builder.signAndExport(executorKey, 2);
	ASSERT_GE(result.size(), 67);

	// We don't expect the signature to match.
	char* expected = formatBinaryDataForHexdump(rawTxHex2, sizeof(rawTxHex2) - 67, 16);

	int startPos = result.get(0);
	char* actual = formatBinaryDataForHexdump(&result.get(startPos), result.size() - 67 - startPos, 16);

	if (strcmp(expected, actual)) {
		ADD_FAILURE();
		printf("Expected:\n%s\n\nActual:\n%s\n", expected, actual);
	}

	free(expected);
	free(actual);
}

TEST(EthereumTransactionBuilderTest, BoolOperatorReturnsFalseForInvalidTransaction) {
	EthereumTransactionBuilder builder(EthereumAddress(), 11, 20999);

	// Invalid gasLimit - 20999 is below the minimum tx gas cost (21000)
	EXPECT_FALSE(builder);
}

TEST(EthereumTransactionBuilderTest, BoolOperatorReturnsTrueForValidTransaction) {
	EthereumAddress dst("0x1234567890abcdef1234567890abcdef12345678");
	EthereumTransactionBuilder builder(dst, 1, 21000);
	EXPECT_TRUE(builder);
}
