#include "bottombar.h"
#include "ui_bottombar.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "tabbarinfo.h"

#include <QPixmap>

namespace BottomBar_NS {
const int logoWidth = 135;
}
using namespace BottomBar_NS;

BottomBar::BottomBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BottomBar)
{
    ui->setupUi(this);
    // Hide the fiat balance label
    ui->lblFiatBalance->hide();
    // Set size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

BottomBar::~BottomBar()
{
    delete ui;
}

void BottomBar::setModel(WalletModel *_model)
{
    this->model = _model;

    setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),  model->getStake(),
               model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getWatchStake());

    connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
}

void BottomBar::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance, const CAmount& watchStake)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(stake);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);
    Q_UNUSED(watchStake);

    if(model && model->getOptionsModel())
    {
        ui->lblBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}
