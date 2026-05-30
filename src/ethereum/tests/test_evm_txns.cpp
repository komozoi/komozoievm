// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-22
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
// Usually it's good to disable these tests.
//#define TEST_TXNS

#ifdef TEST_TXNS

#include <gtest/gtest.h>
#include "bigint.h"
#include "ethereum/EVMSimulationContext.h"
#include "ethereum/EVM.h"
#include "interface/Web3Cpp.h"
#include "interface/apikeys.h"


typedef struct {
	EVMSimulationContext* context;
	EVMSimulationOutput output;
} tmp_sim_output_t;


//Web3Cpp web3("http://192.168.1.221:8545");
Web3Cpp web3("https://mainnet.infura.io/v3/" INFURA_KEY);


tmp_sim_output_t simulate(const char* txHash, uint64_t blockNumber) {
	EthereumTxHash txHashKey(txHash);
	EthereumTransaction transaction = web3.getTransactionDetails(txHashKey);
	if (!transaction.isValid()) {
		ADD_FAILURE();
		printf("Unable to get transaction %s\n", txHash);
		return {nullptr, EVMSimulationOutput(false, Bytes(), "Unable to fetch transaction")};
	}

	block_info_t blockInfo = web3.getBlockInfoByNumber(blockNumber);

	EVM processor(web3);

	sim_tx_info_t txInfo{
			transaction.gasInfo(),
			blockInfo,
			blockNumber - 1
	};
	txInfo.gas.gasLimit *= 2;
	tmp_sim_output_t out{
			new EVMSimulationContext(web3, txInfo, transaction),
			processor.simulate(*(out.context))
	};

	if (out.output.success) {
		printf("SUCCEEDED (%lu gas used)\n\n", out.context->gasUsed);
	} else {
		char* prettyReturnData = formatBinaryDataForHexdump(out.output.returnData.data(), out.output.returnData.size());
		printf("REVERTED %s: %s\n\n", out.output.reason, prettyReturnData);
		free(prettyReturnData);
	}

	return out;
}

TEST(EVMTestTransactions, 0xe21fa3cdf08bc93de2a7b69872ea8d351fd5dd2b942591a2d1361967b4a8aee4) {
	tmp_sim_output_t out = simulate("0xe21fa3cdf08bc93de2a7b69872ea8d351fd5dd2b942591a2d1361967b4a8aee4", 23109176);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 321401);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0xf02d8d32885b80d256f1d6dd6780d77de06e4e30ea0ecfeb0513b9948e48e7c7) {
	tmp_sim_output_t out = simulate("0xf02d8d32885b80d256f1d6dd6780d77de06e4e30ea0ecfeb0513b9948e48e7c7", 23047391);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 204788);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x0b5d8799583d5691239149641C1db41746Ad612bb9158e83835F62434e0554d5) {
	tmp_sim_output_t out = simulate("0x0b5d8799583d5691239149641C1db41746Ad612bb9158e83835F62434e0554d5", 23047117);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 100410);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0xeFC19A35A63777d47413FF7e4AF8F01e739FA09ed62d31A3AF31e03e53Ce2C77) {
	tmp_sim_output_t out = simulate("0xeFC19A35A63777d47413FF7e4AF8F01e739FA09ed62d31A3AF31e03e53Ce2C77", 23042470);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 32650);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x346d54A12d65F5d69C212AF5e5351FF4AC9bC00702AA23F3A7bbCb679d5d9b0e) {
	tmp_sim_output_t out = simulate("0x346d54A12d65F5d69C212AF5e5351FF4AC9bC00702AA23F3A7bbCb679d5d9b0e", 23042469);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 22058);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


// Fails because the access list changes the gas usage, thus causing
// the dynamic jump to be wrong.
TEST(EVMTestTransactions, 0xdAdF3376eF1688C37725CA747CFA4de71e8482CFd9203dCb30e08d3A4F18d3CC) {
	tmp_sim_output_t out = simulate("0xdAdF3376eF1688C37725CA747CFA4de71e8482CFd9203dCb30e08d3A4F18d3CC", 23041632);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 94415);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0xfb516a9539d9e99cbe8bcec7369e0e7b6631d2248004324af01fcc607aa3304f) {
	tmp_sim_output_t out = simulate("0xfb516a9539d9e99cbe8bcec7369e0e7b6631d2248004324af01fcc607aa3304f", 22950743);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 37504);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0x8Fb31e1d27040db839FC32b74A116A84C86A60d106F16Cb7ddFA54CbF0Ce5848) {
	tmp_sim_output_t out = simulate("0x8Fb31e1d27040db839FC32b74A116A84C86A60d106F16Cb7ddFA54CbF0Ce5848", 23008768);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 155967);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x38a39b9d5d40a9fe705a2e7a4644aca7b9b7e778d1b5f962a2492bd50853c88b) {
	tmp_sim_output_t out = simulate("0x38a39b9d5d40a9fe705a2e7a4644aca7b9b7e778d1b5f962a2492bd50853c88b", 22975856);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 225731);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0xCbd6337970bdCC341A79b29152d5153A289117AddF9dbCF1Ab4A46Ad67e52b21) {
	tmp_sim_output_t out = simulate("0xCbd6337970bdCC341A79b29152d5153A289117AddF9dbCF1Ab4A46Ad67e52b21", 23003774);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 841695);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0xCF6d765C4835CdA31428F0AA2eCC56e933A4ed19C07ee9Ce9FA05079eA5546AC) {
	tmp_sim_output_t out = simulate("0xCF6d765C4835CdA31428F0AA2eCC56e933A4ed19C07ee9Ce9FA05079eA5546AC", 23003694);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 46109);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0x66C7F656d7eF7b7C0ACFF3C2A96059C9bde94862234b78FC3A72A2FA60d6061C) {
	tmp_sim_output_t out = simulate("0x66C7F656d7eF7b7C0ACFF3C2A96059C9bde94862234b78FC3A72A2FA60d6061C", 23003694);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 74470);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0xC594729bb8e7d0A0F3Ab6513b49A869d8b512180165A3e93909C42441593d92A) {
	tmp_sim_output_t out = simulate("0xC594729bb8e7d0A0F3Ab6513b49A869d8b512180165A3e93909C42441593d92A", 23003694);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 118815);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0x1AF17AAe5878526C33F2F7bd2b1507dAF5AF377e941d33b7d62886de84F12d2A) {
	tmp_sim_output_t out = simulate("0x1AF17AAe5878526C33F2F7bd2b1507dAF5AF377e941d33b7d62886de84F12d2A", 22977626);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 360188);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0xF6d165A3Fe2C1C507A59246C45092310bAbed9304d1A5A370bFe3d6C20bC355F) {
	tmp_sim_output_t out = simulate("0xF6d165A3Fe2C1C507A59246C45092310bAbed9304d1A5A370bFe3d6C20bC355F", 22977626);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 154045);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

// Working or mostly working

/*TEST(EVMTestTransactions, 0xbd2111A4222d7FdeFAd74005bd8A829336644F8F077A82dCFC30A3b4CF1b88e3) {
	tmp_sim_output_t out = simulate("0xbd2111A4222d7FdeFAd74005bd8A829336644F8F077A82dCFC30A3b4CF1b88e3", 23009180);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 103370);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x28ab474872119c0db36b9124a5b1ddc66acfe0c8d0e5bf5b2ef1262f7f9358fd) {
	tmp_sim_output_t out = simulate("0x28ab474872119c0db36b9124a5b1ddc66acfe0c8d0e5bf5b2ef1262f7f9358fd", 23003587);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 22111);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}


TEST(EVMTestTransactions, 0x7ebb0F07C1669791351C8203C97C19512bdbdd46335bAC0d0C2db5dCFdAC0dF1) {
	tmp_sim_output_t out = simulate("0x7ebb0F07C1669791351C8203C97C19512bdbdd46335bAC0d0C2db5dCFdAC0dF1", 23003694);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 30549);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x198537d91e27e79498ed3bCeFF47A971b8F52F4A87b9eA851066309918662dFb) {
	tmp_sim_output_t out = simulate("0x198537d91e27e79498ed3bCeFF47A971b8F52F4A87b9eA851066309918662dFb", 23047056);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 22111);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}

TEST(EVMTestTransactions, 0x299C89Fb100AA680b4CCC5eAFeFeb5A317F88dA4C86603981FeF82AC63e094d3) {
	tmp_sim_output_t out = simulate("0x299C89Fb100AA680b4CCC5eAFeFeb5A317F88dA4C86603981FeF82AC63e094d3", 23042466);

	if (out.context) {
		EXPECT_EQ(out.context->gasUsed, 22111);
		EXPECT_TRUE(out.output.success) << out.output.reason;
		delete out.context;
	}
}*/


#endif
