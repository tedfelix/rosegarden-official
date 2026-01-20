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

#define RG_MODULE_STRING "[LV2Gtk2]"
#define RG_NO_DEBUG_PRINT

#include "LV2Gtk2.h"

#include "LV2Gtk2So.h"

#include "misc/Debug.h"
#include "misc/Preferences.h"

#include <QObject>
#include <QMessageBox>
#include <QCheckBox>

#include <dlfcn.h>

#ifdef HAVE_GTK2


namespace Rosegarden
{


LV2Gtk2* LV2Gtk2::m_instance = nullptr;

LV2Gtk2* LV2Gtk2::getInstance()
{
    if (m_instance == nullptr) {
        RG_DEBUG << "creating instance";
        // warn user
        if (Preferences::getShowGtk2Warning()) {
            QCheckBox *cb = new QCheckBox(tr("Do not show this again"));
            QMessageBox msgbox;
            msgbox.setText(tr("You are about to load the gtk2 infrastructure. This has been know to cause crashes on some systems. Possible workarounds are to set environment variables for the Rosegarden program:<br/>set QT_QPA_PLATFORMTHEME to gtk2 and install the gtk2 style plugin or<br/>set XDG_SESSION_TYPE to unknown"));
            msgbox.setIcon(QMessageBox::Icon::Question);
            msgbox.addButton(QMessageBox::Ok);
            msgbox.addButton(QMessageBox::Cancel);
            msgbox.setDefaultButton(QMessageBox::Cancel);
            msgbox.setCheckBox(cb);

            const int result = msgbox.exec();
            if (result == QMessageBox::Cancel) return nullptr;
            if (cb->isChecked()) {
                Preferences::setShowGtk2Warning(false);
            }
        }
        m_instance = new LV2Gtk2;
    }
    return m_instance;
}

LV2Gtk2::LV2Gtk2()
{
    // load shared library
    m_dlib = dlopen("librosegardenGtk2.so", RTLD_LOCAL | RTLD_LAZY);
    RG_DEBUG << "getInstance lib:" << m_dlib;
    if (m_dlib) {
        void (*createLV2Gtk2SoPtr)();
        *(void **)(&createLV2Gtk2SoPtr) = dlsym(m_dlib, "createLV2Gtk2So");
        RG_DEBUG << "getInstance SoPtr:" << (void*)createLV2Gtk2SoPtr;
        if (createLV2Gtk2SoPtr) {
            createLV2Gtk2SoPtr();
        } else {
            RG_DEBUG << dlerror();
        }
    } else {
        // warn user that lib not found
        (void)QMessageBox::warning
            (this,
             QObject::tr("Rosegarden"),
             QObject::tr("The rosegarden gtk2 library (librosegardenGtk2.so) has not been found. Ensure that the library is in a standard location or is findable with the LD_LIBRARY_PATH environment variable."),
             QMessageBox::Ok);
    }
}

LV2Gtk2::~LV2Gtk2()
{
    RG_DEBUG << "dtor";
    if (m_dlib) dlclose(m_dlib);
}

LV2Gtk2Types::LV2Gtk2Widget LV2Gtk2::getWidget(
        LV2UI_Widget lv2Widget,
        LV2Gtk2Types::SizeCallback* sizecb) const
{
    RG_DEBUG << "getWidget" << &lv2Widget << sizecb;
    if (!m_dlib)
        return LV2Gtk2Types::LV2Gtk2Widget();

    // Get the function pointer.
    LV2Gtk2Types::LV2Gtk2Widget
        (*getWidgetPtr)(LV2UI_Widget lv2Widget,
                        LV2Gtk2Types::SizeCallback* sizecb);
    *(void **)(&getWidgetPtr) = dlsym(m_dlib, "getWidget");
    if (!getWidgetPtr) {
        RG_DEBUG << dlerror();
        return LV2Gtk2Types::LV2Gtk2Widget();
    }

    // Call it.
    return getWidgetPtr(lv2Widget, sizecb);
}

void LV2Gtk2::getSize(const LV2Gtk2Types::LV2Gtk2Widget& widget,
                      int& width,
                      int& height) const
{
    RG_DEBUG << "getSize" << &widget << width << height;
    if (!m_dlib) {
        width = 0;
        height = 0;
        return;
    }

    // Get the function pointer.
    void (*getSizePtr)(const LV2Gtk2Types::LV2Gtk2Widget& widget,
                       int& width,
                       int& height);
    *(void **)(&getSizePtr) = dlsym(m_dlib, "getSize");
    if (!getSizePtr) {
        RG_DEBUG << dlerror();
        width = 0;
        height = 0;
        return;
    }

    // Call it.
    getSizePtr(widget, width, height);
}

long int LV2Gtk2::getWinId(const LV2Gtk2Types::LV2Gtk2Widget& widget)
{
    RG_DEBUG << "getWinId" << &widget;
    if (!m_dlib)
        return 0;

    // Get the function pointer.
    int (*getWinIdPtr)(const LV2Gtk2Types::LV2Gtk2Widget& widget);
    *(void **)(&getWinIdPtr) = dlsym(m_dlib, "getWinId");
    if (!getWinIdPtr) {
        RG_DEBUG << dlerror();
        return 0;
    }

    // Call it.
    return getWinIdPtr(widget);
}

void LV2Gtk2::deleteWidget(const LV2Gtk2Types::LV2Gtk2Widget& widget)
{
    RG_DEBUG << "deleteWidget" << &widget;
    if (!m_dlib)
        return;

    // Get the function pointer.
    void (*deleteWidgetPtr)(const LV2Gtk2Types::LV2Gtk2Widget& widget);
    *(void **)(&deleteWidgetPtr) = dlsym(m_dlib, "deleteWidget");
    if (!deleteWidgetPtr) {
        RG_DEBUG << dlerror();
        return;
    }

    // Call it.
    deleteWidgetPtr(widget);
}

void LV2Gtk2::shutDown()
{
    RG_DEBUG << "shutDown";
    // Check m_instance directly to avoid getInstance() which will create
    // it.
    if (!m_instance) {
        RG_DEBUG << "LV2Gtk2 not active";
        return;
    }

    // instance exists

    m_instance->doShutDown();
    delete m_instance;
    m_instance = nullptr;
}

void LV2Gtk2::doShutDown()
{
    if (!m_dlib)
        return;

    // Get the function pointer.
    void (*shutDownPtr)();
    *(void **)(&shutDownPtr) = dlsym(m_dlib, "shutDown");
    if (!shutDownPtr) {
        RG_DEBUG << dlerror();
        return;
    }

    // Call it.
    shutDownPtr();
}


}


#else  // no gtk2 - dummy versions


namespace Rosegarden
{


LV2Gtk2* LV2Gtk2::getInstance()
{
    return nullptr;
}

LV2Gtk2::LV2Gtk2() :
    m_dlib(nullptr)
{
}

LV2Gtk2::~LV2Gtk2()
{
}

LV2Gtk2Types::LV2Gtk2Widget LV2Gtk2::getWidget(LV2UI_Widget,
                                               LV2Gtk2Types::SizeCallback*)
{
    LV2Gtk2Types::LV2Gtk2Widget ret;
    return ret;
}

void LV2Gtk2::getSize(const LV2Gtk2Types::LV2Gtk2Widget&, int&, int&) const
{
}

long int LV2Gtk2::getWinId(const LV2Gtk2Types::LV2Gtk2Widget&)
{
    return 0;
}

void LV2Gtk2::deleteWidget(const LV2Gtk2Types::LV2Gtk2Widget&)
{
}

void LV2Gtk2::shutDown()
{
}


}

#endif
