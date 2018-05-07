#include "poa.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "validation.h"
#include "base58.h"
#include "timedata.h"
#include "script/standard.h"
#include "consensus/merkle.h"

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Poa {

void ThreadPoaMiner() {
	int64_t keySleepInterval = 3000;
	int64_t minerSleepInterval = 500;

	BasicPoa* p_basic_poa = BasicPoa::getInstance();

	if (!p_basic_poa->hasMiner()) {
		LogPrintf("%s: no PoA miner specified, exist miner thread\n", __func__);
		return;
	}
	CScript reward_script;
	if (!p_basic_poa->getRewardScript(reward_script)) {
		LogPrintf("ERROR: %s: fail to get reward script, exist miner thread\n", __func__);
		return;
	}

	RenameThread("qtum-poa-miner");

	while (!p_basic_poa->initMinerKey()) {
		LogPrintf("%s: fail to get the miner's key, wait\n", __func__);
		MilliSleep(keySleepInterval);
		continue;
	}

	CBlockIndex* p_last_index = nullptr;
	while (true) {
		CBlockIndex* p_current_index = chainActive.Tip();
		if (p_last_index == p_current_index) {
			LogPrint(BCLog::COINSTAKE, "%s: the chain tip not change, continue\n", __func__);
			MilliSleep(minerSleepInterval);
			continue;
		}
		p_last_index = p_current_index;

		uint32_t next_block_time;
		if (!p_basic_poa->canMineNextBlock(p_current_index, next_block_time)) {
			LogPrint(BCLog::COINSTAKE, "%s: the miner is not able to mine a block next to the chain tip, continue\n",
					__func__);
			continue;
		}
		if (p_current_index != chainActive.Tip()) {
			LogPrint(BCLog::COINSTAKE, "%s: the chain tip changes during authority check, continue\n", __func__);
			continue;
		}

		// generate new block
		std::shared_ptr<CBlock> pblock;
		if (!p_basic_poa->createNextBlock(next_block_time, pblock)) {
			LogPrint(BCLog::COINSTAKE, "ERROR: %s: fail to create a new block next to the chain tip, continue\n", __func__);
			continue;
		}
		if (p_current_index != chainActive.Tip()) {
			LogPrint(BCLog::COINSTAKE, "%s: the chain tip changes during create block, continue\n", __func__);
			continue;
		}
		LogPrint(BCLog::COINSTAKE, "%s: new block is created\n%s\n", __func__, pblock->ToString().c_str());

		// TODO: wait and add the block, if new block is mined during wait
		while (GetAdjustedTime() < next_block_time && chainActive.Tip() == p_current_index) {
			LogPrint(BCLog::COINSTAKE, "%s: waiting for the new block time\n", __func__);
			MilliSleep(minerSleepInterval);
		}
		if (chainActive.Tip() != p_current_index) {
			LogPrint(BCLog::COINSTAKE, "%s: the chain tip changes during block time waiting, continue\n", __func__);
			continue;
		}

        bool fNewBlock = false;
        if (!ProcessNewBlock(Params(), pblock, true, &fNewBlock)) {
        	LogPrint(BCLog::COINSTAKE, "ERROR: %s: process new block fail\n", __func__);
        	continue;
        }
	}
}

BasicPoa* BasicPoa::_instance = nullptr;

bool BasicPoa::initParams() {
	const uint32_t DEFAULT_POA_PERIOD = 10;
	const uint32_t DEFAULT_POA_TIMEOUT = 3;

	// extract the miner list which cannot be empty for PoA, so return false if fail
	std::string minerListArg = gArgs.GetArg("-poa-miner-list", "");
	if (minerListArg.size() == 0) {
		return false;
	}

	std::vector<std::string> vecMinerList;
	boost::split(vecMinerList, minerListArg, boost::is_any_of(","));

	_miner_list.clear();
	_miner_set.clear();
	std::string strMinerList;
	for (const std::string& strAddress : vecMinerList) {
		CBitcoinAddress address(strAddress);
		CKeyID keyID;
		if (!address.GetKeyID(keyID)) {
			return false;
		}
		auto ret = _miner_set.insert(keyID);
		if (!ret.second) {  // duplicate miner
			return false;
		}
		_miner_list.push_back(keyID);
		strMinerList += address.ToString() + ",";
	}
	if (_miner_list.size() == 0) {
		return false;
	}

	// extract the miner
	std::string minerArg = gArgs.GetArg("-poa-miner", "");
	std::string strMiner;
	if (minerArg.size() != 0) {
		CBitcoinAddress address(minerArg);
		CKeyID keyid;
		if (address.GetKeyID(keyid) && _miner_set.count(keyid) != 0) {
			_miner = keyid;
			strMiner = address.ToString();
		}
	}
	_reward_script = GetScriptForDestination(_miner);

	// extract period & timeout
	if (!ParseUInt32(gArgs.GetArg("-poa-period", ""), &_period)) {
		_period = DEFAULT_POA_PERIOD;
	}
	if (!ParseUInt32(gArgs.GetArg("-poa-timeout", ""), &_timeout)) {
		_timeout = DEFAULT_POA_TIMEOUT;
	}

	LogPrintf("%s: PoA parameters init sucess, miner_list=%s miner=%s, period=%d, timeout=%d\n",
			__func__, strMinerList.c_str(), strMiner.c_str(), _period, _timeout);

	return true;
}

bool BasicPoa::initMinerKey() {
    for (CWalletRef pwallet : vpwallets) {
        if (pwallet->GetKey(_miner, _miner_key)) {
        	return true;
        }
    }

	return false;
}

bool BasicPoa::canMineNextBlock(
		const CKeyID& miner,
		const CBlockIndex* p_current_index,
		uint32_t& next_block_time) {
	if (p_current_index == nullptr) {
		return false;
	}

	// get next_block_miner_list
	std::vector<CKeyID> next_block_miner_list;
	if (!getNextBlockMinerList(p_current_index, next_block_miner_list)) {
		LogPrintf("%s: getNextBlockMinerList fail\n", __func__);
		return false;
	}

	std::vector<CKeyID>::iterator it = std::find(
			next_block_miner_list.begin(),
			next_block_miner_list.end(),
			miner);
	if (it == next_block_miner_list.end()) {
		LogPrint(BCLog::COINSTAKE, "%s: miner %s is not in next_block_miner_list\n",
				__func__, CBitcoinAddress(miner).ToString().c_str());
		return false;
	}

	// get block time
	uint32_t miner_index = std::distance(next_block_miner_list.begin(), it);
	next_block_time = (uint32_t)(p_current_index->nTime) + _period + miner_index * _timeout;
	LogPrint(BCLog::COINSTAKE, "%s: next_block_time = %s + %s + %s * %s\n",
					__func__, p_current_index->nTime, _period, miner_index, _timeout);

	return true;
}

bool BasicPoa::canMineNextBlock(
		const CBlockIndex* p_current_index,
		uint32_t& next_block_time) {
	if (p_current_index == nullptr) {
		return false;
	}

	if (!canMineNextBlock(_miner, p_current_index, next_block_time)) {
		return false;
	}

	// TODO: time adjustment, for long time no block
	uint32_t current_time = GetAdjustedTime();
	if (next_block_time < current_time) {  // no block for a long time
		next_block_time = current_time;
	}

	return true;
}

bool BasicPoa::createNextBlock(
		uint32_t next_block_time,
		std::shared_ptr<CBlock>& pblock) {

	int64_t nTotalFees = 0;
	std::unique_ptr<CBlockTemplate> pblocktemplate(_block_assembler.CreateNewBlock(
			_reward_script,
			true,
			false,
			&nTotalFees,
			next_block_time,
			0));

	pblock = std::make_shared<CBlock>(pblocktemplate->block);

    if (!pblock) {
		return false;
    }

    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    if (!sign(pblock)) {
    	return false;
    }

	return true;
}

bool BasicPoa::checkBlock(const CBlockHeader& block) {
	if (block.IsNull()) {
		return false;
	}

	uint256 hash = block.GetHash();
	if (hash == Params().GetConsensus().hashGenesisBlock) {  // genesis block
		return true;
	}

	// get prev block index
	uint256 hash_prev = block.hashPrevBlock;
	BlockMap::iterator it_prev = mapBlockIndex.find(hash_prev);
	if (it_prev == mapBlockIndex.end()) {
		LogPrint(BCLog::COINSTAKE, "%s: fail to get prev block index of block %s\n", __func__, hash.ToString().c_str());
		return false;
	}

	// get block miner
	CKeyID miner;
	if (!getBlockMiner(block, miner)) {
		LogPrintf("%s: fail to get miner of block %s\n", __func__, hash.ToString().c_str());
		return false;
	}

	// determine the miner can mine this block
	uint32_t assigned_block_time;
	if (!canMineNextBlock(miner, it_prev->second, assigned_block_time)) {
		LogPrintf("%s: miner %s is not authorized to mine block %s\n", __func__, CBitcoinAddress(miner).ToString(), hash.ToString().c_str());
		return false;
	}

	// block time should be later than the assigned time
	if (block.nTime < assigned_block_time) {
		LogPrintf("%s: block %s time %d is earlier than assigned time %d\n",
				__func__, hash.ToString().c_str(), block.nTime, assigned_block_time);
		return false;
	}

	return true;
}

bool BasicPoa::getNextBlockMinerSet(
		const CBlockIndex* p_current_index,
		std::set<CKeyID>& next_block_miner_set) {
	if (p_current_index == nullptr) {
		return false;
	}
	if (p_current_index->pprev == nullptr) {  // genesis block
		next_block_miner_set = _miner_set;
		return true;
	}
	next_block_miner_set.clear();

	// get recent n/2 block miners
	std::set<CKeyID> recent_block_miner_set;
	size_t miner_num = _miner_list.size();
	size_t recent_block_num = miner_num / 2;
	const CBlockIndex* p_index_tmp = p_current_index;

	while (recent_block_num != 0 && p_index_tmp->pprev != nullptr) {
		CKeyID keyid;
		if (!getBlockMiner(p_index_tmp, keyid)) {
			return false;
		}
		recent_block_miner_set.insert(keyid);

		p_index_tmp = p_index_tmp->pprev;
		recent_block_num--;
	}

	// subtract recent miners from miner set
	std::vector<CKeyID> next_block_miner_vec(miner_num);
	std::vector<CKeyID>::iterator it = std::set_difference(
			_miner_set.begin(), _miner_set.end(),
			recent_block_miner_set.begin(), recent_block_miner_set.end(),
			next_block_miner_vec.begin());
	next_block_miner_set.insert(next_block_miner_vec.begin(), it);

	return true;
}

bool BasicPoa::getNextBlockMinerList(
		const CBlockIndex* p_current_index,
		std::vector<CKeyID>& next_block_miner_list) {
	if (p_current_index == nullptr || p_current_index->phashBlock == nullptr) {
		return false;
	}
	if (p_current_index->pprev == nullptr) {  // genesis block
		next_block_miner_list = _miner_list;
		return true;
	}
	next_block_miner_list.clear();

	// read cache
	uint256 hash = *(p_current_index->phashBlock);
	if (readNextBlockMinerListFromCache(hash, next_block_miner_list)) {
		return true;
	}

	// get next_block_miner_set
	std::set<CKeyID> next_block_miner_set;
	if (!getNextBlockMinerSet(p_current_index, next_block_miner_set)) {
		return false;
	}
	// debug log
	std::string next_block_miner_set_str;
	for (const CKeyID& keyid: next_block_miner_set) {
		next_block_miner_set_str += CBitcoinAddress(keyid).ToString() + ",";
	}
	if (next_block_miner_set_str.size() != 0) {
		next_block_miner_set_str.pop_back();
	}
	LogPrint(BCLog::COINSTAKE, "%s: next_block_miner_set is {%s}\n", __func__, next_block_miner_set_str.c_str());

	// determine their order
	CKeyID current_miner;
	if (!getBlockMiner(p_current_index, current_miner)) {
		return false;
	}
	std::vector<CKeyID>::iterator current_it = std::find(_miner_list.begin(), _miner_list.end(), current_miner);
	if (current_it == _miner_list.end()) {
		return false;
	}
	std::vector<CKeyID>::iterator it = current_it;
	do {
		if (++it == _miner_list.end()) {
			it = _miner_list.begin();
		}

		if (next_block_miner_set.count(*it) != 0) {
			next_block_miner_list.push_back(*it);
		}
	} while (it != current_it);
	// debug log
	std::string next_block_miner_list_str;
	for (const CKeyID& keyid: next_block_miner_list) {
		next_block_miner_list_str += CBitcoinAddress(keyid).ToString() + ",";
	}
	if (next_block_miner_list_str.size() != 0) {
		next_block_miner_list_str.pop_back();
	}
	LogPrint(BCLog::COINSTAKE, "%s: next_block_miner_list is [%s]\n", __func__, next_block_miner_list_str.c_str());

	// write cache
	if (!writeNextBlockMinerListToCache(hash, next_block_miner_list)) {
		LogPrintf("ERROR: %s: fail to write block %s miner list [%s] to cache\n",
				__func__, hash.GetHex().c_str(), next_block_miner_list_str.c_str());
	}

	return true;
}

bool BasicPoa::sign(std::shared_ptr<CBlock> pblock) {
	if (!pblock || pblock->IsNull()) {
		return false;
	}

	if (!_miner_key.SignCompact(pblock->GetHashWithoutSign(), pblock->vchBlockSig)) {
		return false;
	}

	return true;
}

bool BasicPoa::getBlockMiner(const CBlockHeader& block, CPubKey& pubkey) {
	if (block.IsNull() || block.vchBlockSig.size() == 0) {
		return false;
	}

	return pubkey.RecoverCompact(block.GetHashWithoutSign(), block.vchBlockSig);
}

bool BasicPoa::getBlockMiner(const CBlockHeader& block, CKeyID& keyid) {
	if (block.IsNull() || block.vchBlockSig.size() == 0) {
		return false;
	}

	// read cache
	uint256 hash = block.GetHash();
	if (readBlockMinerFromCache(hash, keyid)) {
		return true;
	}

	CPubKey pubkey;
	if (!getBlockMiner(block, pubkey)) {  // time consuming, so use cache
		return false;
	}
	keyid = pubkey.GetID();

	// write cache
	if (!writeBlockMinerToCache(hash, keyid)) {
		LogPrintf("ERROR: %s: fail to write block %s miner %s to cache\n",
				__func__, hash.GetHex().c_str(), CBitcoinAddress(keyid).ToString().c_str());
	}

	return true;
}

bool BasicPoa::getBlockMiner(const CBlockIndex* p_index, CKeyID& keyid) {
	if (p_index == nullptr
			|| p_index->phashBlock == nullptr
			|| p_index->vchBlockSig.size() == 0) {
		return false;
	}

	// read cache
	uint256 hash = *(p_index->phashBlock);
	if (readBlockMinerFromCache(hash, keyid)) {
		return true;
	}

	CPubKey pubkey;
	if (!getBlockMiner(p_index->GetBlockHeader(), pubkey)) {  // time consuming, so use cache
		return false;
	}
	keyid = pubkey.GetID();

	// write cache
	if (!writeBlockMinerToCache(hash, keyid)) {
		LogPrintf("ERROR: %s: fail to write block %s miner %s to cache\n",
				__func__, hash.GetHex().c_str(), CBitcoinAddress(keyid).ToString().c_str());
	}

	return true;
}
}

