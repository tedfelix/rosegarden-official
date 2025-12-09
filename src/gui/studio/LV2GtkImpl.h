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

#ifndef RG_LV2GTKIMPL_H
#define RG_LV2GTKIMPL_H

#include <lv2/ui/ui.h>

#include "ILV2Gtk.h"

namespace Rosegarden
{

/// Wrapper around GTK that can be used for LV2 plugins that use GTK.
namespace LV2GtkImpl
{

extern "C" __attribute__((visibility("default")))
void createLV2GtkImpl();

extern "C" __attribute__((visibility("default")))
ILV2Gtk::LV2GtkWidget getWidget(LV2UI_Widget lv2Widget,
                                ILV2Gtk::SizeCallback* sizecb);

extern "C" __attribute__((visibility("default")))
void getSize(const ILV2Gtk::LV2GtkWidget& widget,
             int& width,
             int& height);
extern "C" __attribute__((visibility("default")))
long int getWinId(const ILV2Gtk::LV2GtkWidget& widget);
extern "C" __attribute__((visibility("default")))
void deleteWidget(const ILV2Gtk::LV2GtkWidget& widget);
extern "C" __attribute__((visibility("default")))
void shutDown();

}

}

#endif
