#include "contractsinfo.h"
#include "ui_contractsinfo.h"

ContractsInfo::ContractsInfo(WalletModel* _walletModel, QWidget *parent) :
    QWidget(parent), walletModel(_walletModel), ui(new Ui::ContractsInfo){
    ui->setupUi(this);
}

ContractsInfo::~ContractsInfo(){
    delete ui;
}

void ContractsInfo::test(){
    
}

void ContractsInfo::setWalletModel(WalletModel *model){
    this->walletModel = model;

    if(model){
        txTableModel = model->getTransactionTableModel();
        const QObject* tempd = reinterpret_cast<QObject*>(txTableModel);
        connect(tempd, SIGNAL(changedData()), this, SLOT(test()));

        // contracts
        ui->tableViewContractsInfo->setModel(model->getContractModel());
        ui->tableViewContractsInfo->setEditTriggers(QAbstractItemView::DoubleClicked);
        ui->tableViewContractsInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
        ui->tableViewContractsInfo->verticalHeader()->setVisible(false);

        // tokens
        ui->tableViewTokensInfo->setModel(model->getTokenModel());
        ui->tableViewTokensInfo->setEditTriggers(QAbstractItemView::DoubleClicked);
        ui->tableViewTokensInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
        ui->tableViewTokensInfo->verticalHeader()->setVisible(false);
    }
}
