#ifndef QTUM_POA_H
#define QTUM_POA_H

#include <map>
#include "dbwrapper.h"
#include "wallet/wallet.h"

namespace Poa {
const size_t BLOCK_MINER_CACHE_SIZE = 1 << 10;  // 1kB
const size_t NEXT_BLOCK_MINER_LIST_CACHE_SIZE = 3 << 10;  // 3kB

void ThreadPoaMiner();

class BasicPoa {
private:
	std::vector<CKeyID> _miner_list;
	CKeyID _miner;
	uint32_t _period;
	uint32_t _timeout;

	CKey _miner_key;
	CScript _reward_script;

	CDBWrapper _block_miner_cache;
	CDBWrapper _next_block_miner_list_cache;

	// singleton pattern, lazy initialization
	static BasicPoa* _instance;
	BasicPoa():
		_period(0), _timeout(0),
		_block_miner_cache(
				GetDataDir() / "poa_block_miner_cache",
				BLOCK_MINER_CACHE_SIZE,
				true),
		_next_block_miner_list_cache(
				GetDataDir() / "poa_next_block_miner_list_cache",
				NEXT_BLOCK_MINER_LIST_CACHE_SIZE,
				true) {}

public:
	static BasicPoa* getInstance() {
		if (_instance == nullptr) {
			_instance = new BasicPoa();
		}
		assert(_instance != nullptr);
		return _instance;
	}
	static void delInstance() {
		if (_instance != nullptr) {
			delete _instance;
			_instance = nullptr;
		}
	}

	bool initParams();
	bool hasMiner() {
		return !_miner.IsNull();
	}
	bool initMinerKey();
	bool getRewardScript(CScript& script) {
		if (_reward_script.IsPayToPubkeyHash()) {
			script = _reward_script;
			return true;
		}
		return false;
	}
	CScript getRewardScript() {
		return _reward_script;
	}
	bool minerCanProduceNextBlock(
			const CBlockIndex* p_current_index,
			uint32_t& next_block_time);

};

}  // namespace Poa

#endif /* QTUM_POA_H */
