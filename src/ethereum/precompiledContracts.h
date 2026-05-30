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

#ifndef CMEVBOT_PRECOMPILEDCONTRACTS_H
#define CMEVBOT_PRECOMPILEDCONTRACTS_H

#include "stdint.h"


// On success, returns null
// On failure, returns a C string with the reason.
const char* callPrecompiled(int index, const uint8_t* argsMem, int argsSize, uint8_t* retMem, int retSize, int& gas);


#endif //CMEVBOT_PRECOMPILEDCONTRACTS_H
