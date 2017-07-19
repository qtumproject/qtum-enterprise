#include "contractsinfo.h"
#include "ui_contractsinfo.h"
#include <QTextEdit>
#include <QCheckBox>

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

void ContractsInfo::showWatchDialog(){
    auto dialog = new QDialog(this);
    QVBoxLayout * mainLayout = new QVBoxLayout(dialog);
    QLineEdit * lEditAddress = new QLineEdit();

    lEditAddress->setPlaceholderText(tr("Contract address"));
    lEditAddress->setMinimumWidth(400);

    mainLayout->addWidget(lEditAddress);

    QTextEdit * lEditABI = new QTextEdit();

    lEditABI->setPlaceholderText(tr("Contract ABI"));
    lEditABI->setMinimumWidth(400);
    lEditABI->setMinimumHeight(800);

    mainLayout->addWidget(lEditABI);
    
    QCheckBox * chbIsContract = new QCheckBox(tr("Is contract"), this);
    mainLayout->addWidget(chbIsContract);

    QPushButton * btnOk = new QPushButton(tr("OK"), dialog);
    btnOk->setMinimumWidth(400);

    mainLayout->addWidget(btnOk);

    dialog->resize(410, 810);
    dialog->setLayout(mainLayout);
    btnOk->setAutoDefault(true);

    connect(chbIsContract, &QCheckBox::clicked, dialog, 
        [=](){ lEditABI->setEnabled(!chbIsContract->isChecked()); });

    connect(btnOk, SIGNAL(clicked()), dialog, SLOT(accept()));
    if (dialog->exec() == QDialog::Accepted)
    {
        std::string abi = chbIsContract->isChecked() ? ERC20 :
            lEditABI->toPlainText().toStdString();
        std::string address = lEditAddress->text().toStdString();
        if (!globalState->addressInUse(dev::Address(ParseHex(address)))) return;
        AnalyzerERC20 erc;
        CContractInfo info(CContractInfo::DeployStatus::WATCHED,  erc.isERC20(abi), GetAdjustedTime(), uint256(), 0, ParseHex(address), abi);
        pwalletMain->addContractInfo(info);

        if(walletModel){
            QStringList data = walletModel->createDataForTokensAndContractsModel(info);
            if(!info.isToken() && !data.empty()){
                walletModel->addContractToContractModel(data);
            } else if(info.isToken() && !data.empty()){
                walletModel->addTokenToTokenModel(data);
            }
            walletModel->addConfirmContract(info);
        }
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

        connect(ui->tableViewContractsInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(contractSelected(QModelIndex)));
        connect(ui->tableViewTokensInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(tokenSelected(QModelIndex)));
        connect(ui->pushButtonWatch, SIGNAL(clicked()), this, SLOT(showWatchDialog()));
    }
}





std::vector<ContractMethod> ContractsInfo::createListMethods(CContractInfo contractInfo){
    std::vector<ContractMethod> result;
    ParserAbi parser;
    parser.parseAbiJSON(contractInfo.getAbi());
    std::vector<ContractMethod> methods = parser.getContractMethods();
    for(ContractMethod cm : methods){
        if(cm.type == "function" && !cm.constant)
            result.push_back(cm);
    }
    return result;
}


void ContractsInfo::showContractInterface(QString address){
    std::string stdAddres = address.toUtf8().constData();
    auto contract = pwalletMain->mapContractInfo.find(ParseHex(stdAddres));
    CContractInfo contractInfo = contract->second;

    if(contractInfo.getStatus() == "Created" 
        || contractInfo.getStatus() == "Watched"){

        ParserAbi parser;
        parser.parseAbiJSON(contractInfo.getAbi());
        std::vector<ContractMethod> methods = parser.getContractMethods();
        
        std::vector<std::pair<ContractMethod, std::string>> staticCalls;

        for (auto& e : methods)
        {
            if (!e.constant) continue;
            if (e.outputs.empty()) continue;
            staticCalls.push_back({e, std::string()});
        }

        for(auto& e : staticCalls)
        {
            auto res =  PerformStaticCall(e.first, stdAddres);
            e.second = ParseExecutionResult(e.first.outputs[0], res);
        }

        // Create dialog window
        CallDialog* dialog = new CallDialog(walletModel);
        dialog->setContractAddress(address);
        dialog->setDataToScrollArea(staticCalls);
        auto tmp(createListMethods(contractInfo));
        if (!tmp.empty())
        {
            dialog->createWriteToContract(tmp);
        }
        dialog->exec();
        
        
        delete dialog;
    }
}

void ContractsInfo::tokenSelected(QModelIndex index){
        
    // Get the address
    int row = index.row();
    QStandardItem* item1 = walletModel->getTokenModel()->item(row, 3);
    QString address = item1->data(Qt::DisplayRole).toString();
    showContractInterface(address);
}

void ContractsInfo::contractSelected(QModelIndex index){
    
    // Get the address
    int row = index.row();
    QStandardItem* item1 = walletModel->getContractModel()->item(row, 3);
    QString address = item1->data(Qt::DisplayRole).toString();
    showContractInterface(address);
}