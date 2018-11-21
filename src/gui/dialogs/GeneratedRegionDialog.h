/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_GENERATEDREGIONDIALOG_H
#define RG_GENERATEDREGIONDIALOG_H

#include "base/NotationTypes.h"
#include "base/figuration/GeneratedRegion.h"
#include <QDialog>

class QWidget;
class QComboBox;
class QString;

namespace Rosegarden
{
class NotePixmapFactory;
class MacroCommand;

class GeneratedRegionDialog : public QDialog
{
  Q_OBJECT

  public:

  GeneratedRegionDialog(QWidget *parent,
			NotePixmapFactory */*npf*/,
			GeneratedRegion defaultGeneratedRegion,
			QString commandName);

  GeneratedRegion getGeneratedRegion() const
  { return m_generatedRegion; }
  MacroCommand *extractCommand(void) {
      MacroCommand *command = m_command;
      m_command = nullptr;
      return command;
  }
private slots:
  void assignChordSource(int itemIndex);
  void assignFigurationSource(int itemIndex);
    
protected:
  void initializeCombos(void);
  void initComboToID(QComboBox* comboBox, int id);
  //--------------- Data members ---------------------------------

  GeneratedRegion m_generatedRegion;
  // The command that this dialog implies.
  /**
   * This has to live here because while figuring out items for the
   * comboboxes, we update source tags, which can modify segments, so
   * what we do has to be undoable/redoable.
   */
  MacroCommand   *m_command;
  QComboBox *m_figSourcesBox;
  QComboBox *m_chordSourcesBox;
};



}

#endif /* ifndef RG_GENERATEDREGIONDIALOG_H */
