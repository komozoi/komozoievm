// Copyright 2025-2026 komozoi
// Original Creation Date: 2026-05-29
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
// Python bindings entry point for the Komozoi EVM simulator.  The module
// exposes the C++ classes needed to assemble a chain state, build Ethereum
// transactions, and simulate their execution.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <string>
#include <vector>

#include "bigint.h"
#include "ds/HashMap.h"
#include "ds/ArrayList.h"

#include "ethereum/ethereum.h"
#include "ethereum/EthereumTransaction.h"
#include "ethereum/EthereumTransactionBuilder.h"
#include "ethereum/EVM.h"
#include "ethereum/EVMSimulationContext.h"
#include "interface/ChainProvider.h"
#include "util/Bytes.h"

namespace py = pybind11;

namespace {

// Convert a Python-side 20-byte address (bytes or hex string) into the native
// EthereumAddress type.  Hex strings may optionally carry the "0x" prefix.
EthereumAddress addressFromPy(const py::object& obj) {
	if (py::isinstance<py::str>(obj)) {
		std::string s = obj.cast<std::string>();
		return EthereumAddress(s.c_str());
	}
	py::buffer buf = obj.cast<py::buffer>();
	py::buffer_info info = buf.request();
	if (info.size != 20)
		throw py::value_error("Address must be 20 bytes or a hex string");
	return EthereumAddress(static_cast<const uint8_t*>(info.ptr), 20, true);
}

// Render an EthereumAddress as a 0x-prefixed lowercase hex string.
std::string addressToHex(const EthereumAddress& addr) {
	char buffer[64];
	addr.toStr(buffer, true, true);
	return std::string(buffer);
}

// Convert an EthereumAddress to a 20-byte big-endian Python bytes object.
py::bytes addressToBytes(const EthereumAddress& addr) {
	uint8_t buffer[20];
	addr.toBytes(buffer, true);
	return py::bytes(reinterpret_cast<const char*>(buffer), 20);
}

// Convert a Python int (arbitrary precision, but must fit in 256 bits) into a
// uint256_t.  Negative values are rejected.
uint256_t u256FromPy(const py::object& obj) {
	if (py::isinstance<py::int_>(obj)) {
		// Serialize through the Python C API to get raw little-endian bytes.
		py::int_ pyInt = obj.cast<py::int_>();
		PyObject* asLong = pyInt.ptr();
		uint8_t buffer[32] = {0};
		int rc = _PyLong_AsByteArray(reinterpret_cast<PyLongObject*>(asLong),
			buffer, sizeof(buffer), 1, 0);
		if (rc != 0)
			throw py::value_error("integer does not fit in uint256");
		return uint256_t(buffer, 32, false);
	}
	if (py::isinstance<py::str>(obj))
		return uint256_t(obj.cast<std::string>().c_str());
	throw py::type_error("expected int or hex string for uint256");
}

// Convert a uint256_t back to a Python int.
py::int_ u256ToPy(const uint256_t& value) {
	uint8_t buffer[32];
	value.toBytes(buffer, false);
	PyObject* result = _PyLong_FromByteArray(buffer, 32, 1, 0);
	if (!result)
		throw py::error_already_set();
	return py::reinterpret_steal<py::int_>(result);
}

// Convert a libexcessive Bytes value into a Python bytes object.
py::bytes bytesToPy(const Bytes& b) {
	const size_t length = b.size();
	std::string scratch;
	scratch.resize(length);
	for (size_t i = 0; i < length; ++i)
		scratch[i] = static_cast<char>(b.get(static_cast<unsigned int>(i)));
	return py::bytes(scratch.data(), length);
}

// Copy a Python buffer-protocol value into a libexcessive Bytes.
Bytes bytesFromPy(const py::object& obj) {
	if (obj.is_none())
		return Bytes();
	py::buffer buf = obj.cast<py::buffer>();
	py::buffer_info info = buf.request();
	return Bytes(info.ptr, static_cast<size_t>(info.size));
}

// In-memory StateProvider backed by libexcessive hash maps.  Used as the
// default chain in Python tests and as a simple persistent backing store.
class MockChain : public StateProvider {
public:
	MockChain() : accounts(16), code(16), storage(16) {}

	EthereumAccountInfo getAccountInfo(const EthereumAddress& address, uint64_t /*blockNumber*/ = 0) override {
		EthereumAccountInfo* found = accounts.getPtr(address);
		if (found)
			return *found;
		return EthereumAccountInfo();
	}

	Bytes getContractCode(const EthereumAddress& address) override {
		Bytes* found = code.getPtr(address);
		return found ? *found : Bytes();
	}

	ArrayList<uint256_t> getStorageSlots(const EthereumAddress& address,
			const ArrayList<LongKey<256>>& slotKeys, uint64_t /*blockNumber*/ = 0) override {
		ArrayList<uint256_t> result;
		HashMap<LongKey<256>, uint256_t>** existing = storage.getPtr(address);
		HashMap<LongKey<256>, uint256_t>* slots = existing ? *existing : nullptr;
		for (int i = 0; i < slotKeys.size(); ++i) {
			uint256_t value;
			if (slots) {
				uint256_t* found = slots->getPtr(slotKeys.get(i));
				if (found)
					value = *found;
			}
			result.add(value);
		}
		return result;
	}

	bool updateAccount(const EthereumAddress& key, const EthereumAccountInfo& account) override {
		accounts.put(key, account);
		return true;
	}

	bool saveContractCode(const EthereumAddress& key, Bytes runcode) override {
		code.put(key, runcode);
		return true;
	}

	bool updateStorageSlots(const EthereumAddress& key,
			const HashMap<LongKey<256>, uint256_t>& entries) override {
		HashMap<LongKey<256>, uint256_t>** existing = storage.getPtr(key);
		HashMap<LongKey<256>, uint256_t>* slots = existing ? *existing : nullptr;
		if (!slots) {
			slots = new HashMap<LongKey<256>, uint256_t>(16);
			storage.put(key, slots);
		}
		for (MapElement<LongKey<256>, uint256_t> e : entries)
			slots->put(e.key, e.value);
		return true;
	}

	// Pythonic conveniences used by tests and quick scripts.
	void setBalance(const EthereumAddress& addr, const uint256_t& balance) {
		EthereumAccountInfo info(addr, balance, 0);
		accounts.put(addr, info);
	}

	void setCode(const EthereumAddress& addr, Bytes runcode) {
		code.put(addr, runcode);
	}

	void setStorageSlot(const EthereumAddress& addr, const LongKey<256>& slot, const uint256_t& value) {
		HashMap<LongKey<256>, uint256_t>** existing = storage.getPtr(addr);
		HashMap<LongKey<256>, uint256_t>* slots = existing ? *existing : nullptr;
		if (!slots) {
			slots = new HashMap<LongKey<256>, uint256_t>(16);
			storage.put(addr, slots);
		}
		slots->put(slot, value);
	}

private:
	HashMap<EthereumAddress, EthereumAccountInfo> accounts;
	HashMap<EthereumAddress, Bytes> code;
	HashMap<EthereumAddress, HashMap<LongKey<256>, uint256_t>*> storage;
};

// Trampoline that lets Python subclasses implement StateProvider.
class PyStateProvider : public StateProvider {
public:
	using StateProvider::StateProvider;

	EthereumAccountInfo getAccountInfo(const EthereumAddress& address, uint64_t blockNumber = 0) override {
		PYBIND11_OVERRIDE_PURE_NAME(EthereumAccountInfo, StateProvider,
			"get_account_info", getAccountInfo, address, blockNumber);
	}

	Bytes getContractCode(const EthereumAddress& address) override {
		PYBIND11_OVERRIDE_PURE_NAME(Bytes, StateProvider,
			"get_contract_code", getContractCode, address);
	}

	ArrayList<uint256_t> getStorageSlots(const EthereumAddress& address,
			const ArrayList<LongKey<256>>& slotKeys, uint64_t blockNumber = 0) override {
		PYBIND11_OVERRIDE_PURE_NAME(ArrayList<uint256_t>, StateProvider,
			"get_storage_slots", getStorageSlots, address, slotKeys, blockNumber);
	}

	bool updateAccount(const EthereumAddress& key, const EthereumAccountInfo& account) override {
		PYBIND11_OVERRIDE_NAME(bool, StateProvider,
			"update_account", updateAccount, key, account);
	}

	bool saveContractCode(const EthereumAddress& key, Bytes runcode) override {
		PYBIND11_OVERRIDE_NAME(bool, StateProvider,
			"save_contract_code", saveContractCode, key, runcode);
	}

	bool updateStorageSlots(const EthereumAddress& key,
			const HashMap<LongKey<256>, uint256_t>& entries) override {
		PYBIND11_OVERRIDE_NAME(bool, StateProvider,
			"update_storage_slots", updateStorageSlots, key, entries);
	}
};

// Result object returned by EVM.simulate / EVM.execute.
struct SimulationResult {
	bool success;
	py::bytes returnData;
	std::string reason;
	uint64_t gasUsed;
};

SimulationResult runSimulate(EVM& evm, const EthereumTransaction& tx, const block_info_t& block) {
	EVMSimulationOutput out(false, nullptr, 0, nullptr);
	{
		py::gil_scoped_release release;
		out = evm.simulate(tx, block, nullptr);
	}
	SimulationResult result;
	result.success = out.success;
	result.returnData = py::bytes(reinterpret_cast<const char*>(out.returnDataPtr), out.returnDataSize);
	result.reason = out.reason ? std::string(out.reason) : std::string();
	result.gasUsed = 0;
	return result;
}

SimulationResult runExecute(EVM& evm, const EthereumTransaction& tx, const block_info_t& block) {
	evm_execution_outcome_t out;
	{
		py::gil_scoped_release release;
		out = evm.execute(tx, block);
	}
	SimulationResult result;
	result.success = out.succeeded;
	result.returnData = py::bytes();
	result.reason = out.message ? std::string(out.message) : std::string();
	result.gasUsed = out.gasUsed;
	return result;
}

} // namespace


PYBIND11_MODULE(_komozoievm, m) {
	m.doc() = "Komozoi EVM Python bindings";
	m.attr("__version__") = "0.1.0";

	// Address: a thin wrapper over EthereumAddress with Pythonic conversions.
	py::class_<EthereumAddress>(m, "Address")
		.def(py::init([](const py::object& src) { return addressFromPy(src); }), py::arg("value"))
		.def("to_bytes", &addressToBytes)
		.def("to_hex", &addressToHex)
		.def("__bytes__", &addressToBytes)
		.def("__str__", &addressToHex)
		.def("__repr__", [](const EthereumAddress& a) {
			return std::string("Address('") + addressToHex(a) + "')";
		})
		.def("__eq__", [](const EthereumAddress& a, const EthereumAddress& b) { return a == b; })
		.def("__hash__", [](const EthereumAddress& a) { return static_cast<uint64_t>(a); });
	py::implicitly_convertible<py::str, EthereumAddress>();
	py::implicitly_convertible<py::bytes, EthereumAddress>();

	// U256 is exposed mainly as a helper namespace for explicit conversions;
	// most APIs accept and return plain Python ints.
	py::class_<uint256_t>(m, "U256")
		.def(py::init([](const py::object& v) { return u256FromPy(v); }), py::arg("value") = py::int_(0))
		.def("to_int", &u256ToPy)
		.def("__int__", &u256ToPy)
		.def("__repr__", [](const uint256_t& v) {
			return std::string("U256(") + std::string(py::str(u256ToPy(v))) + ")";
		});
	py::implicitly_convertible<py::int_, uint256_t>();

	// Bytes is the libexcessive container; we expose it primarily so that
	// Python code can pass raw `bytes` values to and from the engine.
	py::class_<Bytes>(m, "Bytes")
		.def(py::init([](const py::object& src) { return bytesFromPy(src); }), py::arg("value") = py::bytes())
		.def("__bytes__", &bytesToPy)
		.def("__len__", [](const Bytes& b) { return b.size(); });
	py::implicitly_convertible<py::bytes, Bytes>();
	py::implicitly_convertible<py::bytearray, Bytes>();

	// Block context for simulation.
	py::class_<block_info_t>(m, "BlockInfo")
		.def(py::init([](uint64_t number, uint64_t timestamp, uint64_t baseFee, uint64_t gasLimit,
				uint32_t chainId, py::object coinbase, py::object randao) {
			block_info_t info{};
			info.number = number;
			info.timestamp = timestamp;
			info.baseFee = baseFee;
			info.gasLimit = gasLimit;
			info.chainId = chainId;
			info.coinbase = coinbase.is_none() ? EthereumAddress("0x0") : addressFromPy(coinbase);
			info.randao = randao.is_none() ? uint256_t() : u256FromPy(randao);
			return info;
		}),
			py::arg("number") = 0,
			py::arg("timestamp") = 0,
			py::arg("base_fee") = 0,
			py::arg("gas_limit") = 30000000,
			py::arg("chain_id") = 1,
			py::arg("coinbase") = py::none(),
			py::arg("randao") = py::none())
		.def_readwrite("number", &block_info_t::number)
		.def_readwrite("timestamp", &block_info_t::timestamp)
		.def_readwrite("base_fee", &block_info_t::baseFee)
		.def_readwrite("gas_limit", &block_info_t::gasLimit)
		.def_readwrite("chain_id", &block_info_t::chainId)
		.def_readwrite("coinbase", &block_info_t::coinbase)
		.def_property("randao",
			[](const block_info_t& b) { return u256ToPy(b.randao); },
			[](block_info_t& b, const py::object& v) { b.randao = u256FromPy(v); });

	py::class_<EthereumAccountInfo>(m, "AccountInfo")
		.def(py::init([](const py::object& address, const py::object& balance, uint64_t nextNonce) {
			return EthereumAccountInfo(addressFromPy(address), u256FromPy(balance), nextNonce);
		}),
			py::arg("address"),
			py::arg("balance") = py::int_(0),
			py::arg("next_nonce") = 0)
		.def_property_readonly("address", [](const EthereumAccountInfo& a) { return a.address; })
		.def_property("balance",
			[](const EthereumAccountInfo& a) { return u256ToPy(a.balance); },
			[](EthereumAccountInfo& a, const py::object& v) { a.balance = u256FromPy(v); })
		.def_readwrite("next_nonce", &EthereumAccountInfo::nextNonce);

	py::class_<EthereumAccessListEntry>(m, "AccessListEntry")
		.def(py::init([](const py::object& address, const std::vector<py::object>& slots) {
			EthereumAccessListEntry entry(addressFromPy(address));
			for (const py::object& slot : slots) {
				uint256_t key = u256FromPy(slot);
				entry.storageKeys.add(*reinterpret_cast<LongKey<256>*>(&key));
			}
			return entry;
		}),
			py::arg("address"),
			py::arg("storage_keys") = std::vector<py::object>());

	py::class_<EthereumTransaction>(m, "Transaction")
		// Construct an unsigned transaction suitable for simulation only.  The
		// signature is zeroed, the sender is taken directly from the caller,
		// and the hash is left at zero since `EVM.simulate` doesn't inspect it.
		.def_static("call", [](const py::object& from, const py::object& to, const py::object& data,
				uint64_t nonce, uint32_t gasLimit, uint64_t value, uint64_t maxFeePerGas,
				uint64_t maxPriorityFeePerGas) {
			ArrayList<uint8_t> input;
			if (!data.is_none()) {
				py::buffer buf = data.cast<py::buffer>();
				py::buffer_info info = buf.request();
				const uint8_t* bytes = static_cast<const uint8_t*>(info.ptr);
				for (ssize_t i = 0; i < info.size; ++i)
					input.add(bytes[i]);
			}
			tx_signature_t sig{};
			tx_gas_info_t gas{gasLimit, 0, maxFeePerGas, maxPriorityFeePerGas};
			EthereumTxHash hash;
			uint8_t zero[32] = {0};
			hash = EthereumTxHash(zero, 32);
			return EthereumTransaction(hash, addressFromPy(from), addressFromPy(to),
				sig, gas, input, nonce, TRANSACTION_TYPE_EIP1559, value);
		},
			py::arg("from_"),
			py::arg("to"),
			py::arg("data") = py::none(),
			py::arg("nonce") = 0,
			py::arg("gas_limit") = 1000000,
			py::arg("value") = 0,
			py::arg("max_fee_per_gas") = 0,
			py::arg("max_priority_fee_per_gas") = 0)
		.def_property_readonly("hash", [](const EthereumTransaction& t) {
			char buffer[80];
			t.hash().toStr(buffer, true, true);
			return std::string(buffer);
		})
		.def_property_readonly("sender", [](const EthereumTransaction& t) { return t.sender(); })
		.def_property_readonly("recipient", [](const EthereumTransaction& t) { return t.recipient(); })
		.def_property_readonly("nonce", &EthereumTransaction::nonce)
		.def_property_readonly("gas_limit", &EthereumTransaction::gasLimit)
		.def_property_readonly("value", [](const EthereumTransaction& t) { return u256ToPy(t.value()); })
		.def_property_readonly("calldata", [](const EthereumTransaction& t) { return bytesToPy(t.calldata()); });

	py::class_<EthereumTransactionBuilder>(m, "TransactionBuilder")
		.def(py::init([](const py::object& to, uint64_t nonce, uint32_t gasLimit, const py::object& value,
				uint64_t gasPrice, uint64_t maxFeePerGas, uint64_t maxPriorityFeePerGas,
				const py::object& data) {
			EthereumTransactionBuilder b(addressFromPy(to), nonce, gasLimit);
			b.value = u256FromPy(value);
			b.gasInfo.gasPrice = gasPrice;
			b.gasInfo.maxFeePerGas = maxFeePerGas;
			b.gasInfo.maxPriorityFeePerGas = maxPriorityFeePerGas;
			if (!data.is_none()) {
				py::buffer buf = data.cast<py::buffer>();
				py::buffer_info info = buf.request();
				const uint8_t* bytes = static_cast<const uint8_t*>(info.ptr);
				for (ssize_t i = 0; i < info.size; ++i)
					b.calldata.add(bytes[i]);
			}
			return b;
		}),
			py::arg("to"),
			py::arg("nonce") = 0,
			py::arg("gas_limit") = 21000,
			py::arg("value") = py::int_(0),
			py::arg("gas_price") = 0,
			py::arg("max_fee_per_gas") = 0,
			py::arg("max_priority_fee_per_gas") = 0,
			py::arg("data") = py::none())
		.def_readwrite("dst", &EthereumTransactionBuilder::dst)
		.def_readwrite("nonce", &EthereumTransactionBuilder::nonce);

	py::class_<SimulationResult>(m, "SimulationResult")
		.def_readonly("success", &SimulationResult::success)
		.def_readonly("return_data", &SimulationResult::returnData)
		.def_readonly("reason", &SimulationResult::reason)
		.def_readonly("gas_used", &SimulationResult::gasUsed);

	py::class_<StateProvider, PyStateProvider>(m, "StateProvider")
		.def(py::init<>())
		.def("get_account_info",
			static_cast<EthereumAccountInfo (StateProvider::*)(const EthereumAddress&, uint64_t)>(&StateProvider::getAccountInfo),
			py::arg("address"), py::arg("block_number") = 0)
		.def("get_contract_code",
			static_cast<Bytes (StateProvider::*)(const EthereumAddress&)>(&StateProvider::getContractCode),
			py::arg("address"))
		.def("update_account", &StateProvider::updateAccount, py::arg("address"), py::arg("info"))
		.def("save_contract_code", &StateProvider::saveContractCode, py::arg("address"), py::arg("code"));

	py::class_<MockChain, StateProvider>(m, "MockChain")
		.def(py::init<>())
		.def("set_balance", [](MockChain& self, const py::object& addr, const py::object& balance) {
			self.setBalance(addressFromPy(addr), u256FromPy(balance));
		}, py::arg("address"), py::arg("balance"))
		.def("set_code", [](MockChain& self, const py::object& addr, const py::object& code) {
			self.setCode(addressFromPy(addr), bytesFromPy(code));
		}, py::arg("address"), py::arg("code"))
		.def("set_storage", [](MockChain& self, const py::object& addr, const py::object& slot, const py::object& value) {
			uint256_t slotKey = u256FromPy(slot);
			self.setStorageSlot(addressFromPy(addr),
				*reinterpret_cast<LongKey<256>*>(&slotKey), u256FromPy(value));
		}, py::arg("address"), py::arg("slot"), py::arg("value"));

	py::class_<EVM>(m, "EVM")
		.def(py::init<StateProvider&>(), py::arg("chain"), py::keep_alive<1, 2>())
		.def("simulate", &runSimulate, py::arg("transaction"), py::arg("block"))
		.def("execute", &runExecute, py::arg("transaction"), py::arg("block"))
		.def_static("initial_gas_cost", [](const py::object& data) {
			py::buffer buf = data.cast<py::buffer>();
			py::buffer_info info = buf.request();
			return EVM::getInitialGasCost(Bytes(info.ptr, static_cast<size_t>(info.size)));
		}, py::arg("calldata"));
}


