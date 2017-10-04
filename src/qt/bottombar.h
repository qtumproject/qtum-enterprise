#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include <QWidget>
#include <QSize>
#include <QTabBar>
#include "walletmodel.h"

namespace Ui {
class BottomBar;
}
class WalletModel;
class TabBarInfo;

/**
 * @brief The BottomBar class Title bar widget
 */
class BottomBar : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief BottomBar Constructor
     * @param parent Parent widget
     */
    explicit BottomBar(QWidget *parent = 0);
    
    /**
     * @brief BottomBar Destrustor
     */
    ~BottomBar();

    /**
     * @brief setModel Set wallet model
     * @param _model Wallet model
     */
    void setModel(WalletModel *_model);

Q_SIGNALS:

public Q_SLOTS:
    /**
     * @brief setBalance Slot for changing the balance
     */
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake,
                    const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance, const CAmount& watchStake);

private:
    Ui::BottomBar *ui;
    WalletModel *model;
};

#endif // BOTTOMBAR_H
