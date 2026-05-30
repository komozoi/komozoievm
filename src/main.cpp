// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-10-08
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


#include <csignal>
#include "util/Logger.h"
#include "events/EventBus.h"
#include "services/services.h"
#include "interface/addresses.h"
#include "interface/apikeys.h"
#include "storage/helpers.h"


// Global flag for signal handler
volatile std::sig_atomic_t gSignalStatus = 0;
EventBus* eventBus;
static LogEndpoint* mainLogPtr;

// Sequence handler for SIGINT (Ctrl-C)
void handleSignal(int signal) {
	if (signal == SIGINT) {
		gSignalStatus = signal;
		mainLogPtr->critical("Received interrupt signal, shutting down...");
		eventBus->shutdown();
	}
	if (signal == SIGTERM) {
		// Because CLion is retarded and can't send SIGINT, evidently.
		gSignalStatus = signal;
		mainLogPtr->critical("Received terminate signal, shutting down...");
		eventBus->shutdown();
	}
}


int main() {
	Logger log("logs", LOG_LEVEL_INFO, LOG_LEVEL_DEBUG);
	LogEndpoint mainLog(log, "main");
	mainLogPtr = &mainLog;

	mainLog.debug("Configuring signal handlers");
	std::signal(SIGINT, handleSignal);
	std::signal(SIGTERM, handleSignal);
	std::signal(SIGPIPE, SIG_IGN);

	curl_global_init(CURL_GLOBAL_DEFAULT);

	mainLog.info("Setting up chain access...");
	std::string rootPath = "data/";
	recursiveMkdir(rootPath.c_str(), 0755);
	Web3Cpp gethRpc("http://" GETH_NODE_IP ":8545");
	Web3Cpp infuraRpc("https://mainnet.infura.io/v3/" INFURA_KEY);
	ChainAccess chain(log, gethRpc, infuraRpc, rootPath);
	if (chain.connect("192.168.1.221", 8545) != 0) {
		mainLog.critical("Failed to connect to node (error: %s), aborting...", strerror(errno));
		return -1;
	}

	mainLog.debug("Starting services...");
	eventBus = new EventBus(chain, log);
	eventBus->start();

	startAllServices(log, *eventBus, chain);
	mainLog.debug("Services started.");

	eventBus->join();

	mainLog.info("Event loop has stopped, exiting...");

	delete eventBus;

	return 0;
}
