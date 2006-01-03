// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <qcheckbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qvbox.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qtooltip.h>

#include <kcombobox.h>

#include "Event.h" // for timeT

#define private protected // ugly hack but we want to access KProgressDialog::mShowTimer
#include <kprogress.h>
#undef private

class NotePixmapFactory;

/** Create out own check box which is always Tristate 
 * and allows us to click only between on and off
 * and only to _show_ the third ("Some") state 
 */
class RosegardenTristateCheckBox : public QCheckBox
{
Q_OBJECT
public:
    RosegardenTristateCheckBox(QWidget *parent=0,
                               const char *name=0):QCheckBox(parent, name)
        { setTristate(true) ;}

    virtual ~RosegardenTristateCheckBox();

protected:
    // don't emit when the button is released
    virtual void mouseReleaseEvent(QMouseEvent *);

private:
};


// A label that emits a double click signal and provides scroll wheel information.
//
//
class RosegardenLabel : public QLabel
{
Q_OBJECT
public:
    RosegardenLabel(QWidget *parent = 0, const char *name=0):
        QLabel(parent, name) {;}

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent * /*e*/)
        { emit doubleClicked(); }

    virtual void wheelEvent(QWheelEvent * e) 
        { emit scrollWheel(e->delta()); }

signals:
    void doubleClicked();
    void scrollWheel(int);

};

/**
 * A Combobox that just about handles doubles - you have
 * to set the precision outside of this class if you're
 * using it with Qt designer.  Urch.
 */
class RosegardenSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    RosegardenSpinBox(QWidget *parent = 0, const char *name=0);

    double getDoubleValue() const { return m_doubleValue; }

protected:
    virtual QString mapValueToText (int value);
    virtual int mapTextToValue(bool *ok);

    double m_doubleValue;
};


/**
 * Specialisation of QGroupBox that selects a slightly-smaller-
 * than-normal font size and draws its title in bold.  Not
 * terrifically exciting.
 */
class RosegardenParameterBox : public QGroupBox
{
    Q_OBJECT
public:
    RosegardenParameterBox(int strips,
                           Orientation orientation,
                           QString label,
			   QWidget *parent = 0,
			   const char *name = 0);

    RosegardenParameterBox(QString label,
			   QWidget *parent = 0,
			   const char *name = 0);

protected:
    void init();
    QFont m_font;
};

class RosegardenProgressDialog : public KProgressDialog
{
    Q_OBJECT
public:
    RosegardenProgressDialog(QWidget * creator = 0,
                             const char * name = 0,
                             bool modal = true);

    RosegardenProgressDialog(const QString &labelText,
                             int totalSteps,
                             QWidget *creator = 0,
                             const char *name = 0,
                             bool modal = true);

    ~RosegardenProgressDialog();

    /**
     * A "safe" way to process events without worrying about user
     * input during the process.  If there is a modal progress dialog
     * visible, then this will permit user input so as to allow the
     * user to hit Cancel; otherwise it will prevent all user input
     */
    static void processEvents();

    virtual void polish();

public slots:
    void slotSetOperationName(QString);
    void slotCancel();

    /// Stop and hide (if it's shown) the progress dialog
    void slotFreeze();

    /// Restore the dialog to its normal state
    void slotThaw();

protected slots:
    void slotCheckShow(int);

protected:
    virtual void hideEvent(QHideEvent*);

    //--------------- Data members ---------------------------------

    QTime m_chrono;
    bool m_wasVisible;
    bool m_frozen;
    bool m_modal;
    static bool m_modalVisible;
};

class RosegardenProgressBar : public KProgress
{
    Q_OBJECT

public:
    RosegardenProgressBar(int totalSteps,
			  bool useDelay,
			  QWidget *creator = 0,
			  const char *name = 0,
			  WFlags f = 0);

};

class CurrentProgressDialog : public QObject
{
    Q_OBJECT
public:
    static CurrentProgressDialog* getInstance();

    static RosegardenProgressDialog* get();
    static void set(RosegardenProgressDialog*);

    /**
     * Block the current progress so that it won't appear
     * regardless of passing time and occurring events.
     * This is useful when you want to show another dialog
     * and you want to make sure the progress dialog is out of the way
     */
    static void freeze();

    /**
     * Restores the progress dialog to its normal state atfer a freeze()
     */
    static void thaw();

public slots:
    /// Called then the current progress dialog is being destroyed
    void slotCurrentProgressDialogDestroyed();

protected:
    CurrentProgressDialog(QObject* parent, const char* name = 0)
        : QObject(parent, name) {}
    
    //--------------- Data members ---------------------------------
    static CurrentProgressDialog* m_instance;
    static RosegardenProgressDialog* m_currentProgressDialog;
};


class HZoomable
{
public:
    HZoomable() : m_hScaleFactor(1.0) {}

    void setHScaleFactor(double dy) { m_hScaleFactor = dy; }
    double getHScaleFactor()        { return m_hScaleFactor; }

protected:    
    double m_hScaleFactor;
};

// A Text popup - a tooltip we can control.
//
class RosegardenTextFloat : public QWidget
{
public:
    RosegardenTextFloat(QWidget *parent);
    virtual ~RosegardenTextFloat() {;}

    void setText(const QString &text);

    // Reparent the float correctly by context
    //
    void reparent(QWidget *newParent);

protected:
    virtual void paintEvent(QPaintEvent *e);

    QString m_text;
};



class RosegardenRotary : public QWidget
{
    Q_OBJECT
public:

    enum TickMode {
	NoTicks,        // plain circle with no marks for end points etc
	LimitTicks,     // marks at end points but not any intermediate points
	IntervalTicks,  // end points plus quarter, half, three-quarters
	PageStepTicks,  // end points plus every page-step interval
	StepTicks       // end points plus every step interval
    };

    RosegardenRotary(QWidget *parent,
                     float minValue = 0.0,
                     float maxValue = 100.0,
                     float step = 1.0,
                     float pageStep = 10.0,
                     float initialPosition = 50.0,
                     int size = 20,
		     TickMode ticks = NoTicks,
		     bool snapToTicks = false,
		     bool centred = false);
    ~RosegardenRotary();

    void setMinValue(float min) { m_minValue = min; }
    float getMinValue() const { return m_minValue; }

    void setMaxValue(float max) { m_maxValue = max; }
    float getMaxValue() const { return m_maxValue; }

    void setStep(float step) { m_step = step; }
    float getStep() const { return m_step; }

    void setPageStep(float step) { m_pageStep = step; }
    float getPageStep() const { return m_pageStep; }

    int getSize() const { return m_size; }

    // Position
    //
    float getPosition() const { return m_position; }
    void setPosition(float position);

    // Set the colour of the knob
    //
    void setKnobColour(const QColor &colour);
    QColor getKnobColour() const { return m_knobColour; }

signals:
    void valueChanged(float);

protected slots:
    void slotFloatTimeout();

protected:
    virtual void paintEvent(QPaintEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    void snapPosition();
    void drawPosition();
    void drawTick(QPainter &paint, double angle, int size, bool internal);

    float                m_minValue;
    float                m_maxValue;
    float                m_step;
    float                m_pageStep;
    int                  m_size;
    TickMode             m_tickMode;
    bool                 m_snapToTicks;
    bool                 m_centred;

    float                m_position;
    float                m_snapPosition;
    float                m_initialPosition;
    bool                 m_buttonPressed;
    int                  m_lastY;
    int                  m_lastX;

    QColor               m_knobColour;

    struct CacheIndex {

	CacheIndex(int _s, int _c, int _a, int _n, int _ct) :
	    size(_s), colour(_c), angle(_a), numTicks(_n), centred(_ct) { }

	bool operator<(const CacheIndex &i) const {
	    // woo!
	    if (size < i.size) return true;
	    else if (size > i.size) return false;
	    else if (colour < i.colour) return true;
	    else if (colour > i.colour) return false;
	    else if (angle < i.angle) return true;
	    else if (angle > i.angle) return false;
	    else if (numTicks < i.numTicks) return true;
	    else if (numTicks > i.numTicks) return false;
	    else if (centred == i.centred) return false;
	    else if (!centred) return true;
	    return false;
	}

	int          size;
	unsigned int colour;
	int          angle;
	int          numTicks;
	bool         centred;
    };

    typedef std::map<CacheIndex, QPixmap> PixmapCache;
    static PixmapCache m_pixmaps;
};

namespace Rosegarden { class Quantizer; }

class RosegardenQuantizeParameters : public QFrame
{
    Q_OBJECT
public:
    enum QuantizerType { Grid, Legato, Notation };

    RosegardenQuantizeParameters(QWidget *parent,
				 QuantizerType defaultQuantizer,
				 bool showNotationOption,
				 bool showAdvancedButton,
				 QString configCategory,
				 QString preamble = 0);
    
    /**
     * Returned quantizer object is on heap -- caller must delete.
     * Also writes values to KConfig if so requested in constructor.
     */
    Rosegarden::Quantizer *getQuantizer() const;

    QWidget *getAdvancedWidget() { return m_postProcessingBox; }

    bool shouldRebeam() const { return m_rebeam; }
    bool shouldDeCounterpoint() const { return m_deCounterpoint; }
    bool shouldMakeViable() const { return m_makeViable; }

    void showAdvanced(bool show);

public slots:
    void slotTypeChanged(int);
    void slotAdvancedChanged();

protected:
    QString m_configCategory;

    std::vector<Rosegarden::timeT> m_standardQuantizations;

    QGridLayout *m_mainLayout;

    KComboBox *m_typeCombo;

    QGroupBox *m_gridBox;
    QCheckBox *m_durationCheckBox;
    KComboBox *m_gridUnitCombo;
    QLabel    *m_swingLabel;
    KComboBox *m_swingCombo;
    QLabel    *m_iterativeLabel;
    KComboBox *m_iterativeCombo;

    QGroupBox *m_notationBox;
    QCheckBox *m_notationTarget;
    KComboBox *m_notationUnitCombo;
    KComboBox *m_simplicityCombo;
    KComboBox *m_maxTuplet;
    QCheckBox *m_counterpoint;

    QPushButton *m_advancedButton;
    QGroupBox *m_postProcessingBox;
    QCheckBox *m_articulate;
    QCheckBox *m_makeViable;
    QCheckBox *m_deCounterpoint;
    QCheckBox *m_rebeam;
};


class RosegardenPitchDragLabel : public QWidget
{
    Q_OBJECT
public:
    RosegardenPitchDragLabel(QWidget *parent,
			     int defaultPitch = 60);
    ~RosegardenPitchDragLabel();

    int getPitch() const { return m_pitch; }

    virtual QSize sizeHint() const;

signals:
    void pitchDragged(int);
    void pitchChanged(int); // mouse release
    void preview(int);

public slots:
    void slotSetPitch(int);
    
protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    void calculatePixmap(bool useSharps) const;

    mutable QPixmap m_pixmap;

    int m_pitch;
    int m_clickedY;
    int m_clickedPitch;
    bool m_clicked;

    NotePixmapFactory *m_npf;
};


class RosegardenPitchChooser : public QGroupBox
{
    Q_OBJECT
public:
    RosegardenPitchChooser(QString title,
			   QWidget *parent,
			   int defaultPitch = 60);
    
    int getPitch() const;

signals:
    void pitchChanged(int);
    void preview(int);

public slots:
    void slotSetPitch(int);
    void slotResetToDefault();

protected:
    int m_defaultPitch;
    RosegardenPitchDragLabel *m_pitchDragLabel;
    QSpinBox *m_pitch;
    QLabel *m_pitchLabel;
};

    
#endif // _WIDGETS_H_

