#include "createcontract.h"
#include "ui_createcontract.h"

CreateContract::CreateContract(WalletModel* _walletModel, QWidget *parent) : QWidget(parent), ui(new Ui::CreateContract), 
    walletModel(_walletModel){
    ui->setupUi(this);

    typeCode = false;

    ui->pushButtonSourceCode->setEnabled(false);
    ui->pushButtonByteCode->setEnabled(true);
    ui->pushButtonDeploy->setEnabled(false);
    ui->comboBoxSelectContract->setEnabled(false);
    // ui->pushButtonSourceCode->setDefault(true);
    // ui->pushButtonByteCode->setDefault(true);

    connect(ui->pushButtonSourceCode, SIGNAL(clicked()), this, SLOT(enableButtonByteCode()));
    connect(ui->pushButtonByteCode, SIGNAL(clicked()), this, SLOT(enableButtonSourceCode()));
    connect(ui->pushButtonDeploy, SIGNAL(clicked()), this, SLOT(deployContract()));

    connect(ui->textEditCode, SIGNAL(textChanged()), this, SLOT(compileSourceCode()));
    connect(ui->textEditCode, SIGNAL(textChanged()), this, SLOT(fillingComboBoxSelectContract()));
    connect(ui->textEditCode, SIGNAL(textChanged()), this, SLOT(enableComboBoxAndButtonDeploy()));
}

CreateContract::~CreateContract(){
    delete ui;
}

void CreateContract::enableButtonSourceCode() {
    byteCode = ui->textEditCode->toPlainText();

    ui->pushButtonSourceCode->setEnabled(true);
    ui->pushButtonByteCode->setEnabled(false);
    ui->textEditCode->clear();
    typeCode = true;

    if(sourceCode.count()){
        ui->textEditCode->setText(sourceCode);
    }
}

void CreateContract::enableButtonByteCode() {
    sourceCode = ui->textEditCode->toPlainText();

    ui->pushButtonSourceCode->setEnabled(false);
    ui->pushButtonByteCode->setEnabled(true);
    ui->textEditCode->clear();
    typeCode = false;

    if(byteCode.count()){
        ui->textEditCode->setText(byteCode);
    }
}

void CreateContract::enableComboBoxAndButtonDeploy(){
    if(typeCode && ui->textEditCode->toPlainText().count()){
        ui->pushButtonDeploy->setEnabled(true);
        return;
    } else if(typeCode && !ui->textEditCode->toPlainText().count()){
        ui->pushButtonDeploy->setEnabled(false);
        return;
    }

    if(ui->comboBoxSelectContract->count() != 0){
        ui->pushButtonDeploy->setEnabled(true);
        ui->comboBoxSelectContract->setEnabled(true);
    } else {
        ui->pushButtonDeploy->setEnabled(false);
        ui->comboBoxSelectContract->setEnabled(false);
    }
}

void CreateContract::compileSourceCode(){
    byteCodeContracts.clear();
    if(!typeCode){
        std::string sCode = ui->textEditCode->toPlainText().toUtf8().constData();

        dev::solidity::CompilerStack compiler;
        compiler.addSource("", sCode);

        bool optimize = false;
        unsigned runs = 0;
        bool successful = compiler.compile(optimize, runs, std::map<std::string, dev::h160>());

        // if(successful){
        //     std::vector<std::string> contracts = compiler.contractNames();
        //     for(std::string s : contracts){
        //         std::string key(s.begin() + 1, s.end());
        //         byteCodeContracts[key] = compiler.object(s).toHex();
        //     }
        // }
        if(successful){
            std::vector<std::string> contracts = compiler.contractNames();
            for(std::string s : contracts){
                Contract contr;
                std::string key(s.begin() + 1, s.end());
                contr.code = compiler.object(s).toHex();
                contr.abi = dev::jsonCompactPrint(compiler.contractABI(s));
                byteCodeContracts[key] = contr;
            }
        }
    }
}

void CreateContract::fillingComboBoxSelectContract(){
    ui->comboBoxSelectContract->clear();
    if(!byteCodeContracts.empty()){
        // for(std::pair<std::string, std::string> contr : byteCodeContracts){
        //     ui->comboBoxSelectContract->addItem(QString::fromStdString(contr.first));
        // }
        for(std::pair<std::string, Contract> contr : byteCodeContracts){
            ui->comboBoxSelectContract->addItem(QString::fromStdString(contr.first));
        }
    }
}

void CreateContract::deployContract(){
    std::string error;
    uint64_t nGasLimit = DEFAULT_GAS_LIMIT;
    CAmount nGasPrice = DEFAULT_GAS_PRICE;

    std::string bytecode;
    std::string key = ui->comboBoxSelectContract->currentText().toUtf8().constData();
    if(!typeCode && byteCodeContracts.count(key)){
        // bytecode = byteCodeContracts[key];
        bytecode = byteCodeContracts[key].code;
    } else {
        bytecode = ui->textEditCode->toPlainText().toUtf8().constData();
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);
    if (pwalletMain->IsLocked()){
        QMessageBox::critical(NULL, QObject::tr("Error"), tr("Please enter the wallet passphrase with walletpassphrase first."));

        return;
    }
    
    // Create TX_CREATE transaction
    CCoinControl coinControl;
    CReserveKey reservekey(pwalletMain);
    CWalletTx wtx(createTransactionOpCreate(reservekey, coinControl, bytecode, nGasLimit, nGasPrice, error));
    if(!error.empty()){
        QMessageBox::critical(NULL, QObject::tr("Error"), QString::fromStdString(error));
        return;
    }

    CValidationState st;
    if (!pwalletMain->CommitTransaction(wtx, reservekey, g_connman.get(), st)){
        QMessageBox::critical(NULL, QObject::tr("Error"), tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of the wallet and coins were spent in the copy but not marked as spent here."));
        return;
    }


    CContractInfo info(false, wtx.nTimeReceived, wtx.GetHash(), 0, ParseHex(createQtumAddress(wtx)), std::string());
    pwalletMain->addContractInfo(info);
}
