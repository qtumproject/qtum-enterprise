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
#include <boost/optional.hpp>
#include <qtum/qtumstate.h>

struct VersionVM;
struct QtumState;
struct ResultExecute;

extern std::unique_ptr<QtumState> globalState;
extern std::shared_ptr<dev::eth::SealEngineFace> globalSealEngine;

namespace qtum {
    valtype GetSenderAddress(const COutPoint& out, const CCoinsViewCache* = nullptr, const std::vector<CTransactionRef>* = nullptr);
    namespace vm {

        using opt_script = boost::optional<CScript>;        

        struct SenderExtractor{
            SenderExtractor(const CCoinsViewCache* v = nullptr, const std::vector<CTransactionRef>* blockTxs = nullptr) : cvc(v), bTxs(blockTxs){};
            valtype getSender(const COutPoint& out);
            opt_script getScript(const COutPoint& out);
        private:
            const CCoinsViewCache* cvc;
            const std::vector<CTransactionRef>* bTxs;

            opt_script tryFromBlock(const COutPoint& out); 
            opt_script tryFromGetTx(const COutPoint& out);
            opt_script tryFromCoins(const COutPoint& out);
        };

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
        

        using QtumTransactions = std::vector<QtumTransaction>;

        std::vector<valtype> GetStack(const CScript& scriptPubKey);
        
        class QtumTxConverter{
        
        public:
        
            QtumTxConverter(const CTransaction tx, const valtype _sender) : 
                txBit(tx), sender(_sender){}
        
            bool extractQtumTransactions(QtumTransactions& qtumTx);
        
            static bool trimStack(std::vector<valtype>&);
            
            static bool parseEthTXParams(QtumTransactionParams& params,const std::vector<valtype>& stack);
            static bool validateQtumTransactionParameters(QtumTransactionParams& params);
        private:
            QtumTransaction createEthTX(const QtumTransactionParams& etp, const uint32_t nOut);
            
            const CTransaction txBit;
            const valtype sender;
        };
        


        valtype ExtractAddress(const CScript&);
        uint256 WrapAddress(valtype in);
        valtype UnwrapAddress(uint256 in);

        struct QtumBlockchainDataFeed {
            struct BlockData {
                uint256 blockHash;
                uint256 blockCreator;
                uint64_t blockTime;
                uint32_t blockDifficulty;
            };

            std::vector<BlockData> prevBlocksData;
            BlockData currentBlockData;
            uint32_t currentBlockNum;
            uint64_t currentBlockGasLimit;

            QtumBlockchainDataFeed(CBlockIndex* lastIdx, const CBlock&, const uint64_t);

        private:
            BlockData extractData(const CBlock&);
            BlockData extractData(const CBlockIndex*);
        };
        
        struct ExecutionInfo {
            CScript fullSenderScript;
            uint256 senderAddress;
            std::vector<uint256> externalCallStack;
            valtype inputData;
            int64_t gasLimit;
            int64_t gasPrice;
            bool allowPaymentConsumption;
            uint256 toAddress;
            VersionVM version;
            CAmount outValue;
            const QtumBlockchainDataFeed& blockChainFeed;
        };
        
        struct ExecutionResult {
            uint64_t gasConsumed;
            uint32_t executionStatus;
            std::shared_ptr<const valtype> outputData;
            // const std::map<uint256, std::map<std::vector<uint8_t>, std::vector<uint8_t>>> storageWrites;
            const uint64_t paymentConsumed;
            
        };

        class QtumVMBase {
        protected:
            const QtumBlockchainDataFeed& chainFeed;
            dev::eth::Permanence type; //qtum temp
        public:
            QtumVMBase(const QtumBlockchainDataFeed& _chainFeed, 
                dev::eth::Permanence _type = dev::eth::Permanence::Committed) : chainFeed(_chainFeed), type(_type) {}
            virtual ResultExecute execute(const QtumTransaction&) = 0;
        };
        
        class QtumEVM : public QtumVMBase {
        public:
            QtumEVM(const QtumBlockchainDataFeed& dataFeed,
                dev::eth::Permanence type = dev::eth::Permanence::Committed) : QtumVMBase(dataFeed, type) {}
            ResultExecute execute(const QtumTransaction&) override;
            // ResultExecute execute(const ExecutionInfo&) override;
        };
    
    }
} // namespace qtum::vm