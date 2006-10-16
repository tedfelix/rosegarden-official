/****************************************************************************
** Form interface generated from reading ui file 'gui/ui/RosegardenTransport.ui'
**
** Created: Mon Oct 16 18:46:56 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.6   edited Aug 31 2005 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef ROSEGARDENTRANSPORT_H
#define ROSEGARDENTRANSPORT_H

#include <qvariant.h>
#include <qpixmap.h>
#include <qframe.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
namespace Rosegarden {
class Label;
}
class QPushButton;
class QLabel;

class RosegardenTransport : public QFrame
{
    Q_OBJECT

public:
    RosegardenTransport( QWidget* parent = 0, const char* name = 0 );
    ~RosegardenTransport();

    QFrame* RecordingFrame;
    QPushButton* PanelCloseButton;
    QPushButton* PanicButton;
    QPushButton* MetronomeButton;
    QFrame* MidiEventFrame;
    QLabel* InLabel;
    QLabel* OutLabel;
    QLabel* InDisplay;
    QLabel* OutDisplay;
    QPushButton* RecordButton;
    QPushButton* LoopButton;
    QPushButton* SoloButton;
    QPushButton* SetStartLPButton;
    QPushButton* SetStopLPButton;
    QFrame* MainFrame;
    QFrame* LCDBoxFrame;
    QLabel* UnitHoursPixmap;
    QLabel* HourColonPixmap;
    QLabel* TenMinutesPixmap;
    QLabel* UnitMinutesPixmap;
    QLabel* TenSecondsPixmap;
    QLabel* UnitSecondsPixmap;
    QLabel* HundredthsPixmap;
    QLabel* NegativePixmap;
    QLabel* TenHoursPixmap;
    QLabel* TimeSigLabel;
    QLabel* DivisionLabel;
    QLabel* DivisionDisplay;
    QLabel* TempoLabel;
    Rosegarden::Label* TimeSigDisplay;
    QLabel* ToEndLabel;
    QLabel* TimeDisplayLabel;
    QLabel* TenThousandthsPixmap;
    QLabel* MinuteColonPixmap;
    QLabel* SecondColonPixmap;
    QLabel* HundredthColonPixmap;
    QLabel* TenthsPixmap;
    QLabel* ThousandthsPixmap;
    Rosegarden::Label* TempoDisplay;
    QPushButton* TimeDisplayButton;
    QPushButton* PanelOpenButton;
    QPushButton* RewindButton;
    QPushButton* RewindEndButton;
    QPushButton* PlayButton;
    QPushButton* StopButton;
    QPushButton* FfwdButton;
    QPushButton* FfwdEndButton;
    QPushButton* ToEndButton;

protected:

protected slots:
    virtual void languageChange();

private:
    QPixmap image0;
    QPixmap image1;
    QPixmap image2;
    QPixmap image3;
    QPixmap image4;
    QPixmap image5;
    QPixmap image6;
    QPixmap image7;
    QPixmap image8;
    QPixmap image9;
    QPixmap image10;
    QPixmap image11;
    QPixmap image12;
    QPixmap image13;
    QPixmap image14;
    QPixmap image15;
    QPixmap image16;
    QPixmap image17;
    QPixmap image18;
    QPixmap image19;
    QPixmap image20;

};

#endif // ROSEGARDENTRANSPORT_H
