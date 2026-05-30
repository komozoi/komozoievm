// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-09
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

#ifndef CMEVBOT_EVMTRACER_H
#define CMEVBOT_EVMTRACER_H

#include "ethereum.h"
#include "interface/ChainProvider.h"
#include "util/ConstArray.h"


typedef struct {
	EthereumAddress src;
	EthereumAddress dst;
	Bytes calldata;
	Bytes returndata;
	uint64_t gasUsed;
	bool success;
} call_trace_t;


typedef struct {
	EthereumAddress src;
	uint256_t topics[4];
	Bytes data;
	uint32_t numTopics;
} event_trace_t;


class EVMTracer {
public:
	// Tracer does nothing by default
	virtual void onContractCall(StateProvider& chain, call_trace_t callTrace) {}
	virtual void onEventLog(StateProvider& chain, event_trace_t eventTrace) {}

	virtual ~EVMTracer() = default;
};

#endif //CMEVBOT_EVMTRACER_H
