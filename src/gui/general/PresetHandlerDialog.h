/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PRESETHANDLERDIALOG_H
#define RG_PRESETHANDLERDIALOG_H

#include "CategoryElement.h"

#include <QDialog>
#include <QRadioButton>
#include <QString>

class QWidget;
class QComboBox;


namespace Rosegarden
{

class PresetGroup;

/** Creates a dialog to allow the user to dial in an instrument category, an
 * instrument, and a player ability.
 *
 * Widgets are populated from the XML file generated from the data provided by
 * E. Magnus Johannson, and describe an array of real-world instruments.  These
 * are used to aid composers in writing viable parts.  Internally, the data is
 * parsed from XML and stored into a series of container classes that are used
 * to populate the combo boxes used in this dialog, but users of the dialog
 * don't need to worry about any of these implementation details.
 *
 * \sa { PresetElement, PresetGroup, CategoryElement }
 *
 * \author D. Michael McIntyre
 */
class PresetHandlerDialog : public QDialog
{
    Q_OBJECT

public:
    /** Create a PresetHandlerDialog with parent \c parent.  The \c fromNotation
     * property tells the dialog whether it is being launched off of the Load
     * button in the "Create segments with" section of the Track Parameters Box,
     * or from inside the notation editor.  When used in the former case, the
     * data retrieved from this dialog can be used to create pre-selected
     * segment parameters at a track level.  This should be done prior to
     * drawing or recording segments for the target instrument.  When used in
     * the latter case, this data retrieved from this dialog can be used to
     * convert notation from being suitable for one instrument to being suitable
     * for another one.
     */
    PresetHandlerDialog(QWidget* parent, bool fromNotation = false);

    /** Destroy the PresetHandlerDialog */
    ~PresetHandlerDialog();

    /** Allows access to the dialog's PresetGroup object
     *
     * \sa { PresetGroup }
     * */
    PresetGroup *m_presets;

    /** Allows access to the dialog's CategoriesContainer object
     *
     * \sa { CategoriesContainer }
     * */
    CategoriesContainer m_categories;

    /** Tell the dialog which mode to run in */
    bool m_fromNotation;

    //-------[ accessor functions ]------------------------

    /** Return the real world instrument name the user selected with the dialog
     *
     * This is used as the label for any future segments when this dialog is
     * launched from the Track Parameters box.
     */
    QString getName();

    /** Return the clef (index) that should be used when writing for a
     * real-world instrument of the type the user selected with the dialog
     */
    int getClef();

    /** Return the transpose value that should be used when writing for a
     * real-world instrument of the type the user selected with the dialog
     */
    int getTranspose();

    /** Return the lowest playable  MIDI pitche which will be used to show
     * unplayable notes in red if the notation editor is configured
     * appropriately.  The value returned will depend on what the user selected
     * for "Player ability."
     */
    int getLowRange();

    /** Return the highest playable  MIDI pitche which will be used to show
     * unplayable notes in red if the notation editor is configured
     * appropriately.  The value returned will depend on what the user selected
     * for "Player ability."
     */
    int getHighRange();

    /** Return whether the user intended to apply the new parameters to all
     * segments (on this track?)
     */
    bool getConvertAllSegments();

    /** Return whether the user intended to apply the new parameters to selected
     * segments only
     */
    bool getConvertOnlySelectedSegments();

protected:

    //--------[ member functions ]-------------------------
    
    // initialize the dialog
    void initDialog();

    // populate the category combo
    void populateCategoryCombo();


    //---------[ data members ]-----------------------------

    QComboBox   *m_categoryCombo;
    QComboBox   *m_instrumentCombo;
    QComboBox   *m_playerCombo;
    QRadioButton *m_convertSegments;
    QRadioButton *m_convertAllSegments;

protected slots:

    /// de-populate and re-populate the Instrument combo when the category
    /// changes.
    void slotCategoryIndexChanged(int index);

    /// write out settings to QSettings data for next time and call accept()
    void accept();

    void help();
}; // PresetHandlerDialog


}

#endif
