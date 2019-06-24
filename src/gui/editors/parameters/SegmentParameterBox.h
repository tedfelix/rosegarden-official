/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTPARAMETERBOX_H
#define RG_SEGMENTPARAMETERBOX_H

#include "gui/widgets/ColourTable.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"  // For MidiByte
#include "RosegardenParameterBox.h"
#include "base/Selection.h"  // For SegmentSelection

class QComboBox;
class QPushButton;
class QString;
class QWidget;

#include <vector>


namespace Rosegarden
{


class Command;
class TristateCheckBox;
class Label;
class Segment;
class RosegardenDocument;


class SegmentParameterBox : public RosegardenParameterBox,
                            public CompositionObserver
{

    Q_OBJECT

public:

    // ??? Get document directly.
    SegmentParameterBox(RosegardenDocument *doc,
                        QWidget *parent);
    ~SegmentParameterBox() override;

    // ??? Get document directly.
    void setDocument(RosegardenDocument *doc);

    void useSegments(const SegmentSelection &segments);

    // CompositionObserver interface
    void segmentRemoved(const Composition *,
                        Segment *) override;

public slots:
    // ??? It would be nice if there were an ActionFileClient::createAction()
    //     that takes a function pointer instead of a QString/SLOT().
    void slotRepeatPressed();

    void slotTransposeSelected(int);
    void slotTransposeTextChanged(const QString &);

    void slotDelaySelected(int);
    void slotDelayTimeChanged(timeT delayValue);
    void slotDelayTextChanged(const QString &);

    void slotEditSegmentLabel();

    void slotColourSelected(int);
    void slotDocColoursChanged();

    void slotAudioFadeChanged(int);
    void slotFadeInChanged(int);
    void slotFadeOutChanged(int);

    void slotHighestPressed();
    void slotLowestPressed();

    void slotChangeLinkTranspose();
    void slotResetLinkTranspose();

    // ??? Shadow.  Does this need to be a slot?
    virtual void update();

signals:
    void documentModified();
    void canvasModified();

private slots:
    void slotNewDocument(RosegardenDocument *doc);
    void slotQuantizeSelected(int);

private:
    void initBox();
    void populateBoxFromSegments();
    void updateHighLow();
    void addCommandToHistory(Command *command);

    Label *m_label;
    QPushButton *m_editButton;
    TristateCheckBox *m_repeatCheckBox;
    QComboBox *m_quantizeComboBox;
    QComboBox *m_transposeComboBox;
    QComboBox *m_delayComboBox;
    QComboBox *m_colourComboBox;

    int m_addColourPos;

    // used to keep track of highest/lowest as there is no associated spinbox
    // to query for its value
    int m_highestPlayable;
    int m_lowestPlayable;

    // ??? Can we access the selection directly instead of keeping a
    //     copy?  Of pointers?  That would make this code significantly
    //     safer.
    typedef std::vector<Segment *> SegmentVector;
    SegmentVector m_segments;

    std::vector<timeT> m_standardQuantizations;
    std::vector<timeT> m_delays;
    std::vector<int> m_realTimeDelays;
    ColourTable::ColourList  m_colourList;

    RosegardenDocument *m_doc;

    MidiByte m_transposeRange;
};



}

#endif
