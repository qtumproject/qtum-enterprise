#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QSize>
#include <QTabBar>
#include "walletmodel.h"

namespace Ui {
class TitleBar;
}
class WalletModel;
class TabBarInfo;

/**
 * @brief The TitleBar class Title bar widget
 */
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief TitleBar Constructor
     * @param parent Parent widget
     */
    explicit TitleBar(const PlatformStyle *platformStyle, QWidget *parent = 0);
    
    /**
     * @brief TitleBar Destrustor
     */
    ~TitleBar();

    /**
     * @brief setTabBarInfo Set the tab bar info
     * @param info Information about tabs
     */
    void setTabBarInfo(QObject* info);

Q_SIGNALS:
    void on_settings_clicked();
public Q_SLOTS:

    /**
     * @brief on_navigationResized Slot for changing the size of the navigation bar
     * @param _size Size of the navigation bar
     */
    void on_navigationResized(const QSize& _size);

private:
    Ui::TitleBar *ui;
    TabBarInfo* m_tab;
    const PlatformStyle  *style;
};

#endif // TITLEBAR_H
