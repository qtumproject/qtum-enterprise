// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATECONTRACT_H
#define BITCOIN_QT_CREATECONTRACT_H

#include "amount.h"

#include <QWidget>
#include <memory>
#include <QMessageBox>

#include "libsolidity/interface/CompilerStack.h"
#include <walletmodel.h>
#include "validation.h"
#include "consensus/validation.h"
#include "net.h"
#include "base58.h"

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
    void enableButtonSourceCode();
    void enableButtonByteCode();
    void enableComboBoxAndButtonDeploy();
    void compileSourceCode();
    void fillingComboBoxSelectContract();
    void deployContract();

private:
    QString createDeployInfo(CWalletTx& wtx);

    bool typeCode;
    QString sourceCode;
    QString byteCode;
    std::map<std::string, std::string> byteCodeContracts;

    WalletModel* walletModel;
    Ui::CreateContract *ui;
};

#endif // BITCOIN_QT_CREATECONTRACT_H
