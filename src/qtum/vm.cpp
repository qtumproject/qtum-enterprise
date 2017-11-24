#include "vm.h"

bool ReadBlockFromDisk(CBlock&, const CBlockIndex*, const Consensus::Params&);
bool GetTransaction(const uint256 &hash, CTransactionRef &tx, const Consensus::Params& params, uint256 &hashBlock, bool fAllowSlow = false);


using namespace qtum::vm;

dev::eth::EnvInfo BuildEVMEnvironment(const QtumBlockchainDataFeed& df) {
    dev::eth::EnvInfo env;
    env.setNumber(df.currentBlockNum);
    env.setTimestamp(df.currentBlockData.blockTime);
    env.setDifficulty(df.currentBlockData.blockDifficulty);

    dev::eth::LastHashes lh;
    lh.reserve(256);
    for(auto it = df.prevBlocksData.crbegin(); it != df.prevBlocksData.crend(); it++) {
        if(it->blockHash == uint256())
            break;
        lh.emplace_back(uintToh256(it->blockHash));
    }
    env.setLastHashes(std::move(lh));
    env.setGasLimit(df.currentBlockGasLimit);
    env.setAuthor(dev::Address(UnwrapAddress(df.currentBlockData.blockCreator)));
    return env;
}

valtype qtum::vm::ExtractAddress(const CScript& out) {
    CTxDestination dest;
    if (ExtractDestination(out, dest) && dest.type() == typeid(CKeyID))
        return boost::apply_visitor(DataVisitor(), dest);
    return valtype();
}

opt_script SenderExtractor::tryFromBlock(const COutPoint& out) {
    if (bTxs == nullptr) return opt_script();
    for(auto btx : *bTxs)
        if(btx->GetHash() == out.hash)
            return opt_script(btx->vout[out.n].scriptPubKey);
    return opt_script();
}

opt_script SenderExtractor::tryFromCoins(const COutPoint& out) {
    if (cvc == nullptr) return opt_script();
    auto coin = cvc->AccessCoin(out);
    return coin.nHeight != 0 ? opt_script(coin.out.scriptPubKey) : opt_script(); 
}

opt_script SenderExtractor::tryFromGetTx(const COutPoint& out) {
    CTransactionRef txPrevout;
    uint256 hashBlock;
    return GetTransaction(out.hash, txPrevout, Params().GetConsensus(), hashBlock, true) ?
        opt_script(txPrevout->vout[out.n].scriptPubKey) : opt_script();
}

opt_script SenderExtractor::getScript(const COutPoint& out) {
    opt_script script;
    if ((script = tryFromBlock(out))
        || (script = tryFromCoins(out))
        || (script = tryFromGetTx(out)))
        return script;
    return opt_script();
}

valtype SenderExtractor::getSender(const COutPoint& out) {
    opt_script script;
    if (script = getScript(out))
        return ExtractAddress(*script);
    return valtype();
}

valtype qtum::GetSenderAddress(const COutPoint& out, const CCoinsViewCache* coinsView, const std::vector<CTransactionRef>* blockTxs) {
    return SenderExtractor(coinsView, blockTxs).getSender(out);
}

bool QtumTxConverter::extractQtumTransactions(QtumTransactions& qtumtx){
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
        
    txEth.forceSender(dev::Address(sender));
    txEth.setHashWith(uintToh256(txBit.GetHash()));
    txEth.setNVout(nOut);
    txEth.setVersion(etp.version);

    return txEth;
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

QtumBlockchainDataFeed::QtumBlockchainDataFeed (CBlockIndex* lastIdx, const CBlock& currentBlock, const uint64_t bGasLimit) : 
                                                currentBlockData(extractData(currentBlock)), currentBlockGasLimit(bGasLimit) {
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

ResultExecute QtumEVM::execute(const QtumTransaction& tx) {
    if(!tx.isCreation() && !globalState->addressInUse(tx.receiveAddress())){
        dev::eth::ExecutionResult execRes;
        execRes.excepted = dev::eth::TransactionException::Unknown;
        return ResultExecute{execRes, dev::eth::TransactionReceipt(dev::h256(), dev::u256(), dev::eth::LogEntries()), CTransaction()};
    }
    return globalState->execute(BuildEVMEnvironment(chainFeed), *globalSealEngine.get(), tx, type, OnOpFunc());
}