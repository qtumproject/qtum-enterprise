#ifndef CONTRACTSINFO_H
#define CONTRACTSINFO_H

#include <QWidget>
#include <QStandardItemModel>
#include <walletmodel.h>

namespace Ui {
class ContractsInfo;
}

class ContractsInfo : public QWidget
{
    Q_OBJECT

public:
    explicit ContractsInfo(WalletModel *_walletModel, QWidget *parent = 0);
    ~ContractsInfo();

private:
    QStandardItemModel *model;
    WalletModel* walletModel;
    Ui::ContractsInfo *ui;
};

#endif // CONTRACTSINFO_H

