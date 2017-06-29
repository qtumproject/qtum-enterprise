#include "createcontract.h"
#include "ui_createcontract.h"

QColor colorBackgroundTextEditIncorrect = Qt::red;
QColor colorBackgroundTextEditCorrect = Qt::white;

CreateContract::CreateContract(WalletModel* _walletModel, QWidget *parent) : QWidget(parent), 
    walletModel(_walletModel), ui(new Ui::CreateContract){
    ui->setupUi(this);

    scrollArea = NULL;

    ui->spinBoxGasLimit->setMaximum(INT32_MAX);
    ui->spinBoxGasLimit->setValue(DEFAULT_GAS_LIMIT);
    ui->doubleSpinBoxGasPrice->setMaximum(INT64_MAX);
    ui->doubleSpinBoxGasPrice->setValue(0.00001);

    ui->pushButtonDeploy->setEnabled(false);
    ui->comboBoxSelectContract->setEnabled(false);

    connect(ui->pushButtonDeploy, SIGNAL(clicked()), this, SLOT(deployContract()));
    connect(ui->textEditCode, SIGNAL(textChanged()), this, SLOT(updateCreateContractWidget()));
    connect(ui->textEditByteCode, SIGNAL(textChanged()), this, SLOT(enableComboBoxAndButtonDeploy()));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateCreateContractWidget()));
    connect(ui->comboBoxSelectContract, SIGNAL(currentIndexChanged(int)), this, SLOT(updateParams()));
    connect(ui->comboBoxSelectContract, SIGNAL(currentIndexChanged(int)), this, SLOT(enableComboBoxAndButtonDeploy()));
}

CreateContract::~CreateContract(){
    delete ui;
}

void CreateContract::updateCreateContractWidget() {
    deleteParameters();
    ui->pushButtonDeploy->setEnabled(false);
    ui->comboBoxSelectContract->setEnabled(false);
    compileSourceCode();
    fillingComboBoxSelectContract();
    enableComboBoxAndButtonDeploy();
}

void CreateContract::updateParams(){
    deleteParameters();
    if(ui->comboBoxSelectContract->count()){
        std::string key = ui->comboBoxSelectContract->currentText().toUtf8().constData();
        ParserAbi parser;
        parser.parseAbiJSON(byteCodeContracts[key].abi);
        currentContractMethods = parser.getContractMethods();
        createParameterFields();
    }
}

void CreateContract::enableComboBoxAndButtonDeploy(){
    if(ui->tabWidget->currentIndex() == 1 && ui->textEditByteCode->toPlainText().count()){
        ui->pushButtonDeploy->setEnabled(true);
        return;
    } else if(ui->tabWidget->currentIndex() == 1 && !ui->textEditByteCode->toPlainText().count()){
        ui->pushButtonDeploy->setEnabled(false);
        return;
    }

    if(ui->comboBoxSelectContract->count() != 0){
        if(checkTextEditsParams()){
            ui->pushButtonDeploy->setEnabled(true);
        } else {
            ui->pushButtonDeploy->setEnabled(false);
        }
        ui->comboBoxSelectContract->setEnabled(true);
    } else {
        ui->pushButtonDeploy->setEnabled(false);
        ui->comboBoxSelectContract->setEnabled(false);
    }
}

void CreateContract::compileSourceCode(){
    byteCodeContracts.clear();
    if(ui->tabWidget->currentIndex() == 0){
        std::string sCode = ui->textEditCode->toPlainText().toUtf8().constData();

        dev::solidity::CompilerStack compiler;
        compiler.addSource("", sCode);

        bool optimize = false;
        unsigned runs = 0;
        bool successful = false;
        try{            
            successful = compiler.compile(optimize, runs, std::map<std::string, dev::h160>());
        }catch(...){
            successful = false;
        }

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
        for(std::pair<std::string, Contract> contr : byteCodeContracts){
            ui->comboBoxSelectContract->addItem(QString::fromStdString(contr.first));
        }
    }
}

void CreateContract::deployContract(){
    std::string error;
    uint64_t nGasLimit = ui->spinBoxGasLimit->value();
    CAmount nGasPrice = ui->doubleSpinBoxGasPrice->value() * COIN;

    std::string bytecode;
    std::string key = ui->comboBoxSelectContract->currentText().toUtf8().constData();
    if(ui->tabWidget->currentIndex() == 0 && byteCodeContracts.count(key)){        
        bytecode = byteCodeContracts[key].code + parseParams();
    } else {
        bytecode = ui->textEditByteCode->toPlainText().toUtf8().constData();
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
    CContractInfo info(false, wtx.nTimeReceived, wtx.GetHash(), 0, ParseHex(createQtumAddress(wtx)), byteCodeContracts[key].abi);
    pwalletMain->addContractInfo(info);
}

void CreateContract::createParameterFields(){
    auto construct = ParserAbi::getConstructor(currentContractMethods);

    if(construct != NullContractMethod){
        scrollArea = new QScrollArea(this);
        scrollArea->setMaximumSize(250, 1000);

        QWidget *scrollWidget = new QWidget(scrollArea);
        QVBoxLayout *vLayout = new QVBoxLayout(scrollArea);
        vLayout->addStretch();

        for(size_t i = 0; i < construct.inputs.size(); i++){
            QLabel *label = new QLabel(QString::fromStdString(construct.inputs[i].name + " (" + construct.inputs[i].type + ")"), scrollArea);
            QLineEdit *textEdit = new QLineEdit(scrollArea);

            connect(textEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateTextEditsParams()));
            connect(textEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableComboBoxAndButtonDeploy()));
            
            QPalette palette;
            palette.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
            textEdit->setPalette(palette);

            textEdits.push_back(textEdit);

            vLayout->addWidget(label);
            vLayout->addWidget(textEdit);
        }

        scrollWidget->setLayout(vLayout);
        scrollArea->setWidget(scrollWidget);
        scrollArea->setWidgetResizable(true);
        ui->horizontalLayout->addWidget(scrollArea);
    }
}

void CreateContract::deleteParameters(){
    if(scrollArea != NULL){
        delete scrollArea;
        scrollArea = NULL;
    }
    textEdits.clear();
}

void CreateContract::updateTextEditsParams(){
    if(ui->comboBoxSelectContract->count()){
        ParserAbi parser;
        auto construct = parser.getConstructor(currentContractMethods);
        if(construct != NullContractMethod){
            for(size_t i = 0; i < construct.inputs.size(); i++){
                QPalette palette;
                if(parser.checkData(textEdits[i]->text().toStdString(), construct.inputs[i].type)){
                    palette.setColor(QPalette::Base, colorBackgroundTextEditCorrect);
                    textEdits[i]->setPalette(palette);
                } else {
                    palette.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
                    textEdits[i]->setPalette(palette);
                }
            }
        }
    }
}

bool CreateContract::checkTextEditsParams(){
    QPalette paletteTemp;
    paletteTemp.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
    for(QLineEdit* te : textEdits){
        QPalette palette = te->palette();
        if(paletteTemp == palette){
            return false;
        }
    }
    return true;
}

std::string CreateContract::parseParams(){
    std::string result = "";
    if(ui->comboBoxSelectContract->count()){
        Parameters params;
        ParserAbi parser;
        auto construct = parser.getConstructor(currentContractMethods);
        if(construct != NullContractMethod){
            for(size_t i = 0; i < construct.inputs.size(); i++){
                params.push_back(std::make_pair(construct.inputs[i].type, textEdits[i]->text().toStdString()));
            }
            result = parser.createInputData("", params);
        }
    }
    return result;
}
