#include "poa.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "validation.h"
#include "base58.h"
#include "timedata.h"
#include "script/standard.h"
#include "consensus/merkle.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Poa {

void ThreadPoaMiner() {
	int64_t keySleepInterval = 3000;
	int64_t minerSleepInterval = 100;

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
		MilliSleep(5000);
		if (chainActive.Tip() != p_current_index) {
			continue;
		}

        bool fNewBlock = false;
        if (!ProcessNewBlock(Params(), pblock, true, &fNewBlock)) {  // TODO: fail here
        	LogPrint(BCLog::COINSTAKE, "ERROR: %s: process new block fail\n", __func__);
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
		const CKeyID& keyid,
		const CBlockIndex* p_current_index,
		uint32_t& next_block_time) {
	if (p_current_index == nullptr) {
		return false;
	}

	// TODO: add next_block_miner_list

	next_block_time = (uint32_t)(p_current_index->nTime) + _period;
	// TODO: add n*timeout

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
	CKeyID keyid;
	if (!getBlockMiner(block, keyid)) {
		LogPrintf("%s: new block get miner fail\n", __func__);
	} else {
		LogPrintf("%s: block miner is %s\n", __func__, CBitcoinAddress(keyid).ToString().c_str());
	}
	getBlockMiner(block, keyid);

	return true;
}

bool BasicPoa::getNextBlockMinerSet(
		const CBlockIndex* p_current_index,
		std::set<CKeyID>& next_block_miner_set) {
	if (p_current_index == nullptr) {
		return false;
	}

	// TODO: get recent n/2 block miner

	// TODO: subtract these miners from miner set

	return true;
}

bool BasicPoa::getNextBlockMinerList(
		const CBlockIndex* p_current_index,
		std::vector<CKeyID>& next_block_miner_list) {
	if (p_current_index == nullptr) {
		return false;
	}

	// TODO: read cache

	std::set<CKeyID> next_block_miner_set;
	if (!getNextBlockMinerSet(p_current_index, next_block_miner_set)) {
		return false;
	}

	// TODO: determine their order

	// TODO: write cache

	return true;
}

bool BasicPoa::sign(std::shared_ptr<CBlock> pblock) {
	if (!pblock) {
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

	uint256 hash = block.GetHash();
	if (readBlockMinerFromCache(hash, keyid)) {
		return true;
	}

	CPubKey pubkey;
	if (!getBlockMiner(block, pubkey)) {  // time consuming, so use cache
		return false;
	}

	keyid = pubkey.GetID();
	if (!writeBlockMinerToCache(hash, keyid)) {
		LogPrintf("ERROR: %s: fail to write block %s miner %s to cache\n",
				__func__, hash.GetHex().c_str(), CBitcoinAddress(keyid).ToString().c_str());
	}

	return true;
}
}

