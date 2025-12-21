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

#ifndef RG_LV2GTK_H
#define RG_LV2GTK_H

#include <lv2/ui/ui.h>

#include "LV2GtkTypes.h"

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
 * See LV2GtkImpl which is the shared object (librosegardenGtk.so) that is
 * loaded dynamically to provide the actual functionality for this wrapper.
 * If it is not present or cannot be loaded due to gtk2 not being available,
 * it will fail gracefully with pop-ups explaining the situation to the user.
 */
class LV2Gtk : public QWidget
{
    Q_OBJECT
public:
    static LV2Gtk *getInstance();

    LV2Gtk(LV2Gtk &other) = delete;
    void operator=(const LV2Gtk &) = delete;

 private:
    LV2Gtk();
    ~LV2Gtk();

 public:
    /// Get a widget for a GTK-based LV2 plugin to use for its main widget.
    /**
     * Wrapper around gtk_window_new().
     */
    LV2GtkTypes::LV2GtkWidget getWidget(LV2UI_Widget lv2Widget,
                                        LV2GtkTypes::SizeCallback* sizecb);
    void getSize(const LV2GtkTypes::LV2GtkWidget& widget,
                 int& width,
                 int& height) const;
    long int getWinId(const LV2GtkTypes::LV2GtkWidget& widget);
    void deleteWidget(const LV2GtkTypes::LV2GtkWidget& widget);

    static void shutDown();
    void doShutDown();

private:
    static LV2Gtk* m_instance;

    void* m_dlib;
};


}

#endif
