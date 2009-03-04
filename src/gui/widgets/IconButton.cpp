#include "IconButton.h"

#include <QPainter>
#include <QWidget>
#include <QPixmap>

#include <QDebug>

IconButton::IconButton(QWidget* parent, const QPixmap& icon, const QString & name)
: QAbstractButton(parent),
m_margin(5),
m_textColor(QColor(0,0,0)),
m_checkedColor(QColor(64,64,192))
{
    // Store the icon and name, these will be rendered in the paint event
    m_pixmap = icon;
    m_labelText = name;
    
    // Get size of label
    m_font.setPixelSize(12);
    m_font.setBold(true);
    QFontMetrics metrics(m_font);
    m_labelSize = QSize(metrics.width(m_labelText),metrics.ascent());

    ///@@@ Shouldn't we use contentsmargins instead of a local m_margins member for this?
    setMinimumSize(std::max(icon.width(),m_labelSize.width())+2*m_margin, icon.height()+m_labelSize.height()+3*m_margin);
}

void IconButton::paintEvent(QPaintEvent* event)
{
    QPainter paint(this);

    if (isChecked()) {
        paint.setPen(m_checkedColor);
        paint.setBrush(m_checkedColor);
        paint.drawRect(QRect(0,0,width(),height()));
    }

    paint.drawPixmap((width()-m_pixmap.width())/2,(height()-m_pixmap.height()-m_labelSize.height())/2,m_pixmap);

    paint.setPen(m_textColor);
    paint.setFont(m_font);
    paint.drawText((width()-m_labelSize.width())/2, height()-m_margin, m_labelText);
}

void IconButton::setCheckedColor(QColor color)
{
    m_checkedColor = color;
}

#include "IconButton.moc"
