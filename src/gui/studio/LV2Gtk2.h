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

#ifndef RG_LV2GTK2_H
#define RG_LV2GTK2_H

#include <lv2/ui/ui.h>

#include "LV2Gtk2Types.h"

#include <QWidget>


namespace Rosegarden
{


/// Singleton wrapper around GTK2.
/**
 * Rosegarden uses this to get to GTK2 without forcing a static dependency
 * on GTK2.  GTK2 has proven to be problematic and isn't always included in
 * distros nowadays.
 *
 * GTK2 is required by some LV2 plugins (e.g. Calf).
 *
 * See LV2Gtk2So which is the shared object (librosegardenGtk2.so) that is
 * loaded dynamically to provide the actual functionality for this wrapper.
 * If it is not present or cannot be loaded due to gtk2 not being available,
 * it will fail gracefully with pop-ups explaining the situation to the user.
 */
class LV2Gtk2 : public QWidget
{
    Q_OBJECT
public:
    static LV2Gtk2 *getInstance();

    LV2Gtk2(LV2Gtk2 &) = delete;
    void operator=(const LV2Gtk2 &) = delete;

 private:
    // Singleton.
    LV2Gtk2();
    ~LV2Gtk2();

 public:
    /// Get a widget for a GTK-based LV2 plugin to use for its main widget.
    /**
     * Wrapper around gtk_window_new().
     */
    LV2Gtk2Types::LV2Gtk2Widget getWidget(LV2UI_Widget lv2Widget,
                                          LV2Gtk2Types::SizeCallback* sizecb) const;
    /// gtk_widget_get_allocation()
    void getSize(const LV2Gtk2Types::LV2Gtk2Widget& widget,
                 int& width,
                 int& height) const;
    /// GDK_WINDOW_XWINDOW()
    long int getWinId(const LV2Gtk2Types::LV2Gtk2Widget& widget);
    /// gtk_widget_destroy()
    void deleteWidget(const LV2Gtk2Types::LV2Gtk2Widget& widget);

    static void shutDown();
    void doShutDown();

private:
    // The Singleton instance.
    static LV2Gtk2* m_instance;

    // The .so shared library file.
    void* m_dlib;
};


}

#endif
