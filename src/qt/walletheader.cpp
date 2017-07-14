#include "walletheader.h"
#include "ui_walletheader.h"
#include "optionsmodel.h"

WalletHeader::WalletHeader(const PlatformStyle *style, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WalletHeader)
{
    ui->setupUi(this);
    platformStyle=style;
}
void WalletHeader::setPlatformStyle(const PlatformStyle *style){
    platformStyle=style;
}
void WalletHeader::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getStake(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getWatchStake());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}
void WalletHeader::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentStake,
                       0, 0, 0, 0);
    }
}
void WalletHeader::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake, const CAmount& , const CAmount& , const CAmount& , const CAmount& )
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentStake = stake;
    int total = currentBalance+currentUnconfirmedBalance+currentImmatureBalance+currentStake;

    ui->labelAmout->setText(BitcoinUnits::formatWithUnit(unit, total, false, BitcoinUnits::separatorAlways));
    ui->labelAmout_dolar->setText(BitcoinUnits::formatWithUnit(BitcoinUnits::CNY, currentBalance, false, BitcoinUnits::separatorAlways));


}
WalletHeader::~WalletHeader()
{
    delete ui;
}
