#include "IconStackedWidget.h"
#include "IconButton.h"

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

IconStackedWidget::IconStackedWidget(QWidget *parent)
: QWidget(parent),
m_buttonHeight(0),
m_buttonWidth(0),
m_backgroundColor(QColor(255,255,255))
{
    
    // Use a frame widget for the icon panel, it will hold a bunch of buttons
    m_iconPanel = new QFrame(this);
    m_iconPanel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_iconPanel->setLineWidth(2);

    // Use a QPalette to set the iconPanel background
    ///@@@ Needs attention for style sheets
    QPalette colorPalette;
    colorPalette.setColor(QPalette::Window, m_backgroundColor);
    m_iconPanel->setPalette(colorPalette);
    m_iconPanel->setAutoFillBackground(true);

    // Use a VBoxLayout for the icon buttons
    m_iconLayout = new QVBoxLayout;
    
    // Buttons butt up against each other
    m_iconLayout->setSpacing(0);
    
    // No margin between buttons and their frame
    m_iconLayout->setContentsMargins(0,0,0,0);
    
    // Want the widget fixed to the top of the space
    // A stretch item must be at the bottom of the list
    // This approach changes the direction to from Bottom to Top
    //  and adds a stretch item first in the list
    m_iconLayout->setDirection(QBoxLayout::BottomToTop);
    m_iconLayout->addStretch(1);

    m_iconPanel->setLayout(m_iconLayout);
    
    // Use a stacked widget for the pages so the selected on is displayed
    m_pagePanel = new QStackedWidget(this);

    // Use a QHBoxLayout for icon and page panels
    m_layout = new QHBoxLayout;
    
    // Add the icon and page panels to the main layout
    m_layout->addWidget(m_iconPanel);
    m_layout->addWidget(m_pagePanel);
    setLayout(m_layout);
}
    
void IconStackedWidget::addPage(const QString& name, QWidget *page, const QPixmap& icon)
{
    IconButton *iconButton = new IconButton(m_iconPanel,icon, name);    

    // IconStackedWidget acts like a radio button widget with exclusive buttons
    iconButton->setCheckable(true);
    iconButton->setAutoExclusive(true);

    // If the new button is the biggest so far, update the default size
    if ((iconButton->minimumWidth() > m_buttonWidth) || (iconButton->minimumHeight() > m_buttonHeight)) {
        m_buttonWidth = std::max(iconButton->minimumWidth(),m_buttonWidth);
        m_buttonHeight = std::max(iconButton->minimumHeight(),m_buttonHeight);
        // Update the size of previous buttons
        for (iconbuttons::iterator i = m_iconButtons.begin();
            i != m_iconButtons.end(); ++i)
            (*i)->setMinimumSize(m_buttonWidth, m_buttonHeight);
    }

    iconButton->setMinimumSize(m_buttonWidth, m_buttonHeight);

    // If the list of buttons is not empty set the new buttons background to the default
    if (!m_iconButtons.size()) {
        iconButton->setChecked(true);
    }
    
    // Store the new button in a list for later modification
    m_iconButtons.push_back((IconButton *) iconButton);
    
    // Add the button to the icon layout, insert to the second point in the list
    //   the first hold the stretch item
    m_iconLayout->insertWidget(1,iconButton);

    // Add the new page to the page layout
    m_pagePanel->addWidget(page);

    // Connect the button's clicked data signal to the page select slot
    connect(iconButton, SIGNAL(clicked()), this, SLOT(slotPageSelect()));
}

void IconStackedWidget::slotPageSelect()
{
    // Cycle through the buttons to find the one that is checked
    iconbuttons::iterator i = m_iconButtons.begin();
    int index = 0;
    while (((*i)->isChecked() == false) && (i != m_iconButtons.end())) {
        i++;
        index++;
    }
    
    // Select the new page
    m_pagePanel->setCurrentIndex(index);
}

#include "IconStackedWidget.moc"
