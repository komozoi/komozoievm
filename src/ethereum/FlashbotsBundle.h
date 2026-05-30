// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-18
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

#ifndef CMEVBOT_FLASHBOTSBUNDLE_H
#define CMEVBOT_FLASHBOTSBUNDLE_H

#include "ethereum.h"
#include "EthereumTransaction.h"


struct flashbots_bundle_t {
	ArrayList<ArrayList<uint8_t>> txs;
	ArrayList<EthereumTxHash> reverting;
	ArrayList<EthereumTxHash> droppable;
	uint64_t blockNumber;
};

#endif //CMEVBOT_FLASHBOTSBUNDLE_H
