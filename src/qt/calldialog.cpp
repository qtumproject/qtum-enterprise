#include "calldialog.h"
#include "ui_calldialog.h"
#include <analyzerERC20.h>

CallDialog::CallDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CallDialog)
{
    ui->setupUi(this);

    connect(ui->pushButtonSend, SIGNAL(clicked()), SLOT(accept()));
}

CallDialog::~CallDialog()
{
    delete ui;
}

void CallDialog::setContractAddress(QString address){
    ui->labelContractAddress->setText(address);
}

void CallDialog::setDataToComboBox(std::vector<std::string>& methods){
    for(std::string& s : methods){
        ui->comboBox->addItem(QString::fromStdString(s));
    }
}

void CallDialog::createWriteToContract(std::vector<ContractMethodParams>& inputs){
    QWidget *scrollWidget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addStretch();
    
    for(ContractMethodParams& cm : inputs){
        QLabel *label = new QLabel(QString::fromStdString(cm.name + " (" + cm.type + ")"), this);
        QLineEdit *textEdit = new QLineEdit(this);

        vLayout->addWidget(label);
        vLayout->addWidget(textEdit);
    }

    scrollWidget->setLayout(vLayout);
    ui->scrollAreaWriteToContract->setWidget(scrollWidget);
    ui->scrollAreaWriteToContract->setWidgetResizable(true);
}
