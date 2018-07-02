#ifndef QTUM_POA_H
#define QTUM_POA_H

#include <vector>
#include <set>
#include "miner.h"
#include "dbwrapper.h"
#include "chainparams.h"
#include "wallet/wallet.h"
#include "qtum/qtumDGP.h"

namespace Poa {
const size_t BLOCK_MINER_CACHE_SIZE = 1 << 13;  // 1MB
const size_t NEXT_BLOCK_MINER_LIST_CACHE_SIZE = 3 << 13;  // 3MB
const size_t MAX_MINER_NUM = 1000;
const dev::Address MINER_LIST_DGP_ADDR = dev::Address("0000000000000000000000000000000000000085");
const uint32_t DEFAULT_POA_INTERVAL = 10;
const uint32_t DEFAULT_POA_TIMEOUT = 3;

bool isPoaChain();  // call this function after the chainparams is initiated
std::string const& genesisInfo();  // genesis state for poa, add a dgp for miner list
void ThreadPoaMiner();  // mining thread

// get miner list from dgp
class MinerListDGP{
private:
	std::vector<std::pair<unsigned int, dev::Address>> paramsInstance;

	bool getMinerInstanceForBlockHeight(
			unsigned int block_height,
			unsigned int& activation_height,
			dev::Address& contract_address);
	bool parseContractOutput(
			const std::vector<unsigned char>& contract_output,
			std::vector<CKeyID>& miner_list);

public:
	MinerListDGP(QtumState* state);
	bool getMinerList(
			unsigned int blockHeight,
			std::vector<CKeyID>& miner_list,
			unsigned int& activation_height);
};

class MinerList {
private:
	std::vector<CKeyID> _initial_miner_list;
public:
	MinerList() {}
	bool init();
	void getAuthorizedMiners(
	        int blockHeight,
	        std::vector<CKeyID>& miner_list,
	        int& activation_height);
	bool getNextBlockAuthorizedMiners(
			const CBlockIndex* p_current_index,
			std::vector<CKeyID>& miner_list,
			int& activation_height);
};

class BasicPoa {
private:
	MinerList _miner_list;
	CKeyID _miner;
	uint32_t _interval;
	uint32_t _timeout;

	CScript _reward_script;  // a p2pkh script to the miner

	CKey _miner_key;

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

	// get miner's reward script for building the coinbase
	bool getRewardScript();

	// calculate the miners who can mine the next block
	// first get the miner set, then get their order and use cache
	bool calNextBlockMinerSet(
			const CBlockIndex* p_current_index,
			const std::set<CKeyID>& authorized_miner_set,
			int activation_height,
			std::set<CKeyID>& next_block_miner_set);
	bool calNextBlockMinerList(
			const CBlockIndex* p_current_index,
			std::vector<CKeyID>& next_block_miner_list);
	bool getNextBlockMinerList(
			const CBlockIndex* p_current_index,
			std::vector<CKeyID>& next_block_miner_list);

	// sign the block in a recoverable way
	bool sign(std::shared_ptr<CBlock> pblock);

	// singleton pattern, lazy initialization
	static BasicPoa* _instance;
	BasicPoa():
		_interval(0), _timeout(0),
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
	bool initMiner(const std::string str_miner);
	bool hasMiner() {  // determine if the miner is specified
		return !_miner.IsNull();
	}
	bool initMinerKey();  // try to get the miner's key from wallet

	// determine if the miner can mine the next block
	// if true then return the next_block_time
	bool canMineNextBlock(
			const CKeyID& miner,
			const CBlockIndex* p_current_index,
			uint32_t& next_block_time);  // for validation
	bool canMineNextBlock(
			const CBlockIndex* p_current_index,
			uint32_t& next_block_time);  // for mining

	// create a new block for the miner
	bool createNextBlock(
			uint32_t next_block_time,
			std::shared_ptr<CBlock>& pblock);

	// for the validation
	bool checkBlock(const CBlockHeader& block);

	// recover the miner of the block from the sig
    // first get pubkey, then get keyid and use cache
	bool getBlockMiner(const CBlockHeader& block, CPubKey& pubkey);
	bool getBlockMiner(const CBlockHeader& block, CKeyID& keyid);
	bool getBlockMiner(const CBlockIndex* p_index, CKeyID& keyid);

	MinerList* minerList() {return &_miner_list;}
};

}  // namespace Poa

#endif /* QTUM_POA_H */
