#ifndef WALLETHEADER_H
#define WALLETHEADER_H

#include <QWidget>

namespace Ui {
class WalletHeader;
}

class WalletHeader : public QWidget
{
    Q_OBJECT

public:
    explicit WalletHeader(QWidget *parent = 0);
    ~WalletHeader();

private:
    Ui::WalletHeader *ui;
};

#endif // WALLETHEADER_H
