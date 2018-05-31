#ifndef QTUM_POA_H
#define QTUM_POA_H

#include <vector>
#include <set>
#include "miner.h"
#include "dbwrapper.h"
#include "chainparams.h"
#include "wallet/wallet.h"

namespace Poa {
const size_t BLOCK_MINER_CACHE_SIZE = 1 << 10;  // 1kB
const size_t NEXT_BLOCK_MINER_LIST_CACHE_SIZE = 3 << 10;  // 3kB

bool isPoaChain();
std::string const& genesisInfo();
void ThreadPoaMiner();

class BasicPoa {
private:
	std::vector<CKeyID> _miner_list;
	CKeyID _miner;
	uint32_t _interval;
	uint32_t _timeout;

	std::set<CKeyID> _miner_set;  // for the calculation of the next block miner
	CScript _reward_script;  // a p2pkh script to the miner

	CKey _miner_key;
	BlockAssembler _block_assembler;

	CDBWrapper _block_miner_cache;
	CDBWrapper _next_block_miner_list_cache;
	bool readBlockMinerFromCache(const uint256& hash, CKeyID& keyid) {
		return _block_miner_cache.Read(hash, keyid);
	}
	bool writeBlockMinerToCache(const uint256& hash, const CKeyID& keyid) {
		return _block_miner_cache.Write(hash, keyid, true);
	}
	bool readNextBlockMinerListFromCache(
			const uint256& hash,
			std::vector<CKeyID>& next_block_miner_list) {
		return _next_block_miner_list_cache.Read(hash, next_block_miner_list);
	}
	bool writeNextBlockMinerListToCache(
			const uint256& hash,
			const std::vector<CKeyID>& next_block_miner_list) {
		return _next_block_miner_list_cache.Write(hash, next_block_miner_list, true);
	}


	// singleton pattern, lazy initialization
	static BasicPoa* _instance;
	BasicPoa():
		_interval(0), _timeout(0),
		_block_assembler(Params()),
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

	bool initParams();  // parse params from cmd line
	bool hasMiner() {  // determine if the miner is specified
		return !_miner.IsNull();
	}
	bool initMinerKey();  // try to get the miner's key from wallet
	bool getRewardScript(CScript& script) {
		if (_reward_script.IsPayToPubkeyHash()) {
			script = _reward_script;
			return true;
		}
		return false;
	}

	// determine if the miner can mine the next block
	// if true then return the next_block_time
	bool canMineNextBlock(
			const CKeyID& miner,
			const CBlockIndex* p_current_index,
			uint32_t& next_block_time);  // for validation
	bool canMineNextBlock(
			const CBlockIndex* p_current_index,
			uint32_t& next_block_time);  // for mining

	bool createNextBlock(
			uint32_t next_block_time,
			std::shared_ptr<CBlock>& pblock);
	bool checkBlock(const CBlockHeader& block);

	// determine the miners who can mine the next block
	// first get the miner set, then get their order and use cache
	bool getNextBlockMinerSet(
			const CBlockIndex* p_current_index,
			std::set<CKeyID>& next_block_miner_set);
	bool getNextBlockMinerList(
			const CBlockIndex* p_current_index,
			std::vector<CKeyID>& next_block_miner_list);

	// sign the block in a recoverable way
	bool sign(std::shared_ptr<CBlock> pblock);

	// recover the miner of the block from the sig
    // first get pubkey, then get keyid and use cache
	bool getBlockMiner(const CBlockHeader& block, CPubKey& pubkey);
	bool getBlockMiner(const CBlockHeader& block, CKeyID& keyid);
	bool getBlockMiner(const CBlockIndex* p_index, CKeyID& keyid);
};

}  // namespace Poa

#endif /* QTUM_POA_H */
