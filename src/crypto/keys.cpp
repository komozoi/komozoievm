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

#include "Secp256k1.h"

// I don't mind putting this in my repo - executor accounts are essentially disposable.
// Dangerous contract actions are not permitted to these accounts
// There are various mitigations to both prevent private key leakage, and to prevent issues
// in case of leakage.
Secp256k1PrivateKey executorKey("fd43cd7a03ab2d45e11185dde76f2e4871a94d36fe822eed7894531cd6883664");
