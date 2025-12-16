/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2Gtk]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2Gtk.h"

#include <dlfcn.h>

#include <QObject>
#include <QMessageBox>
#include <QCheckBox>

#include "LV2GtkImpl.h"
#include "misc/Debug.h"
#include "misc/Preferences.h"

#ifdef HAVE_GTK2

namespace Rosegarden
{

LV2Gtk* LV2Gtk::m_instance = nullptr;

LV2Gtk* LV2Gtk::getInstance()
{
    if (m_instance == nullptr) {
        RG_DEBUG << "creating instance";
        // warn user
        if (Preferences::getShowGtk2Warning()) {
            QCheckBox *cb = new QCheckBox(tr("Do not show this again"));
            QMessageBox msgbox;
            msgbox.setText(tr("You are about to load the gtk2 infratructure. This has been know to cause crashes on some systems. Possible workarounds are to set environment variables for the Rosegarden program:<br/>set QT_QPA_PLATFORMTHEME to gtk2 (and install the gtk2 style plugin or<br/>set XDG_SESSION_TYPE to unknown"));
            msgbox.setIcon(QMessageBox::Icon::Question);
            msgbox.addButton(QMessageBox::Ok);
            msgbox.addButton(QMessageBox::Cancel);
            msgbox.setDefaultButton(QMessageBox::Cancel);
            msgbox.setCheckBox(cb);

            int result = msgbox.exec();
            if (result == QMessageBox::Cancel) return nullptr;
            if (cb->isChecked()) {
                Preferences::setShowGtk2Warning(false);
            }
        }
        m_instance = new LV2Gtk;
    }
    return m_instance;
}

LV2Gtk::LV2Gtk()
{

    // load shared library
    m_dlib = dlopen("librosegardenGtk.so", RTLD_LOCAL | RTLD_LAZY);
    RG_DEBUG << "getInstance lib:" << m_dlib;
    if (m_dlib) {
        void (*createLV2GtkImplPtr)();
        *(void **)(&createLV2GtkImplPtr) = dlsym(m_dlib, "createLV2GtkImpl");
        RG_DEBUG << "getInstance implPtr:" << (void*)createLV2GtkImplPtr;
        if (createLV2GtkImplPtr) {
            createLV2GtkImplPtr();
        } else {
            RG_DEBUG << dlerror();
        }
    } else {
        // warn user that lib not found
        (void)QMessageBox::warning
            (this,
             QObject::tr("Rosegarden"),
             QObject::tr("The rosegarden gtk2 library (librosegardenGtk.so) has not been found. Ensure that the library is in a standard location or is findable with the LD_LIBRARY_PATH environment variable."),
             QMessageBox::Ok);
    }
}

LV2Gtk::~LV2Gtk()
{
    RG_DEBUG << "dtor";
    if (m_dlib) dlclose(m_dlib);
}

LV2GtkTypes::LV2GtkWidget LV2Gtk::getWidget(LV2UI_Widget lv2Widget,
                                            LV2GtkTypes::SizeCallback* sizecb)
{
    RG_DEBUG << "getWidget" << &lv2Widget << sizecb;
    if (!m_dlib) {
        LV2GtkTypes::LV2GtkWidget widget;
        return widget;
    }
    LV2GtkTypes::LV2GtkWidget
        (*getWidgetPtr)(LV2UI_Widget lv2Widget,
                        LV2GtkTypes::SizeCallback* sizecb);
    *(void **)(&getWidgetPtr) = dlsym(m_dlib, "getWidget");
    if (getWidgetPtr) {
        return getWidgetPtr(lv2Widget, sizecb);
    } else {
        RG_DEBUG << dlerror();
    }

    LV2GtkTypes::LV2GtkWidget widget;
    return widget;
}

void LV2Gtk::getSize(const LV2GtkTypes::LV2GtkWidget& widget,
                     int& width,
                     int& height) const
{
    RG_DEBUG << "getSize" << &widget << width << height;
    if (!m_dlib) {
        width = 0;
        height = 0;
        return;
    }
    void (*getSizePtr)(const LV2GtkTypes::LV2GtkWidget& widget,
                       int& width,
                       int& height);
    *(void **)(&getSizePtr) = dlsym(m_dlib, "getSize");
    if (getSizePtr) {
        getSizePtr(widget, width, height);
        return;
    } else {
        RG_DEBUG << dlerror();
    }

    width = 0;
    height = 0;
}

long int LV2Gtk::getWinId(const LV2GtkTypes::LV2GtkWidget& widget)
{
    RG_DEBUG << "getWinId" << &widget;
    if (!m_dlib) {
        return 0;
    }
    int (*getWinIdPtr)(const LV2GtkTypes::LV2GtkWidget& widget);
    *(void **)(&getWinIdPtr) = dlsym(m_dlib, "getWinId");
    if (getWinIdPtr) {
        return getWinIdPtr(widget);
    } else {
        RG_DEBUG << dlerror();
    }

    return 0;
}

void LV2Gtk::deleteWidget(const LV2GtkTypes::LV2GtkWidget& widget)
{
    RG_DEBUG << "deleteWidget" << &widget;
    if (!m_dlib) {
        return;
    }
    void (*deleteWidgetPtr)(const LV2GtkTypes::LV2GtkWidget& widget);
    *(void **)(&deleteWidgetPtr) = dlsym(m_dlib, "deleteWidget");
    if (deleteWidgetPtr) {
        deleteWidgetPtr(widget);
        return;
    } else {
        RG_DEBUG << dlerror();
    }

}

void LV2Gtk::shutDown()
{
    RG_DEBUG << "shutDown";
    if (m_instance == nullptr) {
        RG_DEBUG << "LV2Gtk not active";
        return;
    }
    // instance exists
    LV2Gtk* lv2gtk = LV2Gtk::getInstance();
    lv2gtk->doShutDown();
    delete lv2gtk;
    m_instance = nullptr;
}

void LV2Gtk::doShutDown()
{
    if (!m_dlib) {
        return;
    }
    void (*shutDownPtr)();
    *(void **)(&shutDownPtr) = dlsym(m_dlib, "shutDown");
    if (shutDownPtr) {
        shutDownPtr();
        return;
    } else {
        RG_DEBUG << dlerror();
    }
}

}

#else

// no gtk2 - dummy versions
namespace Rosegarden
{

LV2Gtk* LV2Gtk::getInstance()
{
    return nullptr;
}

LV2Gtk::LV2Gtk()
{
}

LV2Gtk::~LV2Gtk()
{
}

LV2Gtk::LV2GtkWidget LV2Gtk::getWidget(LV2UI_Widget,
                                       SizeCallback*)
{
    LV2GtkWidget ret;
    return ret;
}

void LV2Gtk::getSize(const LV2GtkWidget&, int&, int&) const
{
}

long int LV2Gtk::getWinId(const LV2GtkWidget&)
{
    return 0;
}

void LV2Gtk::deleteWidget(const LV2GtkWidget&)
{
}

void LV2Gtk::shutDown()
{
}

}
#endif
