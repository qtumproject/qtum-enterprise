#include "contractsinfo.h"
#include "ui_contractsinfo.h"

ContractsInfo::ContractsInfo(WalletModel* _walletModel, QWidget *parent) :
    QWidget(parent), walletModel(_walletModel),
    ui(new Ui::ContractsInfo)
{
    ui->setupUi(this);

    ui->tableViewContractsInfo->setModel(model=new QStandardItemModel(0,3));
    ui->tableViewContractsInfo->setEditTriggers(QAbstractItemView::DoubleClicked);

    model->setHorizontalHeaderLabels(QStringList()<<tr("status")<<tr("time")<<tr("address")<<tr("balance"));
    ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    ui->tableViewContractsInfo->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);

    ui->tableViewContractsInfo->setColumnWidth(0,160);
    ui->tableViewContractsInfo->setColumnWidth(1,200);
    ui->tableViewContractsInfo->setColumnWidth(2,100);
    ui->tableViewContractsInfo->setColumnWidth(3,100);

    ui->tableViewContractsInfo->verticalHeader()->setVisible(false);
    ui->tableViewContractsInfo->setShowGrid(false);
}

ContractsInfo::~ContractsInfo()
{
    delete ui;
}

