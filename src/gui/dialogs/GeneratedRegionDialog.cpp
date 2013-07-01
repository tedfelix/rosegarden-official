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

#include "GeneratedRegionDialog.h"
#include "base/Segment.h"
#include "base/figuration/SegmentFigData.h"
#include "document/Command.h"
#include "misc/Strings.h"
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>

namespace Rosegarden
{
   //
GeneratedRegionDialog::
GeneratedRegionDialog(QWidget *parent,
		      NotePixmapFactory */*npf*/,
		      GeneratedRegion defaultGeneratedRegion,
                      QString commandName) :
        QDialog(parent),
        m_generatedRegion(defaultGeneratedRegion),
        m_command(new MacroCommand(commandName))
{
  setModal(true);
  setWindowTitle(tr("Generated region"));
  resize(328, 247);

  QDialogButtonBox *buttonBox;
  QLabel *label;
  QLabel *label_2;


  label = new QLabel(this);
  label->setGeometry(QRect(10, 30, 111, 20));
  label->setText(tr("Figuration source"));
  m_figSourcesBox = new QComboBox(this);
  m_figSourcesBox->setGeometry(QRect(100, 30, 200, 22));
  label_2 = new QLabel(this);
  label_2->setGeometry(QRect(10, 110, 81, 16));
  label_2->setText(tr("Chord source"));
  m_chordSourcesBox = new QComboBox(this);
  m_chordSourcesBox->setGeometry(QRect(100, 110, 200, 22));

  buttonBox = new QDialogButtonBox(this);
  buttonBox->setGeometry(QRect(-80, 190, 341, 32));
  buttonBox->setOrientation(Qt::Horizontal);
  buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

  initializeCombos();

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  connect(m_figSourcesBox,   SIGNAL(currentIndexChanged(int)),
	  this, SLOT(assignFigurationSource(int)));
  connect(m_chordSourcesBox, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(assignChordSource(int)));
}

void
GeneratedRegionDialog::
initializeCombos()
{
  typedef SegmentFigData::SegmentFigDataMap SegMap;
  typedef SegMap::iterator iterator;

  SegMap involvedSegments = SegmentFigData::getInvolvedSegments(false, m_command);

  for (iterator i = involvedSegments.begin();
       i != involvedSegments.end();
       ++i) {
      Segment *s = i->first;
      SegmentFigData &data = i->second;
      if (data.isa(SegmentFigData::FigurationSource)) {
          m_figSourcesBox->addItem (strtoqstr(s->getLabel()), data.getID());
      }
      if (data.isa(SegmentFigData::ChordSource)) {
          m_chordSourcesBox->addItem (strtoqstr(s->getLabel()), data.getID());
      }
  }

  initComboToID(m_figSourcesBox, m_generatedRegion.getFigurationSourceID());
  initComboToID(m_chordSourcesBox, m_generatedRegion.getChordSourceID());
}

void
GeneratedRegionDialog::
initComboToID(QComboBox* comboBox, int id)
{
  int index = comboBox->findData(id);
  comboBox->setCurrentIndex(index);
}
    
void
GeneratedRegionDialog::
assignChordSource(int itemIndex)
{
  if (itemIndex < 0) { return; }
  bool ok;
  int id = m_chordSourcesBox->itemData(itemIndex).toInt(&ok);
  if (!ok) { return; }
  m_generatedRegion.setChordSourceID(id);
}
  
void
GeneratedRegionDialog::
assignFigurationSource(int itemIndex)
{
  if (itemIndex < 0) { return; }
  bool ok;
  int id = m_figSourcesBox->itemData(itemIndex).toInt(&ok);
  if (!ok) { return; }
  m_generatedRegion.setFigurationSourceID(id);
}

}

#include "GeneratedRegionDialog.moc"
