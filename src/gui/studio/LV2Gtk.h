/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

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


namespace Rosegarden
{


/// Wrapper around GTK that can be used for LV2 plugins that use GTK.
/**
 * Singleton.  See LV2Utils::getLV2Gtk().
 */
class LV2Gtk
{
public:
    LV2Gtk();
    ~LV2Gtk();

    class SizeCallback
    {
    public:
        virtual ~SizeCallback() {};
        virtual void setSize(int width, int height, bool isRequest) = 0;
    };

    struct LV2GtkWidget
    {
        void* window;
        LV2GtkWidget() {window = nullptr;}
    };

    // ??? This doesn't appear to be called by anyone.
    // should it be called ?
    // cppcheck-suppress functionStatic
#if 0
    void tick() const;
#endif

    /// Get a widget for a GTK-based LV2 plugin to use for its main widget.
    /**
     * Wrapper around gtk_window_new().
     */
    // cppcheck-suppress functionStatic
    LV2GtkWidget getWidget(LV2UI_Widget lv2Widget, SizeCallback* sizecb);

    // cppcheck-suppress functionStatic
    void getSize(const LV2GtkWidget& widget, int& width, int& height) const;
    static long int getWinId(const LV2GtkWidget& widget);
    static void deleteWidget(const LV2GtkWidget& widget);

private:
    // cppcheck-suppress functionStatic
    void startUp();

    bool m_active;
    char** m_argv;
};


}

#endif
