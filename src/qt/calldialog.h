#ifndef CALLDIALOG_H
#define CALLDIALOG_H

#include <QDialog>
#include <QLineEdit>

class ContractMethodParams;

namespace Ui {
class CallDialog;
}

class CallDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CallDialog(QWidget *parent = 0);
    ~CallDialog();

    void setContractAddress(QString address);
    void setDataToComboBox(std::vector<std::string>& methods);

    void createWriteToContract(std::vector<ContractMethodParams>& inputs);

private:
    Ui::CallDialog *ui;
};

#endif // CALLDIALOG_H
