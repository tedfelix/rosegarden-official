#ifndef _ICONSTACKEDWIDGET_H_
#define _ICONSTACKEDWIDGET_H_

#include "IconButton.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <vector>

class IconStackedWidget : public QWidget
{
    Q_OBJECT
public:
    IconStackedWidget(QWidget *parent = 0);
    void addPage(const QString& iconLabel, QWidget *page, const QPixmap& icon);
    typedef std::vector<IconButton *> iconbuttons;
    
public slots:
    void slotPageSelect();
    
protected:
    iconbuttons m_iconButtons;
    int m_buttonHeight;
    int m_buttonWidth;
    QFrame * m_iconPanel;
    
private:
    QVBoxLayout * m_iconLayout;
    QStackedWidget * m_pagePanel;
    QHBoxLayout * m_layout;
    QColor m_backgroundColor;
};

#endif
