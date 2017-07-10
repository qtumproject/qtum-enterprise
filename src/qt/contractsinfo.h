#ifndef CONTRACTSINFO_H
#define CONTRACTSINFO_H

#include <QWidget>
#include <QStandardItemModel>
#include <walletmodel.h>

#include "calldialog.h"

namespace Ui {
class ContractsInfo;
}

class ContractsInfo : public QWidget
{
    Q_OBJECT

public:
    explicit ContractsInfo(WalletModel *_walletModel, QWidget *parent = 0);
    ~ContractsInfo();

    void setWalletModel(WalletModel *walletModel);

public Q_SLOTS:

    void updateInfo();
    void test(QModelIndex index);

private:

    void updateContractModelAndTokenModel(CContractInfo& info, TransactionStatus::Status status);
    void updateContractsToDBWallet(CContractInfo& info);
    void updateConfirmContracts(CContractInfo& info);

    std::vector<std::string> createListMethods(CContractInfo& contractInfo);

    WalletModel *walletModel;
    TransactionTableModel *txTableModel;
    Ui::ContractsInfo *ui;
};

#endif // CONTRACTSINFO_H

