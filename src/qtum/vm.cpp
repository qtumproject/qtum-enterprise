#include "vm.h"

bool ReadBlockFromDisk(CBlock&, const CBlockIndex*, const Consensus::Params&);
bool GetTransaction(const uint256 &hash, CTransactionRef &tx, const Consensus::Params& params, uint256 &hashBlock, bool fAllowSlow = false);

using namespace qtum::vm;


valtype GetSenderAddress(const CTransaction& tx, const CCoinsViewCache* coinsView, const std::vector<CTransactionRef>* blockTxs){
    CScript script;
    // First check the current (or in-progress) block for zero-confirmation change spending that won't yet be in txindex
    if(blockTxs){
        for(auto btx : *blockTxs){
            if(btx->GetHash() == tx.vin[0].prevout.hash){
                script = btx->vout[tx.vin[0].prevout.n].scriptPubKey;
                break;
            }
        }
    }
    if(script.empty()) {
        if(coinsView)
            script = coinsView->AccessCoin(tx.vin[0].prevout).out.scriptPubKey;
        else
        {
            CTransactionRef txPrevout;
            uint256 hashBlock;
            if(GetTransaction(tx.vin[0].prevout.hash, txPrevout, Params().GetConsensus(), hashBlock, true)){
                script = txPrevout->vout[tx.vin[0].prevout.n].scriptPubKey;
            } else {
                // LogPrintf("Error fetching transaction details of tx %s. This will probably cause more errors", tx.vin[0].prevout.hash.ToString());
                return valtype();
            }
        }
    } 

	CTxDestination addressBit;
    txnouttype txType=TX_NONSTANDARD;
	if(ExtractDestination(script, addressBit, &txType)){
		if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) &&
                addressBit.type() == typeid(CKeyID)){
			CKeyID senderAddress(boost::get<CKeyID>(addressBit));
			return valtype(senderAddress.begin(), senderAddress.end());
		}
	}
    //prevout is not a standard transaction format, so just return 0
    return valtype();
}

bool QtumTxConverter::extractionQtumTransactions(ExtractQtumTX& qtumtx){
    std::vector<QtumTransaction> resultTX;
    for(size_t i = 0; i < txBit.vout.size(); i++){
        if(txBit.vout[i].scriptPubKey.HasOpCreate() || txBit.vout[i].scriptPubKey.HasOpCall()){
            auto stack = GetStack(txBit.vout[i].scriptPubKey);
            if (!trimStack(stack))
                return false;

            QtumTransactionParams params;
            if(!parseEthTXParams(params, stack))
                return false;
            resultTX.push_back(createEthTX(params, i));
        }
    }
    qtumtx = resultTX;
    return true;
}

std::vector<valtype> qtum::vm::GetStack(const CScript& scriptPubKey){
    std::vector<valtype> stack;
    EvalScript(stack, scriptPubKey, SCRIPT_EXEC_BYTE_CODE, BaseSignatureChecker(), SIGVERSION_BASE, nullptr);
    return stack;
}

bool QtumTransactionParams::fromStack(const std::vector<valtype>& v) try {
    auto it = std::prev(v.end());
    if (v.size() == 5) {
        if (it->size() < 20)
            return false;
        receiveAddress = dev::Address(*it--);
    }

    code = (*it--);
    gasPrice = CScriptNum::vch_to_uint64(*it--);
    gasLimit = CScriptNum::vch_to_uint64(*it--);
    
    if(it->size() > 4) return false;

    version = VersionVM::fromRaw((uint32_t)CScriptNum::vch_to_uint64(*it--));
    return true;
} catch(const scriptnum_error& err){
    // LogPrintf("Incorrect parameters to VM.");
    return false;
}


bool QtumTxConverter::trimStack(std::vector<valtype>& stack) {
    if (stack.size() == 0) return false; 
    CScript scriptRest(stack.back().begin(), stack.back().end());
    stack.pop_back();

    auto op = (opcodetype)(*scriptRest.begin());
    if (stack.size() < (op == OP_CALL ? 5 : 4))
        return false;

    stack.erase(stack.begin(), std::prev(stack.end(), op == OP_CALL ? 5 : 4));
    return true;
}

bool QtumTxConverter::validateQtumTransactionParameters(QtumTransactionParams& params) {
    if(params.gasPrice > INT64_MAX || params.gasLimit > INT64_MAX)
        return false;

    //we track this as CAmount in some places, which is an int64_t, so constrain to INT64_MAX
    if(params.gasPrice !=0 && params.gasLimit > INT64_MAX / params.gasPrice)
        return false; //overflows past 64bits, reject this tx
    
    if (params.code.empty())
        return false;
    
    return true;
}

bool QtumTxConverter::parseEthTXParams(QtumTransactionParams& params,const std::vector<valtype>& stack){
        QtumTransactionParams ret;
        if (!ret.fromStack(stack))
            return false;

        if (!validateQtumTransactionParameters(ret))
            return false;

        params = ret;
        return true;
}

QtumTransaction QtumTxConverter::createEthTX(const QtumTransactionParams& etp, uint32_t nOut){
    QtumTransaction txEth;
    if (etp.receiveAddress == dev::Address())
        txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.code, dev::u256(0));
    else
        txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.receiveAddress, etp.code, dev::u256(0));
        
    dev::Address sender(GetSenderAddress(txBit, view, blockTransactions));
    txEth.forceSender(sender);
    txEth.setHashWith(uintToh256(txBit.GetHash()));
    txEth.setNVout(nOut);
    txEth.setVersion(etp.version);

    return txEth;
}

valtype qtum::vm::ExtractAddress(const CScript& creatorOut) {
    CTxDestination dest;
    if (ExtractDestination(creatorOut, dest) && dest.type() == typeid(CKeyID))
        return boost::apply_visitor(DataVisitor(), dest);
    return valtype();
}

uint256 qtum::vm::WrapAddress(valtype in){
    assert(sizeof(uint256) >= sizeof(valtype::value_type) * in.size());
    in.insert(in.begin(), sizeof(uint256) - sizeof(valtype::value_type) * in.size(), 0);
    return uint256(in);
}

valtype qtum::vm::UnwrapAddress(uint256 in) {
    return valtype(in.end() - 20, in.end());
}

QtumBlockchainDataFeed::BlockData QtumBlockchainDataFeed::extractData(const CBlockIndex* block) {
    return BlockData{block->GetBlockHash(), uint256(), block->nTime, block->nBits};
}

QtumBlockchainDataFeed::BlockData QtumBlockchainDataFeed::extractData(const CBlock& block) {
    auto& creatorOut(block.vtx[block.IsProofOfStake() ? 1 : 0]->vout[block.IsProofOfStake() ? 1 : 0]);
    auto dest(ExtractAddress(creatorOut.scriptPubKey));
    return BlockData{block.GetHash(), WrapAddress(dest), block.nTime, block.nBits};
} 

QtumBlockchainDataFeed::QtumBlockchainDataFeed (CBlockIndex* lastIdx, const CBlock& currentBlock) : 
                                                currentBlockData(extractData(currentBlock)) {
    if (lastIdx != nullptr)
        currentBlockNum = lastIdx->nHeight + 1;
    CBlockIndex* tip = lastIdx;
    prevBlocksData.resize(256);
    for(int i = 0; i < 256; i++){
        if(tip == nullptr)
            break;
        prevBlocksData[i] = extractData(tip);
        tip = tip->pprev;
    }
}

bool QtumEVM::isValidInput(const ExecutionInfo& in) {
    if(in.gasPrice > INT64_MAX || in.gasLimit > INT64_MAX)
        return false;

    //we track this as CAmount in some places, which is an int64_t, so constrain to INT64_MAX
    if(in.gasPrice !=0 && in.gasLimit > INT64_MAX / in.gasPrice)
        return false; //overflows past 64bits, reject this tx

    if (in.inputData.empty())
        return false;

    if(in.version.toRaw() != VersionVM::GetEVMDefault().toRaw())
        return false;

    if (!in.blockChainFeed)
        return false;

    return true;
}

ExecutionInfo QtumEVM::prepareExecutionInfo(CTxOut& out, CTxOut& prevOut, uint64_t _totalGasLimit) {
    std::vector<valtype> stack(GetStack(out.scriptPubKey));
    if (!QtumTxConverter::trimStack(stack))
        return ExecutionInfo();
    
    QtumTransactionParams params;
    if (!QtumTxConverter::parseEthTXParams(params, stack))
        return ExecutionInfo();

    return ExecutionInfo{CScript(), WrapAddress(ExtractAddress(prevOut.scriptPubKey)), std::vector<uint256>(), params.code,
                         params.gasLimit, params.gasPrice, false, _totalGasLimit, WrapAddress(params.receiveAddress.asBytes()),
                         params.version, out.nValue, std::shared_ptr<QtumBlockchainDataFeed>()};
}

ExecutionResult QtumEVM::execute(const ExecutionInfo&) {
    // dev::eth::EnvInfo envInfo(BuildEVMEnvironment());
    //     //validate VM version
    //     if(!tx.isCreation() && !globalState->addressInUse(tx.receiveAddress())){
    //         dev::eth::ExecutionResult execRes;
    //         execRes.excepted = dev::eth::TransactionException::Unknown;
    //         result.push_back(ResultExecute{execRes, dev::eth::TransactionReceipt(dev::h256(), dev::u256(), dev::eth::LogEntries()), CTransaction()});
    //         continue;
    //     }
    //     result.push_back(globalState->execute(envInfo, *globalSealEngine.get(), tx, type, OnOpFunc()));
    // }
    // globalState->db().commit();
    // globalState->dbUtxo().commit();
    // globalSealEngine.get()->deleteAddresses.clear();
}