#ifndef _ICONBUTTON_H_
#define _ICONBUTTON_H_

#include <QAbstractButton>
#include <QPixmap>
#include <QString>

class IconButton : public QAbstractButton
{
    Q_OBJECT
    
public:
    IconButton(QWidget* parent, const QPixmap& icon, const QString & name);

    virtual void paintEvent(QPaintEvent*);
    void setCheckedColor(QColor color);
    
public slots:
    
signals:

private:
    QPixmap m_pixmap;
    QString m_labelText;
    QFont m_font;
    int m_margin;
    QSize m_labelSize;
    QColor m_textColor;
    QColor m_checkedColor;
};

#endif
