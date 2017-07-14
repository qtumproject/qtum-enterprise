#include "contractsinfo.h"
#include "ui_contractsinfo.h"
#include "transactionrecord.h"
#include <analyzerERC20.h>
#include <libethcore/ABI.h>

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

        connect(ui->tableViewContractsInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(contractSelected(QModelIndex)));
        connect(ui->tableViewTokensInfo, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(tokenSelected(QModelIndex)));
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

    // Get the contract
    auto contract = pwalletMain->mapContractInfo.find(ParseHex(stdAddres));
    CContractInfo contractInfo = contract->second;

    if(contractInfo.getStatus() == "Created"){

        ParserAbi parser;
        parser.parseAbiJSON(contractInfo.getAbi());
        std::vector<ContractMethod> methods = parser.getContractMethods();
        
        std::vector<std::pair<std::string, std::string>> staticCalls; 
        for (auto e : methods)
        {
            if (!e.constant) continue;
            if (!e.inputs.empty()) continue;
            if (e.outputs.empty()) continue;
            std::string sig(e.name+"()");
            dev::FixedHash<4> hash(dev::keccak256(sig));
            auto signature = hash.asBytes();

            dev::u256 gasPrice = 1;
            dev::u256 gasLimit(10000000); // MAX_MONEY
            dev::Address senderAddress("f1b0747fe29c1fe5d4ff1e63cefdbdeaae1329d6");
            
            CBlock block;
            CMutableTransaction tx;
            tx.vout.push_back(CTxOut(0, CScript() << OP_DUP << OP_HASH160 << senderAddress.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG));
            block.vtx.push_back(MakeTransactionRef(CTransaction(tx)));
        
            QtumTransaction callTransaction(0, gasPrice, gasLimit, dev::Address(stdAddres), signature, dev::u256(0));
            callTransaction.forceSender(senderAddress);

            ByteCodeExec exec(block, std::vector<QtumTransaction>(1, callTransaction));
            exec.performByteCode(dev::eth::Permanence::Reverted);
            std::vector<ResultExecute> execResults = exec.getResult();
            auto out(execResults[0].execRes.output);

            if (e.outputs[0].type == "address") {
                staticCalls.push_back(std::make_pair(e.name, "0x" + dev::Address(dev::eth::abiOut<dev::u160>(out)).hex()));
            } else if (e.outputs[0].type == "string"){
                staticCalls.push_back(std::make_pair(e.name, dev::eth::abiOut<std::string>(out)));
            } else {
                std::stringstream ss;
                if (e.outputs[0].type.substr(0,3) == "int"){
                    ss << dev::u2s(dev::eth::abiOut<dev::u256>(out));
                } else {
                    ss << dev::eth::abiOut<dev::u256>(out);
                }
                staticCalls.push_back(std::make_pair(e.name, ss.str()));
            }
        }

        // Get the contract methods
        // std::vector<std::string> listMethods();

        // Create dialog window
        CallDialog* dialog = new CallDialog(walletModel);
        dialog->setContractAddress(QString("0x") + address);
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