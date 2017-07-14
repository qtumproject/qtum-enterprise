#include "createcontract.h"
#include "ui_createcontract.h"

QColor colorBackgroundTextEditIncorrect = QColor(253,229,231);
QColor colorBackgroundTextEditCorrect = Qt::white;

CreateContractPage::CreateContractPage(WalletModel* _walletModel, QWidget *parent) : QWidget(parent), 
    walletModel(_walletModel), ui(new Ui::CreateContractPage){
    ui->setupUi(this);

    scrollArea = NULL;

    ui->spinBoxGasLimit->setMaximum(INT32_MAX);
    ui->spinBoxGasLimit->setValue(DEFAULT_GAS_LIMIT * 5);
    ui->doubleSpinBoxGasPrice->setMaximum(INT64_MAX);
    ui->doubleSpinBoxGasPrice->setValue(0.00001);

    ui->pushButtonDeploy->setEnabled(false);
    ui->comboBoxSelectContract->setEnabled(false);

    connect(ui->pushButtonDeploy, SIGNAL(clicked()), this, SLOT(deployContract()));
    connect(ui->textEditCode, SIGNAL(textChanged()), this, SLOT(updateCreateContractWidget()));
    connect(ui->comboBoxSelectContract, SIGNAL(currentIndexChanged(int)), this, SLOT(updateParams()));
    // connect(ui->textEditByteCode, SIGNAL(textChanged()), this, SLOT(enableComboBoxAndButtonDeploy()));
    // connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateCreateContractWidget()));
    // connect(ui->comboBoxSelectContract, SIGNAL(currentIndexChanged(int)), this, SLOT(enableComboBoxAndButtonDeploy()));
}

CreateContractPage::~CreateContractPage(){
    delete ui;
}

void CreateContractPage::setWalletModel(WalletModel *model){
    this->walletModel = model;
}
void CreateContractPage::updateCreateContractWidget() {
    static QString strCache;
    if (strCache == ui->textEditCode->toPlainText()) return;
    strCache = ui->textEditCode->toPlainText();

    deleteParameters();
    if (compileSourceCode())
    {
        fillingComboBoxSelectContract();
        ui->comboBoxSelectContract->setEnabled(true);
        ui->pushButtonDeploy->setEnabled(true);
        updateParams();
    }
}
void CreateContractPage::updateParams(){
        std::string key = ui->comboBoxSelectContract->currentText().toUtf8().constData();
        ParserAbi parser;
        parser.parseAbiJSON(byteCodeContracts[key].abi);
        currentContractMethods = parser.getContractMethods();

        if(scrollArea != NULL){
            delete scrollArea;
            scrollArea = NULL;
        }
        textEdits.clear();
        createParameterFields();
        ui->textEditByteCode->setText(QString::fromStdString(byteCodeContracts[key].code));
}

bool CreateContractPage::compileSourceCode(){
    std::string sCode = ui->textEditCode->toPlainText().toUtf8().constData();

    dev::solidity::CompilerStack compiler;
    compiler.addSource("", sCode);

    bool optimize = false;
    unsigned runs = 0;
    std::vector<std::string> contracts;
    try{            
        compiler.compile(optimize, runs, std::map<std::string, dev::h160>());
        contracts = compiler.contractNames();
    } catch(...) {
        return false;
    }
    
    for(std::string s : contracts){
        Contract contr;
        std::string key(s.begin() + 1, s.end());
        contr.code = compiler.object(s).toHex();
        contr.abi = dev::jsonCompactPrint(compiler.contractABI(s));
        byteCodeContracts[key] = contr;
    }

    return !byteCodeContracts.empty();
}

void CreateContractPage::fillingComboBoxSelectContract(){
    for(std::pair<std::string, Contract> contr : byteCodeContracts){
        ui->comboBoxSelectContract->addItem(QString::fromStdString(contr.first));
    }
}

void CreateContractPage::deployContract(){
    std::string error;
    uint64_t nGasLimit = ui->spinBoxGasLimit->value();
    CAmount nGasPrice = ui->doubleSpinBoxGasPrice->value() * COIN;

    bool token = false;
    std::string bytecode;
    std::string key = ui->comboBoxSelectContract->currentText().toUtf8().constData();
    if(byteCodeContracts.count(key)){        
        bytecode = byteCodeContracts[key].code + parseParams();
        AnalyzerERC20 erc;
        token = erc.isERC20(byteCodeContracts[key].abi);
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
    CContractInfo info(CContractInfo::DeployStatus::CONFIRMING, token, GetAdjustedTime(), wtx.GetHash(), 0, ParseHex(createQtumAddress(wtx)), byteCodeContracts[key].abi);
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
    Q_EMIT clickedDeploy();
}

void CreateContractPage::createParameterFields(){
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

void CreateContractPage::deleteParameters(){
    byteCodeContracts.clear();
    ui->textEditByteCode->clear();
    ui->pushButtonDeploy->setEnabled(false);
    ui->comboBoxSelectContract->setEnabled(false);
    ui->comboBoxSelectContract->clear();
}

void CreateContractPage::updateTextEditsParams(){
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

std::string CreateContractPage::parseParams(){
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
