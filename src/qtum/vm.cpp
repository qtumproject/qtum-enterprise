#include "vm.h"

bool ReadBlockFromDisk(CBlock&, const CBlockIndex*, const Consensus::Params&);

using namespace qtum::vm;

uint256 qtum::vm::ExtractAddress(const CScript& creatorOut) {
    CTxDestination dest;
    if (ExtractDestination(creatorOut, dest) && dest.type() == typeid(CKeyID)) {
        valtype bytesID(boost::apply_visitor(DataVisitor(), dest));
        assert(sizeof(uint256) >= sizeof(valtype::value_type) * bytesID.size());
        bytesID.insert(bytesID.begin(), sizeof(uint256) - sizeof(valtype::value_type) * bytesID.size(), 0);
        return uint256(bytesID);
    }
    return uint256();
}

QtumBlockchainDataFeed::BlockData QtumBlockchainDataFeed::extractData(const CBlock& block) {
    auto& creatorOut(block.vtx[block.IsProofOfStake() ? 1 : 0]->vout[block.IsProofOfStake() ? 1 : 0]);
    auto creator(ExtractAddress(creatorOut.scriptPubKey));
    return BlockData{block.GetHash(), creator, block.nTime, block.nBits};
} 

QtumBlockchainDataFeed::QtumBlockchainDataFeed (const CChain& chain, const CBlock& currentBlock) : 
                                                currentBlockData(extractData(currentBlock)),
                                                currentBlockNum(chain.Height() + 1) {
    CBlockIndex* tip = chain.Tip();
    prevBlocksData.resize(256);
    
    for(int i = 0; i < 256; i++){
        if(!tip)
            break;
        CBlock block;
        if (!ReadBlockFromDisk(block, tip, Params().GetConsensus()))
            break;
            prevBlocksData[i] = extractData(block);
        tip = tip->pprev;
    }
}

// std::vector<uint8_t>& fullSenderScript;
// uint256& senderAddress;
// std::vector<uint256>& externalCallStack;
// std::vector<uint8_t>& inputData;
// uint64_t gasLimit;
// uint64_t gasPrice;
// bool allowPaymentConsumption;
// uint64_t totalGasLimit;
// uint256 toAddress;

ExecutionInfo QtumEVM::PrepareExecutionInfo(CTransactionRef tx, size_t nOut, uint64_t _totalGasLimit, CCoinsViewCache& view) {
    ExecutionInfo ret;
    ret.totalGasLimit = _totalGasLimit;
    // convert the scriptSig into a stack, so we can inspect exec data
    // std::vector<std::vector<unsigned char> > stack;
    // if (!EvalScript(stack, tx.vout[nOut].scriptPubKey, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SIGVERSION_BASE));
    //     return false;
}

ExecutionResult QtumEVM::Execute(const ExecutionInfo&, const VersionVM, const uint256 address, 
    const std::vector<uint8_t>& bytecode, const uint64_t) {
    
}