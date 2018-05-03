#include "poa.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "chainparams.h"
#include "validation.h"
#include "base58.h"
#include "timedata.h"
#include "miner.h"
#include "script/standard.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Poa {

void ThreadPoaMiner() {
	int64_t keySleepInterval = 3000;
	int64_t minerSleepInterval = 100;

	BasicPoa* p_basic_poa = BasicPoa::getInstance();

	if (!p_basic_poa->hasMiner()) {
		LogPrintf("ThreadPoaMiner(): no PoA miner specified, exist miner thread\n");
		return;
	}
	CScript reward_script;
	if (!p_basic_poa->getRewardScript(reward_script)) {
		LogPrintf("ThreadPoaMiner(): fail to get reward script, exist miner thread\n");
		return;
	}

	RenameThread("qtum-poa-miner");

	while (!p_basic_poa->initMinerKey()) {
		LogPrintf("ThreadPoaMiner(): fail to get the miner's key, wait\n");
		MilliSleep(keySleepInterval);
		continue;
	}

	CBlockIndex* p_prev_index = nullptr;
	while (true) {
		CBlockIndex* p_current_index = chainActive.Tip();
		if (p_prev_index == p_current_index) {
			LogPrint(BCLog::COINSTAKE, "ThreadPoaMiner(): tip not change, continue\n");
			MilliSleep(minerSleepInterval);
			continue;
		}
		p_prev_index = p_current_index;

		uint32_t next_block_time;
		if (!p_basic_poa->minerCanProduceNextBlock(p_current_index, next_block_time)) {
			LogPrint(BCLog::COINSTAKE, "ThreadPoaMiner(): unable to produce block next to %s, continue\n",
					p_current_index->GetBlockHash().GetHex());
			MilliSleep(minerSleepInterval);
			continue;
		}

		int64_t nTotalFees = 0;
		std::unique_ptr<CBlockTemplate> pblocktemplatefilled(BlockAssembler(Params()).CreateNewBlock(
				reward_script,
				true,
				false,
				&nTotalFees,
				next_block_time,
				0));
        std::shared_ptr<CBlock> pblockfilled = std::make_shared<CBlock>(pblocktemplatefilled->block);

		LogPrintf("ThreadPoaMiner(): new block created\n%s\n", pblockfilled->ToString().c_str());

		MilliSleep(10000);
	}
}

BasicPoa* BasicPoa::_instance = nullptr;

bool BasicPoa::initParams() {
	const uint32_t DEFAULT_POA_PERIOD = 10000;
	const uint32_t DEFAULT_POA_TIMEOUT = 3000;

	// extract the miner list which cannot be empty for PoA, so return false if fail
	std::string minerListArg = gArgs.GetArg("-poa-miner-list", "");
	if (minerListArg.size() == 0) {
		return false;
	}

	std::vector<std::string> vecMinerList;
	boost::split(vecMinerList, minerListArg, boost::is_any_of(","));

	_miner_list.clear();
	std::set<CKeyID> minerSet;
	std::string strMinerList;
	for (const std::string& strAddress : vecMinerList) {
		CBitcoinAddress address(strAddress);
		CKeyID keyID;
		if (!address.GetKeyID(keyID)) {
			return false;
		}
		auto ret = minerSet.insert(keyID);
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
		CKeyID keyID;
		if (address.GetKeyID(keyID) && minerSet.count(keyID) != 0) {
			_miner = keyID;
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

	LogPrintf("PoA parameters init sucess, miner_list=%s miner=%s, period=%d, timeout=%d\n",
			strMinerList.c_str(), strMiner.c_str(), _period, _timeout);

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

bool BasicPoa::minerCanProduceNextBlock(const CBlockIndex* p_current_index,
		uint32_t& next_block_time) {
	// TODO: add next_block_miner_list

	next_block_time = (uint32_t)(p_current_index->nTime) + _period;

	uint32_t current_time = GetAdjustedTime();
	if (next_block_time < current_time) {  // no block for a long time
		next_block_time = current_time + _timeout;
	}

	// TODO: add n*timeout

	return true;
}
}

