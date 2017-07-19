#include "calldialog.h"
#include "ui_calldialog.h"
#include <analyzerERC20.h>
#include <libethcore/ABI.h>
#include "guiutil.h"
#include "guiconstants.h"

extern QColor colorBackgroundTextEditIncorrect;
extern QColor colorBackgroundTextEditCorrect;

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

CallDialog::CallDialog(WalletModel* _walletModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CallDialog)
{
    ui->setupUi(this);

    walletModel = _walletModel;

#ifndef USE_QRCODE
    ui->lblQRCode->setVisible(false);
#endif

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

void CallDialog::textChangedOnReadContract(){
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
    QLabel * label = mapEditLabel[lineEdit];

    auto res =  PerformStaticCall(mapEditMethod[lineEdit], contractAddress.hex(), {lineEdit->text().toStdString()});
    label->setText(QString::fromStdString(ParseExecutionResult(mapEditMethod[lineEdit].outputs[0], res)));
}

void CallDialog::setDataToScrollArea(std::vector<std::pair<ContractMethod, std::string>>& data){
    QVBoxLayout *gBoxLayout = new QVBoxLayout(this);
    ui->scrollAreaReadOfContract->setLayout(gBoxLayout);
    for(auto e: data)
    {
        if (!e.first.inputs.empty())
        {
            QHBoxLayout * hBoxLayout = new QHBoxLayout(this);
            QLabel* name = new QLabel(QString::fromStdString("<b>" + e.first.name + " : </b>"), this);   
            QLineEdit *textEdit = new QLineEdit(this);
            QLabel* value = new QLabel(this);   
            mapEditLabel[textEdit] = value;
            mapEditMethod[textEdit] = e.first;

            textEdit->setMaximumWidth(180);

            gBoxLayout->addWidget(name);  
            hBoxLayout->addWidget(textEdit);  
            hBoxLayout->addWidget(value);  

            gBoxLayout->addLayout(hBoxLayout);

            connect(textEdit, SIGNAL(textChanged(const QString &)), this, SLOT(textChangedOnReadContract()));
            continue;
        }
        QLabel* test = new QLabel(QString::fromStdString("<b>" + e.first.name + " :</b>\n" + e.second));
        test->setWordWrap(true);
        test->setTextInteractionFlags(Qt::TextSelectableByMouse);
        gBoxLayout->addWidget(test);    
    }
}

void CallDialog::setContractAddress(QString address){
    contractAddress = dev::Address(address.toStdString());
    ui->labelContractAddress->setText(address);
#ifdef USE_QRCODE
    ui->lblQRCode->setText("");
        // limit URI length
    QRcode *code = QRcode_encodeString(address.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
        ui->lblQRCode->setText(tr("Error encoding address into QR Code."));
        return;
    }
    QImage qrImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
    qrImage.fill(0xffffff);
    unsigned char *p = code->data;
    for (int y = 0; y < code->width; y++)
    {
        for (int x = 0; x < code->width; x++)
        {
            qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
            p++;
        }
    }
    QRcode_free(code);


    QImage qrAddrImage = QImage(QR_IMAGE_SIZE*2, QR_IMAGE_SIZE*2+20, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);
    QPainter painter(&qrAddrImage);
    painter.drawImage(0, 0, qrImage.scaled(QR_IMAGE_SIZE*2, QR_IMAGE_SIZE*2));
    QFont font = GUIUtil::fixedPitchFont();
    font.setPixelSize(12);
    painter.setFont(font);
    QRect paddedRect = qrAddrImage.rect();
    paddedRect.setHeight(QR_IMAGE_SIZE*2+12);
    painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, address);
    painter.end();

    ui->lblQRCode->setPixmap(QPixmap::fromImage(qrAddrImage));
#endif
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
    QWidget *scrollWidget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addStretch();
    textEdits.clear();
    for(ContractMethodParams& cm : method.inputs){

        QLabel *label = new QLabel(QString::fromStdString(cm.name + " (" + cm.type + ")"), this);
        QLineEdit *textEdit = new QLineEdit(this);

        textEdits.push_back(textEdit);

        vLayout->addWidget(label);
        vLayout->addWidget(textEdit);

        ui->scrollAreaWriteToContract->setWidget(scrollWidget);
        ui->scrollAreaWriteToContract->setWidgetResizable(true);
        connect(textEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateTextEditsParams()));
    }

    scrollWidget->setLayout(vLayout);
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
        sig += e.type + ",";
    }
    sig.replace(sig.length() - 1, 1, ")");
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
            return;
        }
    }

    CReserveKey reservekey(pwalletMain);
    CWalletTx wtx(createTransactionOpCall(reservekey, coinControl, result, nGasLimit, nGasPrice, contractAddress, error, true));
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