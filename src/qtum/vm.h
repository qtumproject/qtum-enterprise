#pragma once

#include <uint256.h>
#include <coins.h>
#include <chain.h>
#include <chainparams.h>
#include <script/standard.h>
#include <pubkey.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <qtum/qtumtransaction.h>

struct VersionVM;

namespace qtum {
    namespace vm {

        struct QtumTransactionParams{
            VersionVM version;
            uint64_t gasLimit;
            uint64_t gasPrice;
            valtype code;
            dev::Address receiveAddress;
        
            bool fromStack(const std::vector<valtype>& v);
        
            bool operator!=(QtumTransactionParams etp) {
                return (this->version.toRaw() != etp.version.toRaw() || this->gasLimit != etp.gasLimit ||
                this->gasPrice != etp.gasPrice || this->code != etp.code ||
                this->receiveAddress != etp.receiveAddress);
            }
        };
        

        using ExtractQtumTX = std::pair<std::vector<QtumTransaction>, std::vector<QtumTransactionParams>>;

        std::vector<valtype> GetStack(const CScript& scriptPubKey);
        
        
        class QtumTxConverter{
        
        public:
        
            QtumTxConverter(CTransaction tx, CCoinsViewCache* v = NULL, const std::vector<CTransactionRef>* blockTxs = NULL) : txBit(tx), view(v), blockTransactions(blockTxs){}
        
            bool extractionQtumTransactions(ExtractQtumTX& qtumTx);
        
            static bool trimStack(std::vector<valtype>&);
            
            static bool parseEthTXParams(QtumTransactionParams& params,const std::vector<valtype>& stack);
            static bool validateQtumTransactionParameters(QtumTransactionParams& params);
        private:
        
            QtumTransaction createEthTX(const QtumTransactionParams& etp, const uint32_t nOut);
        
            const CTransaction txBit;
            const CCoinsViewCache* view;
            const std::vector<CTransactionRef> *blockTransactions;
        
        };
        


        valtype ExtractAddress(const CScript&);
        uint256 WrapAddress(valtype in);

        struct QtumBlockchainDataFeed {
            struct BlockData {
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
            BlockData extractData(const CBlockIndex*);
        };
        
        struct ExecutionInfo {
            CScript fullSenderScript;
            uint256 senderAddress;
            std::vector<uint256> externalCallStack;
            valtype inputData;
            uint64_t gasLimit;
            uint64_t gasPrice;
            bool allowPaymentConsumption;
            uint64_t totalGasLimit;
            uint256 toAddress;
            VersionVM version;
            CAmount outValue;
        };
        
        struct ExecutionResult {
            uint64_t gasConsumed;
            uint32_t executionStatus;
            std::shared_ptr<const valtype> outputData;
            // const std::map<uint256, std::map<std::vector<uint8_t>, std::vector<uint8_t>>> storageWrites;
            const uint64_t paymentConsumed;
        };

        class QtumVMBase {
            const QtumBlockchainDataFeed& chainFeed;
        public:
            QtumVMBase(const QtumBlockchainDataFeed& chainFeed) : chainFeed(chainFeed) {}
            virtual bool isValidInput(const ExecutionInfo&) = 0;
            virtual ExecutionResult execute(const ExecutionInfo&) = 0;
        };
        
        class QtumEVM : public QtumVMBase {
        public:
            QtumEVM(const QtumBlockchainDataFeed& dataFeed) : QtumVMBase(dataFeed) {}
            ExecutionInfo prepareExecutionInfo(CTxOut& out, CTxOut& prevOut, uint64_t _totalGasLimit);
            bool isValidInput(const ExecutionInfo&) override;
            ExecutionResult execute(const ExecutionInfo&) override;
        };
    
    }
} // namespace qtum::vm