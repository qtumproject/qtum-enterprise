#include "navigationbar.h"
#include <QActionGroup>
#include <QToolButton>
#include <QLayout>

namespace NavigationBar_NS
{
static const int ToolButtonWidth = 180;
static const int ToolButtonHeight = 30;
static const int ToolButtonIconSize = 20;
static const int MarginLeft = 10;
static const int MarginRight = 0;
static const int MarginTop = 0;
static const int MarginBottom = 9;
static const int MarginLeftSubBar = 115;
static const int MarginRightSubBar = 5;
static const int MarginTopSubBar = 0;
static const int MarginBottomSubBar = 0;
static const float scaleSubBarWidth = 0.5;
static const float scaleSubBarHeight = 0.8;

}
using namespace NavigationBar_NS;

NavigationBar::NavigationBar(QWidget *parent) :
    QWidget(parent),
    m_toolStyle(Qt::ToolButtonTextBesideIcon),
    m_subBar(false),
    m_built(false)
{
}

void NavigationBar::addAction(QAction *action)
{
    // Add action to the list
    m_actions.append(action);
}

QAction *NavigationBar::addGroup(QList<QAction *> list, const QIcon &icon, const QString &text)
{
    // Add new group
    QAction* action = new QAction(icon, text, this);
    mapGroup(action, list);
    return action;
}

QAction *NavigationBar::addGroup(QList<QAction *> list, const QString &text)
{
    // Add new group
    QAction* action = new QAction(text, this);
    mapGroup(action, list);
    return action;
}

void NavigationBar::buildUi()
{
    // Build the layout of the complex GUI component
    if(!m_built)
    {
        // Set it visible if main component
        setVisible(!m_subBar);

        // Create new layout for the bar
        QActionGroup* actionGroup = new QActionGroup(this);
        actionGroup->setExclusive(true);
        QVBoxLayout* vboxLayout = new QVBoxLayout(this);
        int defButtonWidth = m_subBar ? ToolButtonWidth * scaleSubBarWidth : ToolButtonWidth;
        int defButtonHeight = m_subBar ? ToolButtonHeight * scaleSubBarHeight : ToolButtonHeight;
        vboxLayout->setContentsMargins(m_subBar ? MarginLeftSubBar : MarginLeft,
                                       m_subBar ? MarginTopSubBar : MarginTop,
                                       m_subBar ? MarginRightSubBar : MarginRight,
                                       m_subBar ? MarginBottomSubBar : MarginBottom);
        vboxLayout->setSpacing(MarginLeft / 2);

        // List all actions
        for(int i = 0; i < m_actions.count(); i++)
        {
            // Add an action to the layout
            QAction* action = m_actions[i];
            action->setActionGroup(actionGroup);
            action->setCheckable(true);
            QToolButton* toolButton = new QToolButton(this);
            toolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            toolButton->setMinimumHeight(defButtonHeight);
            toolButton->setToolButtonStyle(m_toolStyle);
            toolButton->setDefaultAction(action);
            toolButton->setIconSize(QSize(ToolButtonIconSize, ToolButtonIconSize));
            toolButton->setObjectName(action->objectName());
            vboxLayout->addWidget(toolButton);

            if(m_groups.contains(action))
            {
                // Add sub-navigation bar for the group of actions
                QList<QAction*> group = m_groups[action];
                NavigationBar* subNavBar = new NavigationBar(this);
                subNavBar->setSubBar(true);
                for(int j = 0; j < group.count(); j++)
                {
                    subNavBar->addAction(group[j]);
                }
                vboxLayout->addWidget(subNavBar);
                subNavBar->buildUi();
                connect(action, SIGNAL(toggled(bool)), subNavBar, SLOT(onSubBarClick(bool)));
            }
        }

        if(!m_subBar)
        {
            // Set specific parameters for for the main component
            if(m_actions.count())
            {
                m_actions[0]->setChecked(true);
            }
            setMinimumWidth(defButtonWidth + MarginLeft);
            vboxLayout->addStretch(1);

        }

        // The component is built
        m_built = true;
    }
}

void NavigationBar::onSubBarClick(bool clicked)
{
    // Expand/collapse the sub-navigation bar
    setVisible(clicked);


    if(clicked && m_actions.count() > 0)
    {
        // Activate the checked action
        bool haveChecked = false;
        for(int i = 0; i < m_actions.count(); i++)
        {
            QAction* action = m_actions[i];
            if(action->isChecked())
            {
                action->trigger();
                haveChecked = true;
                break;
            }
        }
        if(!haveChecked)
        {
            m_actions[0]->trigger();
        }
    }
}

void NavigationBar::setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
    // Set the tool button style
    m_toolStyle = toolButtonStyle;
}

void NavigationBar::resizeEvent(QResizeEvent *evt)
{
    QWidget::resizeEvent(evt);
    Q_EMIT resized(size());
}

void NavigationBar::mapGroup(QAction *action, QList<QAction *> list)
{
    // Map the group with the actions
    addAction(action);
    m_groups[action] = list;
}

void NavigationBar::setSubBar(bool subBar)
{
    // Set the component be sub-navigation bar
    m_subBar = subBar;
}

