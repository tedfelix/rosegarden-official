/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ThornStyle]"

#include "ThornStyle.h"

#include "ResourceFinder.h"
#include "gui/general/IconLoader.h"
#include "misc/Debug.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QDebug>
#include <qdrawutil.h>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QRadioButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QAbstractScrollArea>

using namespace Rosegarden;

// Silence gcc compiler warnings due to the switches below not covering all cases, on purpose
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

static QPixmap loadPix(const QString &name)
{
    QPixmap pix(name);
    if (pix.isNull()) {
        RG_WARNING << "::loadPix(): Pixmap not found:" << name;
        Q_ASSERT(0);
    }
    return pix;
}

/**
 * The AppEventFilter class is notified when a new widget is created
 * and can decide whether to apply the Thorn Style to it or not.
 */
class AppEventFilter : public QObject
{
    Q_OBJECT
public:
    AppEventFilter()
        : m_systemPalette(qApp->palette()),
          m_systemStyle(qApp->style()) {
    }
    bool eventFilter(QObject *watched, QEvent *event) override;

    bool shouldIgnoreThornStyle(QWidget *widget) const {
        return qobject_cast<QFileDialog *>(widget)
                || widget->inherits("KDEPlatformFileDialog")
                || widget->inherits("KDirSelectDialog");
    }

    void polishWidget(QWidget *widget);

private:
    ThornStyle m_style;
    QPalette m_systemPalette;
    QStyle *m_systemStyle;
};

// Apply the style to widget and its children, recursively
// Even though every widget goes through the event filter, this is needed
// for the case where a whole widget hierarchy is suddenly reparented into the file dialog.
// Then we need to apply the app style again. Testcase: scrollbars in file dialog.
static void applyStyleRecursive(QWidget* widget, QStyle *style)
{
    if (widget->style() != style) {
        widget->setStyle(style);
    }
    foreach (QObject* obj, widget->children()) {
        if (obj->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(obj);
            applyStyleRecursive(w, style);
        }
    }
}

// when we ditch Qt4, we can switch to qCDebug...
//#define DEBUG_EVENTFILTER

bool AppEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    static bool s_insidePolish = false; // setStyle calls polish again, so skip doing the work twice
    if (!s_insidePolish && watched->isWidgetType() && event->type() == QEvent::Polish) {
        s_insidePolish = true;
        // This is called after every widget is created and just before being shown
        // (we use this so that it has a proper parent widget already)
        QWidget *widget = static_cast<QWidget *>(watched);
        if (shouldIgnoreThornStyle(widget)) {
            // The palette from the mainwindow propagated to the dialog, restore it.
            widget->setPalette(m_systemPalette);
#ifdef DEBUG_EVENTFILTER
            qDebug() << widget << "now using app style (recursive)";
#endif
            applyStyleRecursive(widget, qApp->style());
            s_insidePolish = false;
            return false;
        }
        QWidget *toplevel = widget->window();
#ifdef DEBUG_EVENTFILTER
        qDebug() << widget << "current widget style=" << widget->style() << "shouldignore=" << shouldIgnoreThornStyle(toplevel);
#endif
        if (shouldIgnoreThornStyle(toplevel)) {
            // Here we should apply qApp->style() recursively on widget and its children, in case one was reparented
#ifdef DEBUG_EVENTFILTER
            qDebug() << widget << widget->objectName() << "in" << toplevel << "now using app style (recursive)";
#endif
            applyStyleRecursive(widget, qApp->style());
        } else if (widget->style() != &m_style) {
#ifdef DEBUG_EVENTFILTER
            //qDebug() << "    ToolTipBase=" << widget->palette().color(QPalette::ToolTipBase).name();
#endif
            // Apply style recursively because some child widgets (e.g. QHeaderView in QTreeWidget, in DeviceManagerDialog) don't seem to get here.
            if (qobject_cast<QAbstractItemView *>(widget)) {
                applyStyleRecursive(widget, &m_style);
            } else {
                widget->setStyle(&m_style);
            }
#ifdef DEBUG_EVENTFILTER
            qDebug() << "    now using style" << widget->style();
#endif
            if (widget->windowType() != Qt::Widget) { // window, tooltip, ...
                widget->setPalette(m_style.standardPalette());
#ifdef DEBUG_EVENTFILTER
                qDebug() << "    after setPalette:     ToolTipBase=" << widget->palette().color(QPalette::ToolTipBase).name();
#endif
            } else {
#ifdef DEBUG_EVENTFILTER
                //qDebug() << "    not a toplevel. ToolTipBase=" << widget->palette().color(QPalette::ToolTipBase).name();
#endif
            }
            polishWidget(widget);
        }
        s_insidePolish = false;
    }
    return false; // don't eat the event
}

void AppEventFilter::polishWidget(QWidget *widget)
{
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
        if (qobject_cast<QToolBar *>(widget->parentWidget())) {
            /* Toolbars must be light enough for black icons, therefore black text on their
               QLabels, rather than white, is more appropriate.
               QToolBar QLabel { color: #000000; } */
            QPalette pal = label->palette();
            pal.setColor(label->foregroundRole(), Qt::black);
            label->setPalette(pal);
            //qDebug() << "made label black:" << label << label->text();
        }
        if (widget->objectName() == "SPECIAL_LABEL") {
            widget->setAutoFillBackground(true);
            // QWidget#SPECIAL_LABEL { color: #000000; background-color: #999999; }
            QPalette palette = widget->palette();
            palette.setColor(QPalette::WindowText, Qt::black);
            palette.setColor(QPalette::Window, QColor(0x99, 0x99, 0x99));
            widget->setPalette(palette);
        }
    } else if (widget->objectName() == "Rosegarden Transport") {
        // Give the non-LED parts of the dialog the groupbox "lighter black"
        // background for improved contrast.
        QPalette transportPalette = widget->palette();
        transportPalette.setColor(widget->backgroundRole(), QColor(0x40, 0x40, 0x40));
        widget->setPalette(transportPalette);
        widget->setAutoFillBackground(true);
    } else if (QCheckBox *cb = qobject_cast<QCheckBox *>(widget)) {
        cb->setAttribute(Qt::WA_Hover);
    } else if (QRadioButton *rb = qobject_cast<QRadioButton *>(widget)) {
        rb->setAttribute(Qt::WA_Hover);
    } else if (QPushButton *pb = qobject_cast<QPushButton *>(widget)) {
        pb->setAttribute(Qt::WA_Hover);
        if (qobject_cast<QDialogButtonBox *>(widget->parentWidget())) {
            // Bug in QDialogButtonBox: if the app style sets QStyle::SH_DialogButtonBox_ButtonsHaveIcons
            // a later call to setStyle() doesn't remove the button icon again.
            // Fix submitted at https://codereview.qt-project.org/183788
            pb->setIcon(QIcon());
        }
    } else if (QComboBox *cb = qobject_cast<QComboBox *>(widget)) {
        cb->setAttribute(Qt::WA_Hover);
    } else if (QAbstractSpinBox *sb = qobject_cast<QAbstractSpinBox *>(widget)) {
        sb->setAttribute(Qt::WA_Hover);
    }
}

Q_GLOBAL_STATIC(AppEventFilter, s_eventFilter) // created on demand in setEnabled

////////////////////////////////////////////////////////////////////////////////////////////////////////

ThornStyle::ThornStyle()
      // We could load these on demand, but the mainwindow needs most of them already anyway
    : m_horizontalToolbarSeparatorPixmap(loadPix(":/pixmaps/style/htoolbar-separator.png")),
      m_verticalToolbarSeparatorPixmap(loadPix(":/pixmaps/style/vtoolbar-separator.png")),
      m_checkboxUncheckedPixmap(loadPix(":/pixmaps/style/checkbox_unchecked.png")),
      m_checkboxUncheckedHoverPixmap(loadPix(":/pixmaps/style/checkbox_unchecked_hover.png")),
      m_checkboxUncheckedDisabledPixmap(loadPix(":/pixmaps/style/checkbox_disabled.png")),
      m_checkboxUncheckedPressedPixmap(loadPix(":/pixmaps/style/checkbox_unchecked_pressed.png")),
      m_checkboxCheckedPixmap(loadPix(":/pixmaps/style/checkbox_checked.png")),
      m_checkboxCheckedHoverPixmap(loadPix(":/pixmaps/style/checkbox_checked_hover.png")),
      m_checkboxCheckedDisabledPixmap(loadPix(":/pixmaps/style/checkbox_checked_disabled.png")),
      m_checkboxCheckedPressedPixmap(loadPix(":/pixmaps/style/checkbox_checked_pressed.png")),
      m_checkboxIndeterminatePixmap(loadPix(":/pixmaps/style/checkbox_indeterminate.png")),
      m_checkboxIndeterminateHoverPixmap(loadPix(":/pixmaps/style/checkbox_indeterminate_hover.png")),
      //m_checkboxIndeterminateDisabledPixmap(loadPix(":/pixmaps/style/checkbox_indeterminate_disabled.png")),
      m_checkboxIndeterminatePressedPixmap(loadPix(":/pixmaps/style/checkbox_indeterminate_pressed.png")),
      m_radiobuttonUncheckedPixmap(loadPix(":/pixmaps/style/radiobutton_unchecked.png")),
      m_radiobuttonUncheckedHoverPixmap(loadPix(":/pixmaps/style/radiobutton_unchecked_hover.png")),
      m_radiobuttonUncheckedDisabledPixmap(loadPix(":/pixmaps/style/radiobutton_unchecked_disabled.png")),
      m_radiobuttonUncheckedPressedPixmap(loadPix(":/pixmaps/style/radiobutton_unchecked_pressed.png")),
      m_radiobuttonCheckedPixmap(loadPix(":/pixmaps/style/radiobutton_checked.png")),
      m_radiobuttonCheckedHoverPixmap(loadPix(":/pixmaps/style/radiobutton_checked_hover.png")),
      m_radiobuttonCheckedDisabledPixmap(loadPix(":/pixmaps/style/radiobutton_checked_disabled.png")),
      m_radiobuttonCheckedPressedPixmap(loadPix(":/pixmaps/style/radiobutton_checked_pressed.png")),
      m_arrowDownSmallPixmap(loadPix(":/pixmaps/style/arrow-down-small.png")),
      m_arrowDownSmallInvertedPixmap(loadPix(":/pixmaps/style/arrow-down-small-inverted.png")),
      m_arrowUpSmallPixmap(loadPix(":/pixmaps/style/arrow-up-small.png")),
      m_arrowUpSmallInvertedPixmap(loadPix(":/pixmaps/style/arrow-up-small-inverted.png")),
      m_arrowLeftPixmap(":/pixmaps/style/arrow-left.png"),
      m_arrowRightPixmap(":/pixmaps/style/arrow-right.png"),
      m_arrowUpPixmap(":/pixmaps/style/arrow-up.png"),
      m_arrowDownPixmap(":/pixmaps/style/arrow-down.png"),
      m_spinupPixmap(":/pixmaps/style/spinup.png"),
      m_spinupHoverPixmap(":/pixmaps/style/spinup_hover.png"),
      m_spinupOffPixmap(":/pixmaps/style/spinup_off.png"),
      m_spinupPressedPixmap(":/pixmaps/style/spinup_pressed.png"),
      m_spindownPixmap(":/pixmaps/style/spindown.png"),
      m_spindownHoverPixmap(":/pixmaps/style/spindown_hover.png"),
      m_spindownOffPixmap(":/pixmaps/style/spindown_off.png"),
      m_spindownPressedPixmap(":/pixmaps/style/spindown_pressed.png"),
      m_titleClosePixmap(":/pixmaps/style/title-close.png"),
      m_titleUndockPixmap(":/pixmaps/style/title-undock.png")
{
    // Qt 5 removes QPlastiqueStyle and defaults to a new style called "Fusion."
    // This style forces combo boxes to do bad things.
    // I concluded that using "windows" as a base style causes an acceptable amount
    // of damage while leaving things largely intact.
    setBaseStyle(QStyleFactory::create("windows"));

    m_standardPalette.setColor(QPalette::Window, Qt::black);

    // QLabel { color: white }
    m_standardPalette.setColor(QPalette::WindowText, Qt::white);

    // QListView, QTableView, QTreeView, QLineEdit... :
    // background-color: #FFFFFF;
    // color: #000000;
    // selection-background-color: #80AFFF;
    // selection-color: #FFFFFF;
    m_standardPalette.setColor(QPalette::Base, Qt::white);
    m_standardPalette.setColor(QPalette::Text, Qt::black);
    m_standardPalette.setColor(QPalette::Highlight, QColor(0x80, 0xAF, 0xFF));
    m_standardPalette.setColor(QPalette::HighlightedText, Qt::white);

    // for QPushButton but also QMenu
    const QColor buttonColor = QColor(0xEE, 0xEE, 0xEE);
    m_standardPalette.setColor(QPalette::Button, buttonColor);
    m_standardPalette.setColor(QPalette::ButtonText, Qt::black); // enabled button texts and menu items

    // alternate-background-color: #EEEEFF;
    m_standardPalette.setColor(QPalette::AlternateBase, QColor(0xEE, 0xEE, 0xFF));

    // QToolTip { background-color: #fffbd4; color: #000000; + some awful pixmap hack }
    m_standardPalette.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFB, 0xD4));
    m_standardPalette.setColor(QPalette::ToolTipText, Qt::black);
}

ThornStyle::~ThornStyle()
{
}

QIcon ThornStyle::standardIcon(QStyle::StandardPixmap standardIcon, const QStyleOption *option, const QWidget *parent) const
{
    // NOTE: see src/gui/styles/qcommonstyle.cpp in the Qt source for examples
    // of how to extend this whenever more custom icons are called for
    switch (standardIcon) {

    // custom icons for QMessageBox
    case SP_MessageBoxInformation:
        return m_iconLoader.loadPixmap("messagebox-information");

    case SP_MessageBoxWarning:
        return m_iconLoader.loadPixmap("warning");

    case SP_MessageBoxCritical:
        return m_iconLoader.loadPixmap("messagebox-critical");

    case SP_MessageBoxQuestion:
        return m_iconLoader.loadPixmap("messagebox-question");

    case SP_TitleBarNormalButton:
        return m_titleUndockPixmap;

    case SP_DockWidgetCloseButton:
    case SP_TitleBarCloseButton:
        return m_titleClosePixmap;

    default:
        // let the base class handle the rest
        return QProxyStyle::standardPixmap(standardIcon, option, parent);
    }
}

QSize ThornStyle::pixmapSize(const QPixmap &pixmap) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QSize(pixmap.width() / pixmap.devicePixelRatio(),
                 pixmap.height() / pixmap.devicePixelRatio());
#else
    return pixmap.size();
#endif
}

static bool s_thornStyleEnabled = false;

// This method currently only supports being called once
void ThornStyle::setEnabled(bool b)
{
    s_thornStyleEnabled = b;
    if (b) {
        qApp->installEventFilter(s_eventFilter());
    }
}

bool ThornStyle::isEnabled()
{
    return s_thornStyleEnabled;
}

QPalette ThornStyle::standardPalette() const
{
    return m_standardPalette;
}

int ThornStyle::styleHint(QStyle::StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_EtchDisabledText:
        return 0;
    case SH_Table_GridLineColor:
        // QTableView { gridline-color: #202020; }
        return qRgb(0x20, 0x20, 0x20);
    case SH_GroupBox_TextLabelColor:
        // QGroupBox::title { color: #FFFFFF; }
        // QGroupBox::title:!enabled { color: #000000; }
        // but it was etched; plain black is unreadable, so let's use another color now
        return option->state & State_Enabled ? qRgb(0xFF, 0xFF, 0xFF) : qRgb(0xAA, 0xAA, 0xAA);
    case SH_DialogButtonBox_ButtonsHaveIcons:
        return 0;
    case SH_DockWidget_ButtonsHaveFrame:
        return 1;
    default:
        break;
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int ThornStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_SplitterWidth: //fall-through
    case PM_DockWidgetSeparatorExtent:
        // QMainWindow::separator { height: 5px; }
        return 5;
    case PM_TabBarScrollButtonWidth:
        // QTabBar::scroller { /* the width of the scroll buttons */ width: 13px; }
        return 13;
    case PM_TabBarBaseOverlap:
        return 0;
    case PM_ToolBarHandleExtent: // Horizontal toolbar: width of the handle. Vertical toolbar: height of the handle
        if (option->state & State_Horizontal)
            return m_horizontalToolbarSeparatorPixmap.width();
        else
            return m_verticalToolbarSeparatorPixmap.height();
    case PM_ExclusiveIndicatorWidth:
        return m_radiobuttonUncheckedPixmap.width();
    case PM_ExclusiveIndicatorHeight:
        return m_radiobuttonUncheckedPixmap.height();
    case PM_IndicatorWidth:
        return m_checkboxUncheckedPixmap.width();
    case PM_IndicatorHeight:
        return m_checkboxUncheckedPixmap.height();
    case PM_MenuPanelWidth:
        return 1;
    case PM_MenuBarHMargin:
        // QMenuBar { padding: 4px; }
        return 4;
    case PM_MenuBarItemSpacing:
        // QMenuBar::item { spacing: 3px; padding: 1px 4px; }
        return 4;
    case PM_ScrollBarExtent: {
        QWidget *parent = widget ? widget->parentWidget() : nullptr;
        QWidget *combo = parent ? parent->parentWidget() : nullptr;
        if (qobject_cast<QComboBox *>(combo)) {
            // QComboBox QAbstractItemView QScrollBar:vertical { width: 12px; }
            return 12;
        }
        // QScrollBar:horizontal { height: 16px; }
        // QScrollBar:vertical { width: 16px; }
        return 16;
    }
    case PM_ToolBarItemSpacing:
        return 0;
    case PM_ToolBarItemMargin:
    case PM_ToolBarFrameWidth:
        return 0;
    case PM_DefaultFrameWidth:
        return 2;
    case PM_SpinBoxFrameWidth:
        return 2;
    case PM_DockWidgetTitleBarButtonMargin:
        // icon size is 16x16 but somehow the buttons ended up 13x13
        return -1;
    case PM_DockWidgetTitleMargin: // space above and below the title
        return 0;
    case PM_DockWidgetFrameWidth:
        // QDockWidget { border: none; }
        return 0;
    case PM_SmallIconSize:
        return 16;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

void ThornStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_IndicatorDockWidgetResizeHandle:
        // QMainWindow::separator:hover { background-color: #CCDFFF; }
        if (option->state & QStyle::State_MouseOver) {
            painter->fillRect(option->rect, QColor(0xCC, 0xDF, 0xFF));
            return;
        }
        break;
    case PE_FrameTabWidget: // The tab widget frame
        // QTabWidget::pane { border: 2px solid #BBBBBB; border-radius: 4px; padding: 2px; // and background: #404040;
        painter->save();
        painter->setPen(QPen(QColor(0xBB, 0xBB, 0xBB), 2));
        painter->setBrush(QColor(0x40, 0x40, 0x40));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(option->rect.adjusted(2, 2, -2, -2), 4, 4);
        painter->restore();
        return;
    case PE_FrameGroupBox: // same as above but not the background, already done
        // QGroupBox { background: #404040; color: #FFFFFF; border: 2px solid #BBBBBB; border-radius: 4px; }
        painter->save();
        painter->setPen(QPen(QColor(0xBB, 0xBB, 0xBB), 2));
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(option->rect.adjusted(2, 2, -2, -2), 4, 4);
        painter->restore();
        return;
    case PE_IndicatorToolBarHandle: {
        // top or bottom: image: url(:/pixmaps/style/htoolbar-separator.png);
        // left or right: image: url(:pixmaps/style/vtoolbar-separator.png);
        QPixmap pixmap = option->state & State_Horizontal ? m_horizontalToolbarSeparatorPixmap : m_verticalToolbarSeparatorPixmap;
        const QRect rect = alignedRect(Qt::LayoutDirectionAuto, Qt::AlignCenter, option->rect.size(), option->rect);
        painter->drawPixmap(rect, pixmap);
        return;
    }
    case PE_PanelMenu:
        // QMenu { background-color: #EEEEEE; border: 1px solid black; }
        painter->fillRect(option->rect, QColor(0xEE, 0xEE, 0xEE));
        return;
    case PE_FrameMenu:
        painter->setPen(Qt::black);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        return;
    case PE_FrameDockWidget: // only called when the dockwidget is floating
        // QDockWidget { border: none; }
        return;
    case PE_PanelStatusBar: // no frame around the statusbar
    case PE_FrameStatusBarItem: // no frame around the statusbar items
        return;
    case PE_PanelLineEdit:
        // QLineEdit { border: 1px solid #AAAAAA; background-color: #FFFFFF; }
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (frame->lineWidth > 0) // i.e. not inside QSpinBox
                painter->setPen(QColor(0xAA, 0xAA, 0xAA));
            else
                painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::white);
            painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        }
        return;
    case PE_IndicatorMenuCheckMark:
        return; // done in CE_MenuItem
    case PE_IndicatorCheckBox:
    case PE_IndicatorRadioButton: {
        const bool checked = !(option->state & State_Off);
        const bool disabled = !(option->state & State_Enabled);
        const bool pressed = option->state & State_Sunken;
        const bool hover = option->state & State_MouseOver;
        QPixmap pixmap;
        if (element == PE_IndicatorCheckBox) {
            if (option->state & State_NoChange) {
                // missing icon for disabled
                if (pressed)
                    pixmap = m_checkboxIndeterminatePressedPixmap;
                else if (hover)
                    pixmap = m_checkboxIndeterminateHoverPixmap;
                else
                    pixmap = m_checkboxIndeterminatePixmap;
            } else {
                if (disabled) {
                    pixmap = checked ? m_checkboxCheckedDisabledPixmap : m_checkboxUncheckedDisabledPixmap;
                } else {
                    if (pressed)
                        pixmap = checked ? m_checkboxCheckedPressedPixmap : m_checkboxUncheckedPressedPixmap;
                    else if (hover)
                        pixmap = checked ? m_checkboxCheckedHoverPixmap : m_checkboxUncheckedHoverPixmap;
                    else
                        pixmap = checked ? m_checkboxCheckedPixmap : m_checkboxUncheckedPixmap;
                }
            }
        } else {
            if (disabled) {
                pixmap = checked ? m_radiobuttonCheckedDisabledPixmap : m_radiobuttonUncheckedDisabledPixmap;
            } else {
                if (pressed)
                    pixmap = checked ? m_radiobuttonCheckedPressedPixmap : m_radiobuttonUncheckedPressedPixmap;
                else if (hover)
                    pixmap = checked ? m_radiobuttonCheckedHoverPixmap : m_radiobuttonUncheckedHoverPixmap;
                else
                    pixmap = checked ? m_radiobuttonCheckedPixmap : m_radiobuttonUncheckedPixmap;
            }
        }
        QRect pmr(QPoint(0, 0), pixmapSize(pixmap));
        pmr.moveCenter(option->rect.center());
        painter->drawPixmap(pmr.topLeft(), pixmap);
        return;
    }
    case PE_IndicatorHeaderArrow:
        // QHeaderView::down-arrow / QHeaderView::up-arrow
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            const bool up = (header->sortIndicator & QStyleOptionHeader::SortUp);
            const QPixmap pixmap = up ? m_arrowUpSmallInvertedPixmap : m_arrowDownSmallInvertedPixmap;
            painter->drawPixmap(option->rect.topLeft(), pixmap);
        }
        return;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        QPixmap pixmap;
        if (element == PE_IndicatorArrowLeft)
            pixmap = m_arrowLeftPixmap;
        else if (element == PE_IndicatorArrowRight)
            pixmap = m_arrowRightPixmap;
        else if (element == PE_IndicatorArrowUp)
            pixmap = m_arrowUpPixmap;
        else if (element == PE_IndicatorArrowDown)
            pixmap = m_arrowDownPixmap;
        // Scale the pixmap to the desired rect (for scrollbars the pixmap is 12x12 but the rect is 8x8)
        pixmap = pixmap.scaled(option->rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // In case the scaling didn't occupy the full height (due to aspect ratio), center the resulting pixmap (eg: toolbutton menu indicator)
        const QRect drawRect = alignedRect(option->direction, Qt::AlignCenter, pixmap.size(), option->rect);
        painter->drawPixmap(drawRect.topLeft(), pixmap);
    }
        return;
    case PE_PanelButtonTool:
        if (widget && widget->inherits("QDockWidgetTitleButton")) {
            // QDockWidget::close-button, QDockWidget::float-button {
            //    border: 1px solid #AAAAAA;
            //    border-radius: 3px;
            //    background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD);
            //}
            painter->save();
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(0, option->rect.height());
            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
            painter->setBrush(gradient);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(option->rect.adjusted(1, 1, -1, -1), 3, 3);
            painter->restore();
        }
        else if ((option->state & State_On) || (option->state & State_Sunken)) {
            // QToolButton::pressed, QToolButton::checked { border: 1px solid #AAAAAA; border-radius: 2px;
            //            background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE); }
            painter->save();
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(0, option->rect.height());
            gradient.setColorAt(0, QColor(0xEE, 0xEE, 0xEE));
            gradient.setColorAt(1, QColor(0xE0, 0xE0, 0xE0));
            painter->setBrush(gradient);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 2, 2);
            painter->restore();
        }
        else if ((option->state & State_MouseOver) && (option->state & State_Enabled)) {
            // QToolButton::enabled:hover { border: 1px solid #AAAAAA; border-radius: 2px; background-color: #CCDFFF; }
            painter->save();
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            painter->setBrush(QColor(0xCC, 0xDF, 0xFF));
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 2, 2);
            painter->restore();
        }
        return;
    case PE_IndicatorProgressChunk:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            QStyleOptionProgressBar copy = *pb;
            // QProgressBar::chunk { background-color: #D6E8FB; }
            copy.palette.setColor(QPalette::Highlight, QColor(0xD6, 0xE8, 0xFB)); // qwindowsstyle.cpp uses Highlight
            QProxyStyle::drawPrimitive(element, &copy, painter, widget);
        }
        return;
    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void ThornStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_Splitter: // currently unused, but consistent with PE_IndicatorDockWidgetResizeHandle
        if (option->state & QStyle::State_MouseOver) {
            painter->fillRect(option->rect, QColor(0xCC, 0xDF, 0xFF));
            return;
        }
        break;
    case CE_TabBarTab:
        /* border: 1px solid #AAAAAA;
           background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD);
           color: #000000;
           border-bottom-color: #BBBBBB; // same as the pane color
           border-top-left-radius: 4px;
           border-top-right-radius: 4px;
        */
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            QRect tabRect = tab->rect;
            QColor borderColor;
            QLinearGradient gradient(0, 0, 0, tab->rect.height());
            const bool selected = tab->state & State_Selected;
            if (selected) {
                // QTabBar::tab:top:selected said
                // background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE);
                gradient.setColorAt(0, QColor(0xE0, 0xE0, 0xE0));
                gradient.setColorAt(1, QColor(0xEE, 0xEE, 0xEE));
                // border: 1px solid #E0E0E0;
                borderColor = QColor(0xE0, 0xE0, 0xE0);
            } else {
                gradient.setColorAt(0, QColor(0x99, 0x99, 0x99));
                gradient.setColorAt(1, QColor(0xDD, 0xDD, 0xDD));
                borderColor = QColor(0xAA, 0xAA, 0xAA);
            }

            QRect roundedRect;
            // Ruse: we draw a rounded rect that is too big at the bottom (or top, for South), but clipped, so that it looks square
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                if (!selected) {
                    // QTabBar::tab:top:!selected { margin-top: 2px; }
                    tabRect.adjust(0, 2, 0, 0);
                }
                roundedRect = tabRect.adjusted(0, 0, 0, 5);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                if (!selected) {
                    // QTabBar::tab:bottom:!selected { margin-bottom: 2px }
                    tabRect.adjust(0, 0, 0, -2);
                }
                roundedRect = tabRect.adjusted(0, -5, 0, 0);
                break;
            default:
                RG_WARNING << "drawControl(): Vertical tabbars not implemented yet, call David";
            }

            // Draw tab shape
            painter->save();
            painter->setClipRect(tabRect);
            painter->setPen(borderColor);
            painter->setBrush(gradient);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(roundedRect, 4, 4);
            painter->restore();

            // Draw tab text
            QStyleOptionTab modifiedOption = *tab;
            modifiedOption.palette.setColor(QPalette::WindowText, Qt::black);
            QProxyStyle::drawControl(CE_TabBarTabLabel, &modifiedOption, painter, widget);
        }
        return;
    case CE_ToolBar:
        {
            /*
             top or bottom: background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD)
             left or right: background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 #DDDDDD, stop:1 #999999)
            */
            QLinearGradient gradient;
            if (option->state & State_Horizontal) {
                gradient.setStart(0, 0);
                gradient.setFinalStop(0, option->rect.height());
            } else {
                gradient.setStart(0, 0);
                gradient.setFinalStop(option->rect.width(), 0);
            }
            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
            painter->fillRect(option->rect, gradient);
        }
        return;
    case CE_PushButtonBevel:
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (option->state & State_MouseOver) {
            // QPushButton:hover { border: 1px solid #AAAAAA; border-radius: 3px; background-color: #DDEFFF; }
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            painter->setBrush(QColor(0xDD, 0xEF, 0xFF));
        } else if ((option->state & State_On) || (option->state & State_Sunken)) {
            // QPushButton::checked, QPushButton::pressed { border: 1px solid #AAAAAA; border-radius: 2px; background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0EA, stop:1 #BBCFFF); }
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(0, option->rect.height());
            gradient.setColorAt(0, QColor(0xE0, 0xE0, 0xEA));
            gradient.setColorAt(1, QColor(0xBB, 0xCF, 0xFF));
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            painter->setBrush(gradient);
        } else {
            // QPushButton::enabled { border: 1px solid #AAAAAA; border-radius: 3px; background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); }
            // QPushButton::!enabled { border: 1px solid #808080; + same background as Qt does the sunken effect on these, and that looks fine. }
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(0, option->rect.height());
            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
            if (option->state & State_Enabled)
                painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            else
                painter->setPen(QPen(QColor(0x80, 0x80, 0x80)));
            painter->setBrush(gradient);
        }
        painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 3, 3);
        painter->restore();
        return;
    case CE_MenuItem:
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            int x, y, w, h;
            menuitem->rect.getRect(&x, &y, &w, &h);
            const bool checked = menuitem->checkType != QStyleOptionMenuItem::NotCheckable ? menuitem->checked : false;
            const bool selected = menuitem->state & State_Selected;
            const bool disabled = !(menuitem->state & State_Enabled);

            QColor textColor;
            if (selected) {
                // QMenu::item:selected { background-color: #80AFFF; }
                const QColor fill(0x80, 0xAF, 0xFF);
                painter->fillRect(menuitem->rect.adjusted(0, 0, -1, 0), fill);

                // QMenu::item:selected { color: #FFFFFF; }
                textColor = QColor(0xFF, 0xFF, 0xFF);
            } else {
                if (disabled) {
                    // QMenu::item:!enabled { color: #AAAAAA; }
                    textColor = QColor(0xAA, 0xAA, 0xAA);
                } else {
                    // QMenu::item:enabled { color: #000000; }
                    textColor = Qt::black;
                }
            }

            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator) {
                /* QMenu::separator {
                       height: 2px;
                       background: #AAAAAA;
                       margin-left: 10px;
                       margin-right: 5px;
                   } */
                const int yoff = y-1 + h / 2;
                painter->fillRect(x + 10, yoff, w - 15, 2, QColor(0xAA, 0xAA, 0xAA));
                return;
            }

            // taken from qwindowsstyle.cpp, and modified (due to the width of the checkmarks)

            const bool checkable = menuitem->checkType != QStyleOptionMenuItem::NotCheckable;
            const int checkcol = qMax<int>(menuitem->maxIconWidth, m_checkboxCheckedPixmap.width() + 4);
            const QRect vCheckRect = visualRect(option->direction, menuitem->rect, QRect(menuitem->rect.x(), menuitem->rect.y(), checkcol, menuitem->rect.height()));

            // Draw icon or checkmark
            QPixmap pixmap;
            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = disabled ? QIcon::Disabled : QIcon::Normal;
                if (selected && !disabled)
                    mode = QIcon::Active;
                if (checked)
                    pixmap = menuitem->icon.pixmap(pixelMetric(PM_SmallIconSize, option, widget), mode, QIcon::On);
                else
                    pixmap = menuitem->icon.pixmap(pixelMetric(PM_SmallIconSize, option, widget), mode);
            } else if (checkable) {
                if (!selected) {
                    if (menuitem->checkType & QStyleOptionMenuItem::Exclusive) {
                        pixmap = menuitem->checked ? m_radiobuttonCheckedPixmap :  m_radiobuttonUncheckedPixmap;
                    } else {
                        pixmap = menuitem->checked ? m_checkboxCheckedPixmap :  m_checkboxUncheckedPixmap;
                    }
                } else {
                    if (menuitem->checkType & QStyleOptionMenuItem::Exclusive) {
                        pixmap = menuitem->checked ? m_radiobuttonCheckedHoverPixmap :  m_radiobuttonUncheckedHoverPixmap;
                    } else {
                        pixmap = menuitem->checked ? m_checkboxCheckedHoverPixmap :  m_checkboxUncheckedHoverPixmap;
                    }
                }
            }
            QRect pmr(QPoint(0, 0), pixmapSize(pixmap));
            pmr.moveCenter(vCheckRect.center());
            painter->setPen(menuitem->palette.text().color());
            painter->drawPixmap(pmr.topLeft(), pixmap);

            const int itemFrame = 2;
            const int itemHMargin = 2;
            const int itemVMargin = 2;
            const int rightBorder = 15;
            const int arrowHMargin = 6;
            const int xm = itemFrame + checkcol + itemHMargin;
            const int xpos = menuitem->rect.x() + xm;
            painter->setPen(textColor);
            QRect textRect(xpos, y + itemVMargin,
                           w - xm - rightBorder - menuitem->tabWidth + 1, h - 2 * itemVMargin);
            QRect vTextRect = visualRect(option->direction, menuitem->rect, textRect);
            QString s(menuitem->text);
            if (!s.isEmpty()) {                     // draw text
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                if (!styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, menuitem->rect,
                        QRect(textRect.topRight(), QPoint(menuitem->rect.right(), textRect.bottom())));
                    const QString textToDraw = s.mid(t + 1);
                    painter->drawText(vShortcutRect, text_flags, textToDraw);
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                painter->setFont(font);
                const QString textToDraw = s.left(t);
                painter->drawText(vTextRect, text_flags, textToDraw);
            }
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) { // draw sub menu arrow
                const int dim = (h - 2 * itemFrame) / 2;
                const PrimitiveElement arrow = (option->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                const int submenuXPos = x + w - arrowHMargin - itemFrame - dim;
                const QRect vSubMenuRect = visualRect(option->direction, menuitem->rect, QRect(submenuXPos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = disabled ? State_None : State_Enabled;
                if (selected) {
                    newMI.palette.setColor(QPalette::ButtonText, textColor);
                }
                QProxyStyle::drawPrimitive(arrow, &newMI, painter, widget);
            }
        }
        return;
    case CE_CheckBoxLabel:
    case CE_RadioButtonLabel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QStyleOptionButton modifiedOption = *btn;
            const bool disabled = !(option->state & State_Enabled);
            // QCheckBox:!enabled { color: #000000; }
            // QCheckBox:enabled { color: #FFFFFF }
            modifiedOption.palette.setColor(QPalette::WindowText, disabled ? Qt::black : Qt::white);
            QProxyStyle::drawControl(element, &modifiedOption, painter, widget);
        }
        return;
    case CE_ComboBoxLabel:
        if (option->state & State_Enabled) {
            painter->setPen(Qt::black);
        } else {
            // disabled text light enough to be legible but not as stark as white
            painter->setPen(QColor(0xEE, 0xEE, 0xEE));
        }
        QCommonStyle::drawControl(element, option, painter, widget);
        return;
    case CE_Header:
        // Copied straight from qwindowsstyle.cpp because the subElementRect calls don't use proxy()->.
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QRegion clipRegion = painter->clipRegion();
            painter->setClipRect(option->rect);
            drawControl(CE_HeaderSection, header, painter, widget);
            QStyleOptionHeader subopt = *header;
            subopt.rect = subElementRect(SE_HeaderLabel, header, widget);
            if (subopt.rect.isValid()) {
                subopt.palette.setColor(QPalette::ButtonText, Qt::white); // QHeaderView::section { color: #FFFFFF; }
                drawControl(CE_HeaderLabel, &subopt, painter, widget);
            }
            if (header->sortIndicator != QStyleOptionHeader::None) {
                subopt.rect = subElementRect(SE_HeaderArrow, option, widget);
                drawPrimitive(PE_IndicatorHeaderArrow, &subopt, painter, widget);
            }
            painter->setClipRegion(clipRegion);
        }
        return;
    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            // QHeaderView::section { background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #707070, stop:1 #808080); }
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(0, option->rect.height());
            gradient.setColorAt(0, QColor(0x80, 0x80, 0x80));
            gradient.setColorAt(1, QColor(0x70, 0x70, 0x70));
            painter->setBrush(gradient);
            // border: 1px solid #AAAAAA;
            painter->setPen(QPen(QColor(0xAA, 0xAA, 0xAA)));
            painter->drawRect(header->rect);
            // Not converted, not sure what it did:   padding-left: 4px; padding-right: 1em;
        }
        return;
    case CE_MenuBarItem:
        if (const QStyleOptionMenuItem *m = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QStyleOptionMenuItem modifiedOption(*m);
            if (option->state & State_Selected) {
                // QMenuBar::item:selected { background-color: #80AFFF; }
                modifiedOption.palette.setColor(QPalette::Button, QColor(0x80, 0xAF, 0xFF));
            } else if (option->state & State_Sunken) {
                // QMenuBar::item:pressed { background-color: #BBCEFF; }
                modifiedOption.palette.setColor(QPalette::Button, QColor(0xBB, 0xCE, 0xFF));
            } else {
                // QMenuBar { background-color: #404040; }
                modifiedOption.palette.setColor(QPalette::Button, QColor(0x40, 0x40, 0x40));
            }
            // QMenuBar::item:selected { color: #FFFFFF; }
            // QMenuBar::item { color: #FFFFFF; }
            modifiedOption.palette.setColor(QPalette::ButtonText, Qt::white);

            QProxyStyle::drawControl(element, &modifiedOption, painter, widget);
        }
        return;
    case CE_MenuBarEmptyArea:
        // QMenuBar { background-color: #404040; }
        painter->fillRect(option->rect, QColor(0x40, 0x40, 0x40));
        return;
    case CE_ScrollBarSubLine:
    case CE_ScrollBarAddLine:
        {
            // QScrollBar::add-line:horizontal { border: 2px solid #404040; background: #808080; } but the 2px border didn't appear
            painter->fillRect(option->rect, QColor(0x80, 0x80, 0x80));
            // taken from qwindowsstyle.cpp
            PrimitiveElement arrow;
            if (option->state & State_Horizontal) {
                if (element == CE_ScrollBarAddLine)
                    arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowRight : PE_IndicatorArrowLeft;
                else
                    arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
            } else {
                if (element == CE_ScrollBarAddLine)
                    arrow = PE_IndicatorArrowDown;
                else
                    arrow = PE_IndicatorArrowUp;
            }
            QStyleOption arrowOpt = *option;
            // QScrollBar:*-arrow { width: 8px; height 8px; }
            arrowOpt.rect = alignedRect(option->direction, Qt::AlignCenter, QSize(8, 8), option->rect);
            drawPrimitive(arrow, &arrowOpt, painter, widget);
        }
        return;
    case CE_ScrollBarSubPage:
    case CE_ScrollBarAddPage:
        // Nothing to be done, CC_ScrollBar did the full fillRect()
        return;
    case CE_ScrollBarSlider:
        {
            // QScrollBar::handle:horizontal { background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); }
            // QScrollBar::handle:vertical { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 #DDDDDD, stop:1 #999999); }
            QLinearGradient gradient;
            gradient.setStart(0, 0);
            if (option->state & State_Horizontal)
                gradient.setFinalStop(0, option->rect.height());
            else
                gradient.setFinalStop(option->rect.width(), 0);
            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
            painter->fillRect(option->rect, gradient);
        }
        return;
    case CE_DockWidgetTitle: // titlebar on floating dock widgets
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            const bool active = dwOpt->state & State_Active;
            const bool floating = dwOpt->movable && (dwOpt->state & State_Window);
            QStyleOptionDockWidget copy = *dwOpt;
            copy.palette.setColor(QPalette::Dark, Qt::transparent); // QCommonStyle draws a border around the title with this color, we don't want it
            if (floating && !active)
                copy.palette.setColor(QPalette::WindowText, QColor(0xAA, 0xAA, 0xAA));
            else
                copy.palette.setColor(QPalette::WindowText, Qt::white);
            // Don't use QWindowsStyle for this, it uses black for the floating && !active case, cached from the initial app palette.
            QCommonStyle::drawControl(element, &copy, painter, widget);
        }
        return;
    case CE_ProgressBarGroove:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            // QProgressBar { background: #FFFFFF; border: 1px solid #AAAAAA; border-radius: 3px; }
            painter->save();
            painter->setBrush(Qt::white);
            painter->setPen(QColor(0xAA, 0xAA, 0xAA));
            painter->drawRoundedRect(pb->rect.adjusted(0, 0, -1, -1), 3, 3);
            painter->restore();
        }
        return;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            // QProgressBar { color: #000000; text-align: center; }
            QStyleOptionProgressBar copy = *pb;
            copy.palette.setColor(QPalette::HighlightedText, Qt::black);
            QProxyStyle::drawControl(element, &copy, painter, widget);
        }
        return;
    default:
        break;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void ThornStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    switch (control) {
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            // QScrollBar { border: 2px solid #404040; background-color: none; }
            // QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: #404040; }
            // Done here by filling the whole scrollbar, and moving the subcontrols by 2 pixels, then drawing nothing for AddPage/SubPage
            painter->fillRect(scrollBar->rect, QColor(0x40, 0x40, 0x40));
        }
        break; // let the base class do the rest
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
            const QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);
            if ((slider->subControls & SC_SliderGroove) && groove.isValid()) {
                // QSlider::groove:horizontal { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #EEEEEE); }
                QLinearGradient gradient;
                gradient.setStart(0, 0);
                if (option->state & State_Horizontal)
                    gradient.setFinalStop(0, option->rect.height());
                else
                    gradient.setFinalStop(option->rect.width(), 0);
                gradient.setColorAt(0, QColor(0xE0, 0xE0, 0xE0));
                gradient.setColorAt(1, QColor(0xEE, 0xEE, 0xEE));
                painter->fillRect(groove, gradient);
            }
#if 0 // no tickmarks
            if (slider->subControls & SC_SliderTickmarks) {
                QStyleOptionSlider tmpSlider = *slider;
                tmpSlider.subControls = SC_SliderTickmarks;
                QCommonStyle::drawComplexControl(control, &tmpSlider, painter, widget);
            }
#endif
            if (slider->subControls & SC_SliderHandle) {
                // QSlider::handle:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
                //                              border: 1px solid #5c5c5c; border-radius: 3px; }
                QLinearGradient gradient;
                gradient.setStart(0, 0);
                gradient.setFinalStop(option->rect.width(), option->rect.height());
                gradient.setColorAt(0, QColor(0xB4, 0xB4, 0xB4));
                gradient.setColorAt(1, QColor(0x8F, 0x8F, 0x8F));
                painter->setPen(QColor(0x5C, 0x5C, 0x5C));
                painter->setBrush(gradient);
                painter->setRenderHint(QPainter::Antialiasing);
                painter->drawRoundedRect(handle, 3, 3);
            }
        }
        return;
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            QStyleOptionGroupBox copy = *groupBox;
            // QGroupBox::title { subcontrol-position: top center; }
            copy.textAlignment = Qt::AlignHCenter;

            // Draw frame
            QRect textRect = subControlRect(CC_GroupBox, &copy, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = subControlRect(CC_GroupBox, &copy, SC_GroupBoxCheckBox, widget);
            if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                QStyleOptionFrame frame;
                frame.QStyleOption::operator=(*groupBox);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                frame.features = groupBox->features;
#endif
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = subControlRect(CC_GroupBox, &copy, SC_GroupBoxFrame, widget);
                painter->save();

                QRegion region(groupBox->rect);
                if (!groupBox->text.isEmpty()) {
                    bool ltr = groupBox->direction == Qt::LeftToRight;
                    QRect finalRect;
                    if (groupBox->subControls & QStyle::SC_GroupBoxCheckBox) {
                        finalRect = checkBoxRect.united(textRect);
                        finalRect.adjust(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
                    } else {
                        finalRect = textRect;
                    }
                    region -= finalRect;
                }

                // Draw background without clipping
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(0x40, 0x40, 0x40));
                painter->drawRoundedRect(frame.rect.adjusted(2, 2, -2, -2), 4, 4);

                painter->setClipRegion(region);
                // Draw frame with clipping
                drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
                painter->restore();
            }

            copy.subControls &= ~SC_GroupBoxFrame;
            QProxyStyle::drawComplexControl(control, &copy, painter, widget);
        }
        return;
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect button = subControlRect(control, toolbutton, SC_ToolButton, widget);
            QRect menuarea = subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);
            State bflags = toolbutton->state & ~State_Sunken;

            /*if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled)) {
                    bflags &= ~State_Raised;
                }
            }*/
            State mflags = bflags;
            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton)
                    bflags |= State_Sunken;
                else // added this "else" so that the down arrow only shifts by +1,+1 when actually clicking on it, not when clicking on the main toolbutton
                    mflags |= State_Sunken;
            }

            QStyleOption tool = *toolbutton;
            if (toolbutton->subControls & SC_ToolButton) {
                tool.rect = button;
                tool.state = bflags;
                drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            int fw = pixelMetric(PM_DefaultFrameWidth, option, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            // QToolButton { color: #FFFFFF; }
            label.palette.setColor(QPalette::ButtonText, Qt::white);
            drawControl(CE_ToolButtonLabel, &label, painter, widget);

            if (mflags & State_Sunken) {
                // QToolButton::menu-arrow:open { top: 1px; left: 1px; /* shift it a bit */ }
                painter->translate(1, 1);
            }
            if (toolbutton->subControls & SC_ToolButtonMenu) { // popupMode == QToolButton::MenuButtonPopup
                tool.rect = menuarea;
                tool.state = mflags;
                drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu) { // InstantPopup, like QPushButton menu
                // "the arrow on tool buttons with menus in InstantPopup mode is intentionally styled out of existence"
            }
        }
        return;
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if ((cmb->subControls & SC_ComboBoxFrame)) {
                // QComboBox { border: 1px solid #AAAAAA; border-radius: 3px; }
                // QComboBox::!enabled { border: 1px solid #808080; border-radius: 3px; }
                if (option->state & State_Enabled)
                    painter->setPen(QColor(0xAA, 0xAA, 0xAA));
                else
                    painter->setPen(QColor(0x80, 0x80, 0x80));

                if (option->state & State_MouseOver) {
                    // QComboBox:hover { background-color: #CCDFFF; }
                    painter->setBrush(QColor(0xCC, 0xDF, 0xFF));
                } else if (cmb->editable) {
                    // QComboBox::editable { background-color: #FFFFFF; }
                    painter->setBrush(Qt::white);
                } else {
                    // QComboBox::!editable { background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD); }
                    // QComboBox::!editable::on { background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE); }
                    QLinearGradient gradient;
                    gradient.setStart(0, 0);
                    gradient.setFinalStop(0, option->rect.height());
                    if (option->state & State_On) {
                        gradient.setColorAt(0, QColor(0xEE, 0xEE, 0xEE));
                        gradient.setColorAt(1, QColor(0xE0, 0xE0, 0xE0));
                    } else {
                        if (qobject_cast<QToolBar*>(widget->parentWidget())) {
                            gradient.setColorAt(0, QColor(0xEE, 0xEE, 0xEE));
                            gradient.setColorAt(1, QColor(0xDD, 0xDD, 0xDD));
                        } else {
                            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
                            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
                        }
                    }
                    painter->setBrush(gradient);
                }
                painter->setRenderHint(QPainter::Antialiasing);
                painter->drawRoundedRect(option->rect, 3, 3);
            }
            if (cmb->subControls & SC_ComboBoxArrow) {
                // from qwindowsstyle.cpp, without the rect around the arrow
                State flags = State_None;
                QRect ar = subControlRect(CC_ComboBox, cmb, SC_ComboBoxArrow, widget);
                bool sunkenArrow = cmb->activeSubControls == SC_ComboBoxArrow
                        && cmb->state & State_Sunken;
                ar.adjust(2, 2, -2, -2);
                if (option->state & State_Enabled)
                    flags |= State_Enabled;
                if (option->state & State_HasFocus)
                    flags |= State_HasFocus;

                if (sunkenArrow)
                    flags |= State_Sunken;
                QStyleOption arrowOpt = *cmb;
                arrowOpt.rect = ar.adjusted(1, 1, -1, -1);
                arrowOpt.state = flags;
                drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, painter, widget);
            }
#if 0
            if (cmb->subControls & SC_ComboBoxEditField) {
                QRect re = subControlRect(CC_ComboBox, cmb, SC_ComboBoxEditField, widget);
                if (cmb->state & State_HasFocus && !cmb->editable)
                    p->fillRect(re.x(), re.y(), re.width(), re.height(),
                                cmb->palette.brush(QPalette::Highlight));
            }
#endif
        }
        return;
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            bool enabled = option->state & State_Enabled;
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath framePath;
            // Draw background color (white), clipped to the rounded rect frame,
            // without drawing that frame yet (we'll do that last).
            // That same clipping will be useful for the round corner of the up/down buttons
            QRect frameRect = subControlRect(CC_SpinBox, sb, SC_SpinBoxFrame, widget).adjusted(2, 2, -2, -2);
            framePath.addRoundedRect(frameRect, 4, 4);
            painter->setClipPath(framePath);
            painter->fillRect(frameRect, sb->palette.brush(QPalette::Base));
            const QRect spinupRect = subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, widget);
            QLinearGradient gradient;
            gradient.setColorAt(0, QColor(0xDD, 0xDD, 0xDD));
            gradient.setColorAt(1, QColor(0x99, 0x99, 0x99));
            painter->setPen(QColor(0xAA, 0xAA, 0xAA));
            if (sb->subControls & SC_SpinBoxUp) {
                gradient.setStart(spinupRect.topLeft());
                gradient.setFinalStop(spinupRect.bottomLeft());
                if (sb->activeSubControls == SC_SpinBoxUp && !(option->state & State_Sunken))
                    painter->setBrush(QColor(0xDD, 0xEF, 0xFF));
                else
                    painter->setBrush(gradient);
                painter->drawRect(spinupRect);

                QPixmap pixmap = enabled ? m_arrowUpSmallPixmap
                                         : m_arrowUpSmallInvertedPixmap; // off state when value is max
                pixmap = pixmap.scaled(QSize(7, 7), Qt::KeepAspectRatio, Qt::SmoothTransformation); // TODO: edit the PNGs instead, would be faster
                const QRect drawRect = alignedRect(option->direction, Qt::AlignCenter, pixmap.size(), spinupRect);
                painter->drawPixmap(drawRect.topLeft(), pixmap);
            }
            if (sb->subControls & SC_SpinBoxDown) {
                const QRect spindownRect = subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, widget);
                gradient.setStart(spindownRect.topLeft());
                gradient.setFinalStop(spindownRect.bottomLeft());
                if (sb->activeSubControls == SC_SpinBoxDown && (option->state & State_MouseOver))
                    painter->setBrush(QColor(0xDD, 0xEF, 0xFF));
                else
                    painter->setBrush(gradient);
                painter->drawRect(spindownRect);

                QPixmap pixmap = enabled ? m_arrowDownSmallPixmap
                                         : m_arrowDownSmallInvertedPixmap; // off state when value is max
                pixmap = pixmap.scaled(QSize(7, 7), Qt::KeepAspectRatio, Qt::SmoothTransformation); // TODO: edit the PNGs instead, would be faster
                const QRect drawRect = alignedRect(option->direction, Qt::AlignCenter, pixmap.size(), spindownRect);
                painter->drawPixmap(drawRect.topLeft(), pixmap);
            }
            painter->setClipping(false);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QColor(0xB0, 0xB0, 0xB0)); // grabbed in frame.png
            painter->drawRoundedRect(frameRect, 4, 4);
        }
        return;
    default:
        break;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QSize ThornStyle::sizeFromContents(QStyle::ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    QSize sz = QProxyStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_LineEdit:
        // Reduce size of lineedits, to make the NameSetEditor more compact (in the BankEditorDialog)
        sz -= QSize(2, 2);
        break;
    case CT_SpinBox:
        if (const QStyleOptionSpinBox *vopt = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            // Add button + frame widths
            const int buttonWidth = m_spinupPixmap.width();
            const int fw = vopt->frame ? pixelMetric(PM_SpinBoxFrameWidth, vopt, widget) : 0;
            sz += QSize(buttonWidth + 2*fw, 2*fw);
        }
        break;
    default:
        break;
    }
    return sz;
}

QRect ThornStyle::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    QRect rect = QProxyStyle::subElementRect(element, option, widget);
    switch (element) {
    case SE_TabWidgetTabBar:
        // QTabWidget::tab-bar { left: 5px; /* move to the right by 5px */ }
        return rect.translated(5, 0);
    case SE_TabWidgetTabPane:
        return rect;
    case SE_TabWidgetTabContents:
        return rect.adjusted(2, 2, -2, -2);
    case SE_HeaderArrow: {
        const QSize size = pixmapSize(m_arrowUpSmallInvertedPixmap);
        QRect sectionRect = option->rect.adjusted(0, 0, -5, 0);
        const QRect ret = alignedRect(option->direction, Qt::AlignRight | Qt::AlignVCenter, size, sectionRect);
        return ret;
    }
    case SE_ToolBarHandle:
        if (const QStyleOptionToolBar *tbopt = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            // initially from qcommonstyle.cpp
            if (tbopt->features & QStyleOptionToolBar::Movable) {
                const QToolBar *tb = qobject_cast<const QToolBar*>(widget);
                const int margin = 1;
                const int handleExtent = pixelMetric(QStyle::PM_ToolBarHandleExtent, option, tb);
                QRect ret;
                if (tbopt->state & QStyle::State_Horizontal) {
                    ret = QRect(margin, margin, handleExtent, tbopt->rect.height() - 2*margin);
                    ret = QStyle::visualRect(tbopt->direction, tbopt->rect, ret);
                } else {
                    ret = QRect(margin, margin, tbopt->rect.width() - 2*margin, handleExtent);
                }
                return ret;
            }
        }
        break;
    default:
        break;
    }

    return rect;
}

QRect ThornStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex *option, QStyle::SubControl sc, const QWidget *widget) const
{
    switch (cc) {
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QStyleOptionSlider copy = *scrollBar;
            // QScrollBar said { border: 2px solid #404040; }
            // We draw that border by filling everything and reducing the available size for contents
            copy.rect.adjust(2, 2, -2, -2);
            return QProxyStyle::subControlRect(cc, &copy, sc, widget).adjusted(2, 2, 2, 2);
        }
        break;
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            //QSlider::groove:horizontal { height: 5px; }
            const int grooveThickness = 5;
            QRect ret;

            switch (sc) {
            case SC_SliderHandle: {
                int sliderPos = 0;
                // QSlider::handle:horizontal { width: 8px; }, but this needs to be 9 to look like the old qss for some reason
                int len = 9;
                bool horizontal = slider->orientation == Qt::Horizontal;
                sliderPos = sliderPositionFromValue(slider->minimum, slider->maximum,
                                                    slider->sliderPosition,
                                                    (horizontal ? slider->rect.width()
                                                                : slider->rect.height()) - len,
                                                    slider->upsideDown);
                if (horizontal) {
                    ret.setRect(slider->rect.x() + sliderPos, slider->rect.y() + 1, len, slider->rect.height() - 2);
                } else {
                    ret.setRect(slider->rect.x() + 1, slider->rect.y() + sliderPos, slider->rect.width() - 2, len);
                }
                //qDebug() << "ret=" << ret;
                break;
            }
            case SC_SliderGroove:
                if (slider->orientation == Qt::Horizontal) {
                    const int yOff = (slider->rect.height() - grooveThickness ) / 2;
                    ret.setRect(slider->rect.x(), slider->rect.y() + yOff,
                                slider->rect.width(), grooveThickness);
                } else {
                    const int xOff = (slider->rect.width() - grooveThickness ) / 2;
                    ret.setRect(slider->rect.x() + xOff, slider->rect.y(),
                                grooveThickness, slider->rect.height());
                }
                break;
            default:
                break;
            }
            return visualRect(slider->direction, slider->rect, ret);
        }
        break;
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            const int fw = 3; // frame width
            const int buttonTopBottomMargin = fw - 1;
            const QSize buttonSize(18, (sb->rect.height() - buttonTopBottomMargin) / 2 - 1);
            const int x = sb->rect.x() + sb->rect.width() - fw - buttonSize.width();
            const int y = sb->rect.y() + buttonTopBottomMargin;
            QRect ret;
            switch (sc) {
            case SC_SpinBoxFrame:
                return sb->rect;
            case SC_SpinBoxUp:
                if (sb->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                ret = QRect(x, y, buttonSize.width(), buttonSize.height());
                break;
            case SC_SpinBoxDown:
                if (sb->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                ret = QRect(x, sb->rect.height() - buttonTopBottomMargin - buttonSize.height(), buttonSize.width(), buttonSize.height());
                break;
            case SC_SpinBoxEditField:
                if (sb->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    ret = QRect(sb->rect.x() + fw, fw, sb->rect.width() - 2*fw, sb->rect.height() - 2*fw);
                } else {
                    ret = QRect(sb->rect.x() + fw, fw, x - fw, sb->rect.height() - 2*fw);
                }
                break;
            default:
                break;
            }
            return visualRect(sb->direction, sb->rect, ret);
        }
        break;
    default:
        break;
    }

    return QProxyStyle::subControlRect(cc, option, sc, widget);
}

#pragma GCC diagnostic pop

// TODO
#if 0
QTabBar QToolButton
{ /* the scroll buttons are tool buttons */
    border-image: url(:pixmaps/style/tab-scroll-button.png) 1;
    border-width: 1px;
}

QTabBar QToolButton:hover
{
    border-image: url(:pixmaps/style/tab-scroll-button-hover.png) 1;
    border-width: 1px;
}

QTabBar QToolButton:!enabled
{
    border-image: url(:pixmaps/style/tab-scroll-button-disabled.png) 1;
    border-width: 1px;
}

QTabBar QToolButton::right-arrow
{ /* the arrow mark in the tool buttons */
    image: url(:pixmaps/style/arrow-right-small.png);
}

QTabBar QToolButton::left-arrow
{
    image: url(:pixmaps/style/arrow-left-small.png);
}

#endif

#if 0 // TODO?
/* Transport buttons are styled independently, with a smaller radius and a
 * lighter "pressed" state
 */
#RosegardenTransport QPushButton::enabled,
#RosegardenTransport QFrame QWidget QPushbutton
{
    border: 1px solid #AAAAAA;
    border-radius: 2px;
    background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #999999, stop:1 #DDDDDD);
    min-width: 0;
}

#RosegardenTransport QFrame QWidget QPushbutton:hover,
#RosegardenTransport QPushButton:hover
{
    border: 1px solid #AAAAAA;
    border-radius: 2px;
    background-color: #CCDFFF;
    color: #000000;
    min-width: 0;
}

#RosegardenTransport QFrame QWidget QPushbutton:checked,
#RosegardenTransport QFrame QWidget QPushbutton:pressed,
#RosegardenTransport QPushButton::checked,
#RosegardenTransport QPushButton::pressed
{
    border: 1px solid #E0E0E0;
    border-radius: 1px;
    background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE);
}

#endif

#include "ThornStyle.moc"
