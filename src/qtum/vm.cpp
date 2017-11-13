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

// const std::vector<uint8_t>& fullSenderScript;
// const uint256& senderAddress;
// const std::vector<uint256>& externalCallStack;
// const std::vector<uint8_t>& inputData;
// const uint64_t gasLimit;
// const uint64_t gasPrice;
// const bool allowPaymentConsumption;
// const uint64_t totalGasLimit;
// const uint256& toAddress;

// ExecutionInfo::ExecutionInfo(CTransactionRef tx, size_t nOut, uint64_t _totalGasLimit, CCoinsViewCache& view) : totalGasLimit() {
//     fullSenderScript = valtype(tx.vout[nOut].scriptPubKey.begin(), tx.vout[nOut].scriptPubKey.end());
//     senderAddress = ExtractAddress(tx.vout[nOut].scriptPubKey);
    
// }

ExecutionResult QtumEVM::Execute(const ExecutionInfo&, const VersionVM, const uint256 address, 
    const std::vector<uint8_t>& bytecode, const uint64_t) {
    
}