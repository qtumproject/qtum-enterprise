#pragma once

#include <uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <script/standard.h>
#include <pubkey.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <qtum/qtumtransaction.h>

struct CCoinsViewCache;
struct VersionVM;

namespace qtum {
    namespace vm {

        uint256 ExtractAddress(const CScript&);

        struct QtumBlockchainDataFeed {
            struct BlockData{
                uint256 blockHash;
                uint256 blockCreator;
                uint64_t blockTime;
                uint32_t blockDifficulty;
            };

            std::vector<BlockData> prevBlocksData;
            const BlockData currentBlockData;
            const uint32_t currentBlockNum;

            QtumBlockchainDataFeed(const CChain&, const CBlock&);

        private:
            BlockData extractData(const CBlock&);
        };
        
        struct ExecutionInfo {
            std::vector<uint8_t> fullSenderScript;
            uint256 senderAddress;
            std::vector<uint256> externalCallStack;
            std::vector<uint8_t> inputData;
            uint64_t gasLimit;
            uint64_t gasPrice;
            bool allowPaymentConsumption;
            uint64_t totalGasLimit;
            uint256 toAddress;
        };
        
        struct ExecutionResult {
            uint64_t gasConsumed;
            uint32_t executionStatus;
            std::shared_ptr<const std::vector<uint8_t>> outputData;
            // const std::map<uint256, std::map<std::vector<uint8_t>, std::vector<uint8_t>>> storageWrites;
            const uint64_t paymentConsumed;
        };

        class QtumVMBase {
            const QtumBlockchainDataFeed& chainFeed;
        public:
            QtumVMBase(const QtumBlockchainDataFeed& chainFeed) : chainFeed(chainFeed) {}
            virtual ExecutionResult Execute(const ExecutionInfo&, const VersionVM, const uint256 address, 
                                const std::vector<uint8_t>& bytecode, const uint64_t) = 0;
        };
        
        class QtumEVM : public QtumVMBase {
        public:
            QtumEVM(const QtumBlockchainDataFeed& dataFeed) : QtumVMBase(dataFeed) {}
            ExecutionInfo PrepareExecutionInfo(CTransactionRef, size_t nOut, uint64_t _totalGasLimit, CCoinsViewCache&);
            ExecutionResult Execute(const ExecutionInfo&, const VersionVM, const uint256 address, 
                        const std::vector<uint8_t>& bytecode, const uint64_t) override;
        };
    
    }
} // namespace qtum::vm