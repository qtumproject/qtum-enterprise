#ifndef CALLDIALOG_H
#define CALLDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include <walletmodel.h>

#include "validation.h"
#include "consensus/validation.h"
#include "net.h"
#include "base58.h"

struct ContractMethod;

namespace Ui {
class CallDialog;
}

class CallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CallDialog(WalletModel* _walletModel, QWidget *parent = 0);
    ~CallDialog();
    
    void setContractABI(std::string abi);
    void setContractAddress(QString address);
    void setDataToScrollArea(std::vector<std::pair<std::string, std::string>>);
    void createWriteToContract(std::vector<ContractMethod>& methods);

public Q_SLOTS:

    void callFunction();
    void validateSender();
    void updateActive();
    void updateTextEditsParams();

private:
    Ui::CallDialog *ui;

    bool senderAddrValid;
    dev::Address contractAddress;
    std::string contractABI;
    std::vector<QLineEdit*> textEdits;
    std::vector<ContractMethod> contractMethods;
    uint32_t selectedMethod;
    void setParameters();

    WalletModel* walletModel;
};

#endif // CALLDIALOG_H
