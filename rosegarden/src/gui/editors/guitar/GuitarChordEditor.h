#ifndef GUITARCHORDEDITOR_H
#define GUITARCHORDEDITOR_H

#include <qvariant.h>
#include <qpixmap.h>

#include <kstatusbar.h>
#include <kdialogbase.h>

#include "ChordName.h"
#include "Chord.h"
#include "ChordMap.h"
#include "Fingering.h"
#include <list>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QTabWidget;
class QWidget;
class QLabel;
class KComboBox;
class KLineEdit;
class KPushButton;

namespace Rosegarden
{

namespace Guitar
{ 
	class FingeringConstructor;
	class ChordInfo;
}

class GuitarChordEditor : public KDialogBase
{
    Q_OBJECT

public:
    GuitarChordEditor( Guitar::GuitarNeck* g_ptr,
                       Guitar::ChordMap* cMap,
                       QWidget* parent = 0 );

    ~GuitarChordEditor() {}

    QWidget* tab;

    /**
     * Set Chord object to be edited
     * @param  c_ptr Chord object pointer
     */
    void setChord (Guitar::Chord* c_ptr);

public slots:
    virtual void saveChord();
    virtual void newAliasTab();
    virtual void toggleMode();

protected:
    Guitar::FingeringConstructor* fingerConstPtr;
    QTabWidget* tabWidget;
    KStatusBar* m_status;

protected slots:

    virtual void clearDisplay();

private:

    // Variables
    Guitar::ChordMap* m_map;
    QPixmap image0;

    typedef std::pair <QWidget*, ChordInfo*> InfoPair;
    typedef std::list <InfoPair> InfoMap;
    InfoMap m_infoMap;

    // Functions
    void createTab ( QString const& name );
    Guitar::ChordName* getChordName (void);

    // Present chord being modified
    Guitar::Chord* m_old_chord_ptr;

};

}

#endif // GUITARCHORDEDITOR_H

