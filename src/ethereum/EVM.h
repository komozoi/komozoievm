// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-14
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

#ifndef CMEVBOT_EVM_H
#define CMEVBOT_EVM_H


#include "EthereumTransaction.h"
#include "EVMSimulationContext.h"


struct evm_execution_outcome_t {
	uint64_t gasUsed;
	bool succeeded;
	const char* message;
	Bytes returnData;
};


class EVMSimulationOutput {
public:
	EVMSimulationOutput(bool success, Bytes returnData, const char* reason)
		: success(success), returnData(std::move(returnData)), reason(reason) {}

	bool success;
	Bytes returnData;
	const char* reason;
};


class EVM {
public:
	EVM(StateProvider& chain) : chain(chain) {

	}

	// Simulates a single transaction
	EVMSimulationOutput simulate(const EthereumTransaction& transaction, const block_info_t& blockInfo);
	void simulateMany(const EthereumTransaction& txTemplate, const block_info_t& blockInfo, ArrayList<char*> inputVariants, EVMSimulationOutput* outputs);

	EVMSimulationOutput simulate(EVMSimulationContext& context);
	EVMSimulationOutput simulate(const EthereumTransaction& transaction, const block_info_t& blockInfo, EVMTracer* tracer = nullptr);

	evm_execution_outcome_t execute(const EthereumTransaction& transaction, const block_info_t& blockInfo);

	static uint32_t getInitialGasCost(Bytes calldata);
	static uint32_t getInitialGasCost(const ArrayList<uint8_t>& calldata);

private:
	StateProvider& chain;
};


#endif //CMEVBOT_EVM_H
