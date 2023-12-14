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

#include "LV2Gtk.h"

#ifdef HAVE_GTK2

// gtk can give wanings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtk/gtk.h>
#pragma GCC diagnostic pop

#include <gdk/gdkx.h>

//#define GTK_DEBUG true
#define GTK_DEBUG false
#define debug_print(...) \
            do { if (GTK_DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

namespace {
    void size_request(GtkWidget*, GtkRequisition *req, gpointer user_data)
    {
        debug_print("size_request %d %d\n", req->width, req->height);
        Rosegarden::LV2Gtk::SizeCallback* sizecb =
            (Rosegarden::LV2Gtk::SizeCallback*)user_data;
        sizecb->setSize(req->width, req->height, true);
    }

    void size_allocate(GtkWidget*, GdkRectangle *rect, gpointer user_data)
    {
        debug_print("size_allocate %d %d\n", rect->width, rect->height);
        Rosegarden::LV2Gtk::SizeCallback* sizecb =
            (Rosegarden::LV2Gtk::SizeCallback*)user_data;
        sizecb->setSize(rect->width, rect->height, false);
    }

    gboolean delete_event(GtkWidget *,
                          GdkEvent  *,
                          gpointer   )
    {
        debug_print("gtk delete event occurred\n");

        return FALSE;
    }

    static void destroy(GtkWidget *,
                        gpointer   )
    {
        debug_print("gtk destroy\n");
    }

}

namespace Rosegarden
{

LV2Gtk::LV2Gtk() :
    m_active(false),
    m_argv(nullptr)
{
    debug_print("gtk constructor\n");
}

LV2Gtk::~LV2Gtk()
{
    if (m_argv) {
        int i = 0;
        while (m_argv[i]) {
            free(m_argv[i]);
            i++;
        }
        delete[] m_argv;
    }
}

void LV2Gtk::tick() const
{
    if (m_active) {
        while (gtk_events_pending()) gtk_main_iteration();
    }
}

LV2Gtk::LV2GtkWidget LV2Gtk::getWidget(LV2UI_Widget lv2Widget,
                                       SizeCallback* sizecb)
{
    debug_print("gtk getWidget\n");
    if (!m_active) {
        // gtk start up on demand
        //printf("starting gtk\n");
        startUp();
        m_active = true;
    }
    GtkWidget* wid = (GtkWidget*)lv2Widget;
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), 0);
    gtk_container_add (GTK_CONTAINER(window), wid);
    gtk_widget_show_all(window);
    g_signal_connect(G_OBJECT(window), "size-request",
                     G_CALLBACK(size_request), sizecb);
    g_signal_connect(G_OBJECT(window), "size-allocate",
                     G_CALLBACK(size_allocate), sizecb);
    g_signal_connect(G_OBJECT(window), "delete_event",
                     G_CALLBACK(delete_event), NULL);
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(destroy), NULL);
    LV2GtkWidget ret;
    ret.window = window;
    return ret;
}

void LV2Gtk::getSize(const LV2GtkWidget& widget, int& width, int& height) const
{
    debug_print("gtk getSize\n");
    GtkAllocation alloc;
    gtk_widget_get_allocation((GtkWidget*)(widget.window), &alloc);
    width = alloc.width;
    height = alloc.height;
    //printf("getSize %d %d\n", width, height);
}

long int LV2Gtk::getWinId(const LV2GtkWidget& widget)
{
    debug_print("gtk getWinId\n");
    unsigned long id = GDK_WINDOW_XWINDOW(((GtkWidget*)(widget.window))->window);
    return id;
}

void LV2Gtk::deleteWidget(const LV2GtkWidget& widget)
{
    debug_print("gtk deleteWidget\n");
    if (widget.window) {
        gtk_widget_destroy((GtkWidget*)(widget.window));
    }
}

void LV2Gtk::startUp()
{
    debug_print("gtk startUp\n");
    int argc = 1;
    m_argv = new char*[2];
    m_argv[0] = strdup("lv2gtk");
    m_argv[1] = nullptr;
    gtk_init (&argc, &m_argv);
}

}
#else

// no gtk2 - dummy versions
namespace Rosegarden
{

LV2Gtk::LV2Gtk() :
    m_active(false),
    m_argv(nullptr)
{
}

LV2Gtk::~LV2Gtk()
{
}

// cppcheck-suppress unusedFunction
void LV2Gtk::tick() const
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

void LV2Gtk::startUp()
{
}

}
#endif
