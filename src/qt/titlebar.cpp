#include "titlebar.h"
#include "ui_titlebar.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "tabbarinfo.h"
#include <QPixmap>
#include "platformstyle.h"

namespace TitleBar_NS {
const int logoWidth = 135;
}
using namespace TitleBar_NS;

TitleBar::TitleBar(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleBar),
    m_tab(0),
    style(platformStyle)
{
    ui->setupUi(this); this->setWindowFlags(this->windowFlags()& ~Qt::WindowContextHelpButtonHint);
    // Set the logo
    // Set size policy
    ui->settings->setIcon(style->SingleColorIcon(":/icons/options",QColor(0xa1bdcd)));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->tab_Widget->setDrawBase(false);
}

TitleBar::~TitleBar()
{
    delete ui;
}

void TitleBar::setTabBarInfo(QObject *info)
{
    return;
    if(m_tab)
    {
        m_tab->detach();
    }

    if(info && info->inherits("TabBarInfo"))
    {
        TabBarInfo* tab = (TabBarInfo*)info;
        m_tab = tab;
        m_tab->attach(ui->tab_Widget);
    }
}

void TitleBar::on_navigationResized(const QSize &_size)
{
    ui->widgetLogo->setMaximumWidth(_size.width());
}
