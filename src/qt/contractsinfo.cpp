#include "contractsinfo.h"
#include "ui_contractsinfo.h"
#include "transactionrecord.h"

ContractsInfo::ContractsInfo(WalletModel* _walletModel, QWidget *parent) :
    QWidget(parent), walletModel(_walletModel), ui(new Ui::ContractsInfo){
    ui->setupUi(this);
}

ContractsInfo::~ContractsInfo(){
    delete ui;
}

void ContractsInfo::updateInfo(){
    std::vector<CContractInfo> data = walletModel->getConfirmContracts();
    for(size_t i = 0; i < data.size(); i++){
        uint256 hash = data[i].getHashTx();
        updateContractModelAndTokenModel(data[i], walletModel->getTransactionTableModel()->getStatusTx(hash).status);
        updateContractsToDBWallet(data[i]);
        updateConfirmContracts(data[i]);
    }
}

void ContractsInfo::updateContractModelAndTokenModel(CContractInfo& info, TransactionStatus::Status status){

    if(status == TransactionStatus::Status::Confirming || status == TransactionStatus::Status::Unconfirmed){
        return;
    } else if(status == TransactionStatus::Status::Confirmed){
        bool inUse = globalState->addressInUse(dev::Address(info.getAddressContract()));
        info.setStatus(inUse ? CContractInfo::DeployStatus::CREATED : CContractInfo::DeployStatus::NOT_CREATED);
    } else if(status != TransactionStatus::Status::Confirmed && status != TransactionStatus::Status::Confirming){
        info.setStatus(CContractInfo::DeployStatus::NOT_CREATED);
    }

    QStandardItemModel* model;
    model = info.isToken() ? walletModel->getTokenModel() : walletModel->getContractModel();
    QString address = QString::fromStdString(dev::Address(info.getAddressContract()).hex());

    QStandardItem temp(address);
    size_t rows = model->rowCount();
    for(size_t i = 0; i < rows; i++){
        QStandardItem* item1 = model->item(i, 3);
        if(temp.text() == item1->text()){
            QStandardItem* item2 = model->item(i, 0);
            item2->setText(QString::fromStdString(info.getStatus()));
            return;
        }
    }
}

void ContractsInfo::updateContractsToDBWallet(CContractInfo& info){
    if(info.getStatus() != "Confirming"){
        pwalletMain->eraseContractInfo(info.getAddressContract());
        pwalletMain->addContractInfo(info);
    }
}

void ContractsInfo::updateConfirmContracts(CContractInfo& info){
    if(info.getStatus() != "Confirming"){
        walletModel->eraseConfirmContract(info.getAddressContract());
    }
}

void ContractsInfo::setWalletModel(WalletModel *model){
    this->walletModel = model;

    if(model){
        txTableModel = model->getTransactionTableModel();
        const QObject* tempd = reinterpret_cast<QObject*>(txTableModel);
        connect(tempd, SIGNAL(changedData()), this, SLOT(updateInfo()));

        // contracts
        ui->tableViewContractsInfo->setModel(model->getContractModel());
        ui->tableViewContractsInfo->setAlternatingRowColors(true);
        ui->tableViewContractsInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableViewContractsInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
        ui->tableViewContractsInfo->verticalHeader()->setVisible(false);

        // tokens
        ui->tableViewTokensInfo->setModel(model->getTokenModel());
        ui->tableViewTokensInfo->setAlternatingRowColors(true);
        ui->tableViewTokensInfo->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableViewTokensInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
        ui->tableViewTokensInfo->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
        ui->tableViewTokensInfo->verticalHeader()->setVisible(false);

        connect(ui->tableViewContractsInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(test(QModelIndex)));
        connect(ui->tableViewTokensInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(test(QModelIndex)));
    }
}
