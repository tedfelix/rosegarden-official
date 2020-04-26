/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2019 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ColorCombo]"

#include "ColorCombo.h"

#include "base/ColourMap.h"
#include "base/Composition.h"
#include "gui/general/GUIPalette.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"

#include "misc/Debug.h"

#include <QPixmap>
#include <QString>

namespace Rosegarden
{


ColorCombo::ColorCombo(QWidget *parent) :
    QComboBox(parent)
{
    setEditable(false);
    setMaxVisibleItems(20);
}

void
ColorCombo::updateColors()
{
    // The color combobox is handled differently from the others.  Since
    // there are 420 strings of up to 25 chars in here, it would be
    // expensive to detect changes by comparing vectors of strings.

    // For now, we'll handle the document colors changed notification
    // and reload the combobox then.

    // See the comments on RosegardenDocument::docColoursChanged()
    // in RosegardenDocument.h.

    // Note that as of this writing (June 2019) there is no way
    // to modify the document colors.  See ColourConfigurationPage
    // which was probably meant to be used by DocumentConfigureDialog.
    // See TrackParameterBox::slotDocColoursChanged().

    clear();

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    if (!doc)
        return;

    // Populate it from Composition::m_segmentColourMap
    ColourMap temp = doc->getComposition().getSegmentColourMap();

    // For each color in the segment color map
    for (ColourMap::MapType::const_iterator colourIter = temp.colours.begin();
         colourIter != temp.colours.end();
         ++colourIter) {
        // Wrap in a tr() call in case the color is on the list of translated
        // color names we're including since 09.10.
        QString colourName(QObject::tr(colourIter->second.second.c_str()));

        QPixmap colourIcon(15, 15);
        colourIcon.fill(colourIter->second.first);

        if (colourName == "") {
            addItem(colourIcon, tr("Default"));
        } else {
            // truncate name to 25 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide (This expands from 15 because the
            // translators wrote books instead of copying the style of
            // TheShortEnglishNames, and because we have that much room to
            // spare.)
            if (colourName.length() > 25)
                colourName = colourName.left(22) + "...";

            addItem(colourIcon, colourName);
        }
    }

#if 0
// Removing this since it has never been in there.
    m_color->addItem(tr("Add New Color"));
    m_addColourPos = m_color->count() - 1;
#endif

}


}
