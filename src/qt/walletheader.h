#ifndef WALLETHEADER_H
#define WALLETHEADER_H

#include <QWidget>
#include "platformstyle.h"
#include "walletmodel.h"
#include "../amount.h"

namespace Ui {
class WalletHeader;
}

class WalletHeader : public QWidget
{
    Q_OBJECT

public:
    explicit WalletHeader(const PlatformStyle* style,QWidget *parent = 0);
    void setPlatformStyle(const PlatformStyle* style);
    void setWalletModel(WalletModel *walletModel);
    ~WalletHeader();
public Q_SLOTS:
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake, const CAmount&, const CAmount&, const CAmount&, const CAmount&);

private:
    CAmount currentBalance;
    CAmount currentUnconfirmedBalance;
    CAmount currentImmatureBalance;
    CAmount currentStake;
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    Ui::WalletHeader *ui;
private Q_SLOTS:
    void updateDisplayUnit();
};

#endif // WALLETHEADER_H
