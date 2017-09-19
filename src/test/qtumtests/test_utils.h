#ifndef BITCOIN_TEST_TEST_UTILS_H
#define BITCOIN_TEST_TEST_UTILS_H

#include <util.h>
#include <testutil.h>
#include <validation.h>
#include <utilstrencodings.h>
#include <test/test_bitcoin.h>

extern std::unique_ptr<QtumState> globalState;

void initState();

CBlock generateBlock();

dev::Address createQtumAddress(dev::h256 hashTx, uint32_t voutNumber);

QtumTransaction createQtumTransaction(valtype data, dev::u256 value, dev::u256 gasLimit, dev::u256 gasPrice,
    dev::h256 hashTransaction, dev::Address recipient, int32_t nvout = 0);

std::pair<std::vector<ResultExecute>, ByteCodeExecResult> executeBC(std::vector<QtumTransaction> txs);

#endif // BITCOIN_TEST_TEST_UTILS_H
