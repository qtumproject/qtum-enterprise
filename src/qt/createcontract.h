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

extern QColor colorBackgroundTextEdit;
extern QColor colorBackgroundTextEditCorrect;

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
    void createParameterFields();
    void deleteParameters();
    bool checkTextEditsParams();
    std::string parseParams();

    std::vector<ContractMethod> currentContractMethods;
    std::map<std::string, Contract> byteCodeContracts;
    QScrollArea *scrollArea;
    std::vector<QLineEdit*> textEdits;
    WalletModel* walletModel;
    Ui::CreateContract *ui;
};

#endif // BITCOIN_QT_CREATECONTRACT_H
