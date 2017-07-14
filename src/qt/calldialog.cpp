#include "calldialog.h"
#include "ui_calldialog.h"
#include <analyzerERC20.h>
#include <libethcore/ABI.h>

extern QColor colorBackgroundTextEditIncorrect;
extern QColor colorBackgroundTextEditCorrect;

CallDialog::CallDialog(WalletModel* _walletModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CallDialog)
{
    ui->setupUi(this);

    walletModel = _walletModel;

    connect(ui->pushButtonSend, SIGNAL(clicked()), SLOT(callFunction()));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateActive()));
    connect(ui->senderLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(validateSender()));
}

CallDialog::~CallDialog()
{
    delete ui;
}

void CallDialog::setContractABI(std::string abi)
{
    contractABI = abi;
}

void CallDialog::setDataToScrollArea(std::vector<std::pair<std::string, std::string>> data){
    QVBoxLayout *gBoxLayout = new QVBoxLayout(this);
    ui->scrollAreaReadOfContract->setLayout(gBoxLayout);
    for(auto e: data)
    {
        QLabel* test = new QLabel(QString::fromStdString("<b>" + e.first + " :</b>\n" + e.second));
        test->setWordWrap(true);
        gBoxLayout->addWidget(test);    
    }
}

void CallDialog::setContractAddress(QString address){
    contractAddress = dev::Address(address.toStdString());
    ui->labelContractAddress->setText(address);
}

void CallDialog::validateSender()
{
    CBitcoinAddress senderAddress;
    QPalette palette;
    senderAddress.SetString(ui->senderLineEdit->text().toStdString());
    senderAddrValid = senderAddress.IsValid();
    if (!senderAddrValid)
    {
        palette.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
        ui->senderLineEdit->setPalette(palette); 
    }
    else
    {
        palette.setColor(QPalette::Base, colorBackgroundTextEditCorrect);
        ui->senderLineEdit->setPalette(palette);
    }
}

void CallDialog::setParameters()
{
    ContractMethod method(contractMethods[selectedMethod]);
    textEdits.clear();
    for(ContractMethodParams& cm : method.inputs){
        QWidget *scrollWidget = new QWidget(this);
        QVBoxLayout *vLayout = new QVBoxLayout(this);
        vLayout->addStretch();

        QLabel *label = new QLabel(QString::fromStdString(cm.name + " (" + cm.type + ")"), this);
        QLineEdit *textEdit = new QLineEdit(this);

        textEdits.push_back(textEdit);

        vLayout->addWidget(label);
        vLayout->addWidget(textEdit);

        scrollWidget->setLayout(vLayout);
        ui->scrollAreaWriteToContract->setWidget(scrollWidget);
        ui->scrollAreaWriteToContract->setWidgetResizable(true);
        connect(textEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateTextEditsParams()));
    }
}

void CallDialog::updateActive(){
    selectedMethod = ui->comboBox->currentIndex();
    setParameters();
}

void CallDialog::createWriteToContract(std::vector<ContractMethod>& methods){
    contractMethods = methods;
    for(auto s : methods){
        ui->comboBox->addItem(QString::fromStdString(s.name));
    }
    selectedMethod = 0;
    setParameters();
}

void CallDialog::callFunction(){
    std::string error;
    uint64_t nGasLimit = ui->spinBoxGasLimit->value();
    CAmount nGasPrice = ui->doubleSpinBoxGasPrice->value() * COIN;

    auto method = contractMethods[selectedMethod]; 
    std::string sig(method.name + "(");
    for (auto e: method.inputs)
    {
        sig += e.type + ", ";
    }
    sig.replace(sig.length() - 2, 2, ")");
    dev::FixedHash<4> hash(dev::keccak256(sig));

    ParserAbi parser;
    Parameters params;
    for(size_t i = 0; i < method.inputs.size(); i++){
        params.push_back(std::make_pair(method.inputs[i].type, textEdits[i]->text().toStdString()));
    }
    std::string result = hash.hex() + parser.createInputData("", params);


    LOCK2(cs_main, pwalletMain->cs_wallet);
    if (pwalletMain->IsLocked()){
        QMessageBox::critical(NULL, QObject::tr("Error"), tr("Please enter the wallet passphrase with walletpassphrase first."));
        return;
    }
    
    // Create TX_CREATE transaction
    CCoinControl coinControl;

    if(senderAddrValid){

        UniValue results(UniValue::VARR);
        std::vector<COutput> vecOutputs;

        coinControl.fAllowOtherInputs=true;

        assert(pwalletMain != NULL);
        pwalletMain->AvailableCoins(vecOutputs, false, NULL, true);

        BOOST_FOREACH(const COutput& out, vecOutputs) {

            CTxDestination address;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
            bool fValidAddress = ExtractDestination(scriptPubKey, address);

            CBitcoinAddress senderAddress;
            senderAddress.SetString(ui->senderLineEdit->text().toStdString());

            CBitcoinAddress destAdress(address);
            //use this weird !( == ) to avoid compilation errors on Ubuntu 14.04
            if (!fValidAddress || !(senderAddress == destAdress))
                continue;

            coinControl.Select(COutPoint(out.tx->GetHash(),out.i));

            break;

        }

        if(!coinControl.HasSelected()){
            QPalette palette;
            palette.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
            ui->senderLineEdit->setPalette(palette); 
        }
    }

    CReserveKey reservekey(pwalletMain);
    CWalletTx wtx(createTransactionOpCall(reservekey, coinControl, result, nGasLimit, nGasPrice, contractAddress, error));
    if(!error.empty()){
        QMessageBox::critical(NULL, QObject::tr("Error"), QString::fromStdString(error));
        return;
    }
    CValidationState st;
    if (!pwalletMain->CommitTransaction(wtx, reservekey, g_connman.get(), st)){
        QMessageBox::critical(NULL, QObject::tr("Error"), tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of the wallet and coins were spent in the copy but not marked as spent here."));
    }
}

void CallDialog::updateTextEditsParams(){
    auto method = contractMethods[selectedMethod]; 
    ParserAbi parser;
    for(size_t i = 0; i < method.inputs.size(); i++){
        QPalette palette;
        if(parser.checkData(textEdits[i]->text().toStdString(), method.inputs[i].type)){
            palette.setColor(QPalette::Base, colorBackgroundTextEditCorrect);
            textEdits[i]->setPalette(palette);
        } else {
            palette.setColor(QPalette::Base, colorBackgroundTextEditIncorrect);
            textEdits[i]->setPalette(palette);
        }
    }
}