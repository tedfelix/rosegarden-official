/****************************************************************************
** Form interface generated from reading ui file 'guitarchordeditor.ui'
**
** Created: Sun Nov 13 13:07:07 2005
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef GUITARCHORDEDITOR_H
#define GUITARCHORDEDITOR_H

#include <qvariant.h>
#include <qpixmap.h>

#include <kstatusbar.h>
#include <kdialogbase.h>

#include "chordname.h"
#include "chord.h"
#include "chordmap.h"
#include "fingers.h"
#include <list>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class FingeringConstructor;
class QTabWidget;
class QWidget;
class QLabel;
class KComboBox;
class KLineEdit;
class KPushButton;
class ChordInfo;

class GuitarChordEditor : public KDialogBase
{
    Q_OBJECT

public:
    GuitarChordEditor( guitar::Guitar* g_ptr,
                       guitar::ChordMap* cMap,
                       QWidget* parent = 0 );

    ~GuitarChordEditor() {}

    QWidget* tab;

    /**
     * Set Chord object to be edited
     * @param  c_ptr Chord object pointer
     */
    void setChord (guitar::Chord* c_ptr);

public slots:
    virtual void saveChord();
    virtual void newAliasTab();
    virtual void toggleMode();

protected:
    guitar::FingeringConstructor* fingerConstPtr;
    QTabWidget* tabWidget;
    KStatusBar* m_status;

protected slots:

    virtual void clearDisplay();

private:

    // Variables
    guitar::ChordMap* m_map;
    QPixmap image0;

    typedef std::pair <QWidget*, ChordInfo*> InfoPair;
    typedef std::list <InfoPair> InfoMap;
    InfoMap m_infoMap;

    // Functions
    void createTab ( QString const& name );
    guitar::ChordName* getChordName (void);

    // Present chord being modified
    guitar::Chord* m_old_chord_ptr;

};

#endif // GUITARCHORDEDITOR_H
