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
    class CreateContractPage;
}

class CreateContractPage : public QWidget
{
    Q_OBJECT

public:
    explicit CreateContractPage(WalletModel* _walletModel, QWidget* parent = 0);
    ~CreateContractPage();

    void setWalletModel(WalletModel *walletModel);

private Q_SLOTS:
    void updateParams();
    void updateCreateContractWidget();
    bool compileSourceCode();
    void fillingComboBoxSelectContract();
    void deployContract();
    void updateTextEditsParams();

Q_SIGNALS:

    void clickedDeploy();

private:
    QString createDeployInfo(CWalletTx& wtx);
    void createParameterFields();
    void deleteParameters();
    std::string parseParams();

    std::vector<ContractMethod> currentContractMethods;
    std::map<std::string, Contract> byteCodeContracts;
    QScrollArea *scrollArea;
    std::vector<QLineEdit*> textEdits;
    WalletModel* walletModel;
    Ui::CreateContractPage *ui;
};

#endif // BITCOIN_QT_CREATECONTRACT_H
