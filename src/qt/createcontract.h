// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATECONTRACT_H
#define BITCOIN_QT_CREATECONTRACT_H

#include "amount.h"

#include <QWidget>
#include <memory>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QPalette>

#include "libsolidity/interface/CompilerStack.h"
#include "libdevcore/JSON.h"

#include <walletmodel.h>
#include "analyzerERC20.h"

#include "validation.h"
#include "consensus/validation.h"
#include "net.h"
#include "base58.h"

// struct Data{
//     std::vector<uint8> uint8Values;
//     std::vector<uint16> uint8Values;
//     std::vector<uint32> uint32Values;
//     std::vector<uint64> uint64Values;
//     std::vector<uint128> uint128Values;
//     std::vector<uint256> uint256Values;

//     std::vector<int8> int8Values;
//     std::vector<int16> int8Values;
//     std::vector<int32> int32Values;
//     std::vector<int64> int64Values;
//     std::vector<int128> int128Values;
//     std::vector<int256> int256Values;

//     std::vector<std::string> addressValues;
//     std::vector<std::string> stringValues;

//     std::vector<bool> boolValues;
// };

struct Contract{
    std::string code = "";
    std::string abi = "";
};

namespace Ui {
    class CreateContract;
}

class CreateContract : public QWidget
{
    Q_OBJECT

public:
    explicit CreateContract(WalletModel* _walletModel, QWidget* parent = 0);
    ~CreateContract();

private Q_SLOTS:
    void updateParams();
    void updateCreateContractWidget();
    void enableComboBoxAndButtonDeploy();
    void compileSourceCode();
    void fillingComboBoxSelectContract();
    void deployContract();

    void updateTextEditsParams();

private:
    QString createDeployInfo(CWalletTx& wtx);
    void createParameterFields(std::string abiStr);
    void deleteParameters();

    std::map<std::string, Contract> byteCodeContracts;

    QScrollArea *scrollArea;
    std::vector<QLineEdit*> textEdits;

    WalletModel* walletModel;
    Ui::CreateContract *ui;
};

#endif // BITCOIN_QT_CREATECONTRACT_H
