/* $Id: klearlook.cpp,v 1.25 2006/04/26 18:55:41 jck Exp $

Klearlook (C) Joerg C. Koenig, 2005 jck@gmx.org

----

Based upon QtCurve (C) Craig Drummond, 2003 Craig.Drummond@lycos.co.uk
    Bernhard Rosenkr�zer <bero@r?dh?t.com>
    Preston Brown <pbrown@r?dh?t.com>
    Than Ngo <than@r?dh?t.com>

Released under the GNU General Public License (GPL) v2.

----

B???Curve is based on the KDE Light style, 2nd revision:
Copyright(c)2000-2001 Trolltech AS (info@trolltech.com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files(the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <kdeversion.h>
#include <qmenubar.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qdrawutil.h>
#include <qscrollbar.h>
#include <qtabbar.h>
#include <qtabwidget.h>
#include <qguardedptr.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qbitmap.h>
#include <qcleanuphandler.h>
#include <qimage.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qstylefactory.h>
#include <qcleanuphandler.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qtoolbutton.h>
#include <qtoolbar.h>
#include <qprogressbar.h>
#include <qcursor.h>
#include <qheader.h>
#include <qwidgetstack.h>
#include <qsplitter.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "klearlook.h"
#include <qsettings.h>

#if KDE_VERSION >= 0x30200
#include <qfile.h>
#include <qdir.h>
#endif

// Uncomment the following to enable gradients in toolbars and menubars
// NOTE: Not yet complete!!!
//#define QTC_GRADIENT_TOOLBARS_AND_MENUBARS

//static int HIGH_LIGHT_FACTORS[]={ 100, 100, 100, 101, 102, 103, 104, 105 /*def*/, 106, 107, 108 };
//#define HIGHLIGHT_FACTOR(X) (X<0||X>10) ? 105 : HIGH_LIGHT_FACTORS[X]
#define QTC_HIGHLIGHT_FACTOR 105
#define QTC_BORDERED_FRAME_WIDTH 2
#define QTC_DEF_FRAME_WIDTH 1 
//#define QTC_MIN_BTN_SIZE 10
#define QTC_NO_SECT -1

#define MENU_POPUP_ITEM_HIGH_HI  7
#define MENU_POPUP_ITEM_HIGH_LO  5 
//#define MENU_POPUP_SHADOW

#define POS_DIV(a, b)  ( (a)/(b) +  ( ( (a) % (b) >= (b)/2 ) ? 1 : 0 ) )

static const int itemHMargin = 6;
static const int itemFrame = 2;
static const int arrowHMargin = 6;
static const int rightBorder = 12;

#if KDE_VERSION >= 0x30200 
// Try to read $KDEHOME/share/config/kickerrc to find out if kicker is transparent...

static QString readEnvPath( const char *env ) {
    QCString path = getenv( env );

    return path.isEmpty()
           ? QString::null
           : QFile::decodeName( path );
}

static bool kickerIsTrans() {
    QString kdeHome( readEnvPath( getuid() ? "KDEHOME" : "KDEROOTHOME" ) ),
    cfgFileName;
    bool trans = false;

    if ( kdeHome.isEmpty() )
        cfgFileName = QDir::homeDirPath() + "/.kde/share/config/kickerrc";
    else
        cfgFileName = QString( kdeHome ) + "/share/config/kickerrc";

    QFile cfgFile( cfgFileName );

    if ( cfgFile.open( IO_ReadOnly ) ) {
        QTextStream stream( &cfgFile );
        QString line;
        bool stop = false,
                    inGen = false;

        while ( !stream.atEnd() && !stop ) {
            line = stream.readLine();

            if ( inGen ) {
                if ( 0 == line.find( "Transparent=" ) )    // Found it!
                {
                    if ( -1 != line.find( "true" ) )
                        trans = true;
                    stop = true;
                } else if ( line[ 0 ] == QChar( '[' ) )     // Then wasn't in General section...
                    stop = true;
            } else if ( 0 == line.find( "[General]" ) )
                inGen = true;
        }
        cfgFile.close();
    }

    return trans;
}
#endif

inline int limit( double c ) {
    return c < 0.0
           ? 0
           : c > 255.0
           ? 255
           : ( int ) c;
}

inline QColor midColor( const QColor &a, const QColor &b, double factor = 1.0 ) {
    return QColor( ( a.red() + limit( b.red() * factor ) ) >> 1,
                   ( a.green() + limit( b.green() * factor ) ) >> 1,
                   ( a.blue() + limit( b.blue() * factor ) ) >> 1 );
}

// Copied from Keramik...
static bool isFormWidget( const QWidget *widget ) {
    //Form widgets are in the KHTMLView, but that has 2 further inner levels
    //of widgets - QClipperWidget, and outside of that, QViewportWidget
    QWidget * potentialClipPort = widget->parentWidget();

    if ( !potentialClipPort || potentialClipPort->isTopLevel() )
        return false;

    QWidget *potentialViewPort = potentialClipPort->parentWidget();

    if ( !potentialViewPort || potentialViewPort->isTopLevel() || qstrcmp( potentialViewPort->name(), "qt_viewport" ) )
        return false;

    QWidget *potentialKHTML = potentialViewPort->parentWidget();

    if ( !potentialKHTML || potentialKHTML->isTopLevel() || qstrcmp( potentialKHTML->className(), "KHTMLView" ) )
        return false;

    return true;
}

static void rgb2hls( double *r, double *g, double *b ) 
{
	double min;
	double max;
	double red;
	double green;
	double blue;
	double h, l, s;
	double delta;
	
	red = *r;
	green = *g;
	blue = *b;

	if (red > green)
	{
		if (red > blue)
			max = red;
		else
			max = blue;
	
		if (green < blue)
			min = green;
		else
			min = blue;
	}
	else
	{
		if (green > blue)
			max = green;
		else
			max = blue;
	
		if (red < blue)
			min = red;
		else
			min = blue;
	}

	l = (max + min) / 2;
	s = 0;
	h = 0;

	if (max != min)
	{
		if (l <= 0.5)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2 - max - min);
	
		delta = max -min;
		if (red == max)
			h = (green - blue) / delta;
		else if (green == max)
			h = 2 + (blue - red) / delta;
		else if (blue == max)
			h = 4 + (red - green) / delta;
	
		h *= 60;
		if (h < 0.0)
			h += 360;
	}

	*r = h;
	*g = l;
	*b = s;
}


static void hls2rgb( double *h, double *l, double *s ) {
	double hue;
	double lightness;
	double saturation;
	double m1, m2;
	double r, g, b;
	lightness = *l;
	saturation = *s;

	if (lightness <= 0.5)
		m2 = lightness * (1 + saturation);
	else
		m2 = lightness + saturation - lightness * saturation;
		
	m1 = 2 * lightness - m2;

	if (saturation == 0)
	{
		*h = lightness;
		*l = lightness;
		*s = lightness;
	}
	else
	{
		hue = *h + 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;
	
		if (hue < 60)
			r = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			r = m2;
		else if (hue < 240)
			r = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			r = m1;
	
		hue = *h;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;
	
		if (hue < 60)
			g = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			g = m2;
		else if (hue < 240)
			g = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			g = m1;
	
		hue = *h - 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;
	
		if (hue < 60)
			b = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			b = m2;
		else if (hue < 240)
			b = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			b = m1;
	
		*h = r;
		*l = g;
		*s = b;
	}

}

static bool equal( double d1, double d2 ) {
    return ( fabs( d1 - d2 ) < 0.0001 );
}

static void shade( const QColor &a, QColor *b, float k ) {

    if ( equal( k, 1.0 ) )
        * b = a;
    else {
        double red = a.red() / 256.0,
               green = a.green() / 256.0,
               blue = a.blue() / 256.0;

        rgb2hls( &red, &green, &blue );

        green *= k;
        if ( green > 1.0 )
            green = 1.0;
        else if ( green < 0.0 )
            green = 0.0;

        blue *= k;
        if ( blue > 1.0 )
            blue = 1.0;
        else if ( blue < 0.0 )
            blue = 0.0;

        hls2rgb( &red, &green, &blue );

        b->setRgb( limit( red * 256 ), limit( green * 256 ), limit( blue * 256 ) );
    }
}

static void shadeGradient( const QColor &base, QColor *vals ) {
    vals[ KlearlookStyle::GRADIENT_BASE ] = base;
    
    shade( vals[ KlearlookStyle::GRADIENT_BASE ],
        &( vals[ KlearlookStyle::GRADIENT_TOP ] ), SHADE_GRADIENT_TOP );
        
    shade( vals[ KlearlookStyle::GRADIENT_BASE ],
        &( vals[ KlearlookStyle::GRADIENT_BOTTOM ] ), SHADE_GRADIENT_BOTTOM );
        
    shade( vals[ KlearlookStyle::GRADIENT_BASE ],
        &( vals[ KlearlookStyle::GRADIENT_LIGHT ] ), SHADE_GRADIENT_LIGHT );
    shade( vals[ KlearlookStyle::GRADIENT_BASE ],
        &( vals[ KlearlookStyle::GRADIENT_DARK ] ), SHADE_GRADIENT_DARK );
}

static void drawLines( QPainter *p, const QRect &r, bool horiz, int nLines, int offset, const QColor *cols,
                       int startOffset, bool etched, bool lightGradient ) {
    int space = ( nLines * 2 ) + ( nLines - 1 ),
                x = horiz ? r.x() : r.x() + ( ( r.width() - space ) >> 1 ),
                    y = horiz ? r.y() + ( ( r.height() - space ) >> 1 ) : r.y(),
                        x2 = r.x() + r.width() - 1,
                             y2 = r.y() + r.height() - 1,
                                  i,
                                  displacement = etched ? 1 : 0;

    if ( horiz ) {
        if ( startOffset && y + startOffset > 0 )
            y += startOffset;

	p->setPen( cols[ etched ? lightGradient ? 3 : 4 : 0 ] );
        for ( i = 0; i < space; i += 3 )
            p->drawLine( x + offset, y + i, x2 - ( offset + displacement ), y + i );

        p->setPen( cols[ etched ? 0 : lightGradient ? 3 : 4 ] );
        for ( i = 1; i < space; i += 3 )
            p->drawLine( x + offset + displacement, y + i - 2, x2 - offset, y + i  - 2);
    } else {
        if ( startOffset && x + startOffset > 0 )
            x += startOffset;

	p->setPen( cols[ etched ? lightGradient ? 3 : 4 : 0 ] );
        for ( i = 0; i < space; i += 3 )
            p->drawLine( x + i, y + offset, x + i, y2 - ( offset + displacement ) );

        p->setPen( cols[ etched ? 0 : lightGradient ? 3 : 4 ] );
        for ( i = 1; i < space; i += 3 )
            p->drawLine( x + i -2, y + offset + displacement, x + i -2, y2 - offset );
    }
}

inline QColor getFill( QStyle::SFlags flags, const QColor *use ) {
    return !( flags & QStyle::Style_Enabled )
           ? use[ 1 ]
           : flags & QStyle::Style_Down
           ? use[ 3 ]
           : flags & QStyle::Style_MouseOver
           ? flags & ( QStyle::Style_On | QStyle::Style_Sunken )
           ? use[ 3 ].light( QTC_HIGHLIGHT_FACTOR )
           : use[ NUM_SHADES ].light( QTC_HIGHLIGHT_FACTOR )
                   : flags & ( QStyle::Style_On | QStyle::Style_Sunken )
                   ? use[ 3 ]
                   : use[ NUM_SHADES ];
}

#ifdef USE_SINGLE_STYLE
KlearlookStyle::KlearlookStyle()
#else
KlearlookStyle::KlearlookStyle(
    bool gpm, bool bb, bool bf, bool round, EGroove st, h,
    bool ge, bool va, bool bdt, bool crlh, EDefBtnIndicator dbi, ETBarBorder tbb,
    ELvExpander lve, ELvLines lvl, bool lvd, bool ico, int popuplvl ) )
#endif
: KStyle( AllowMenuTransparency, WindowsStyleScrollBar ),
themedApp( APP_OTHER ),
#ifndef USE_SINGLE_STYLE
borderButton( bb ), borderFrame( bf ), rounded( round ), etchedSlider( etched ), appearance( ge ? APPEARANCE_GRADIENT : APPEARANCE_FLAT ),
pmProfile( PROFILE_SUNKEN ), vArrow( va ), boldDefText( bdt ), crLabelHighlight( crlh ), lvDark( lvd ),
defBtnIndicator( dbi ), sliderThumbs( st ), handles( h ), toolbarBorders( tbb ), lvExpander( lve ), lvLines( lvl ), menuIcons( ico ), borderSplitter( true ), popupmenuHighlightLevel(popuplvl)
#endif
#if KDE_VERSION >= 0x30200
isTransKicker( false ),
#endif
hover( HOVER_NONE ),
oldCursor( -1, -1 ),
formMode( false ),
hoverWidget( NULL ),
hoverSect( QTC_NO_SECT ) {
    QSettings s;

    contrast = s.readNumEntry( "/Qt/KDE/contrast", 7 );
    if ( contrast < 0 || contrast > 10 )
        contrast = 7;
#ifdef USE_SINGLE_STYLE

    borderButton = borderFrame = s.readBoolEntry( "/klearlookstyle/Settings/border", true );
    rounded = borderButton ? s.readBoolEntry( "/klearlookstyle/Settings/round", true ) : false;
    menuIcons = s.readBoolEntry( "/klearlookstyle/Settings/icons", true );
    darkMenubar = s.readBoolEntry( "/klearlookstyle/Settings/darkMenubar", true );
    popupmenuHighlightLevel = s.readNumEntry( "/klearlookstyle/Settings/popupmenuHighlightLevel", 3);

    QString tmp = s.readEntry( "/klearlookstyle/Settings/toolbarBorders", QString::null );
    toolbarBorders = tmp.isEmpty()
                     ? TB_LIGHT
                     : qtc_to_tbar_border( tmp.latin1() );

    bool etched = s.readBoolEntry( "/klearlookstyle/Settings/etched", true );

    tmp = s.readEntry( "/klearlookstyle/Settings/sliderThumbs", QString::null );
    sliderThumbs = tmp.isEmpty()
                   ? etched ? GROOVE_SUNKEN : GROOVE_RAISED
               : qtc_to_groove( tmp.latin1() );

    tmp = s.readEntry( "/klearlookstyle/Settings/lvExpander", QString::null );
    lvExpander = tmp.isEmpty()
                 ? LV_EXP_ARR
                 : qtc_to_lv_expander( tmp.latin1() );

    tmp = s.readEntry( "/klearlookstyle/Settings/lvLines", QString::null );
    lvLines = tmp.isEmpty()
              ? LV_LINES_SOLID
              : qtc_to_lv_lines( tmp.latin1() );
              
    
    lvDark = s.readBoolEntry( "/klearlookstyle/Settings/lvDark", false );
    handles = qtc_to_groove( s.readEntry( "/klearlookstyle/Settings/sliderThumbs", DEF_HANDLE_STR ).latin1() );
    
    if ( GROOVE_NONE == handles )
        handles = GROOVE_RAISED;
        
    appearance = qtc_to_appearance(
        s.readEntry( "/klearlookstyle/Settings/appearance", DEF_APPEARANCE_STR ).latin1() );
    pmProfile = qtc_to_profile( s.readEntry( "/klearlookstyle/Settings/pm", DEF_PROFILE_STR ).latin1() );
    vArrow = s.readBoolEntry( "/klearlookstyle/Settings/vArrow", false );
    boldDefText = s.readBoolEntry( "/klearlookstyle/Settings/embolden", false );
    crLabelHighlight = s.readBoolEntry( "/klearlookstyle/Settings/crLabelHighlight", false );
    defBtnIndicator = qtc_to_ind(
        s.readEntry( "/klearlookstyle/Settings/defBtnIndicator", DEF_IND_STR ).latin1() );
        
    //if(!boldDefText && IND_NONE==defBtnIndicator)
    //    defBtnIndicator=IND_CORNER;
    
    borderSplitter = s.readBoolEntry( "/klearlookstyle/Settings/borderSplitter", false );
#endif

    if ( PROFILE_RAISED != pmProfile )
        shadeColors( qApp->palette().active().highlight(), menuPbar );
    else
        shadeGradient( qApp->palette().active().highlight(), menuPbar );
    shadeColors( qApp->palette().active().background(), gray );
    shadeColors( qApp->palette().active().button(), button );
}

void KlearlookStyle::polish( QApplication *app ) {
    if ( !qstrcmp( app->argv() [ 0 ], "kicker" ) || !qstrcmp( app->argv() [ 0 ], "appletproxy" ) ) {
        themedApp = APP_KICKER;
#if KDE_VERSION >= 0x30200

        isTransKicker = rounded && kickerIsTrans();
#endif

    } else if ( !qstrcmp( app->argv() [ 0 ], "korn" ) ) {
        themedApp = APP_KORN;
#if KDE_VERSION >= 0x30200

        isTransKicker = rounded && kickerIsTrans();
#endif

    } else
        themedApp = qstrcmp( qApp->argv() [ 0 ], "soffice.bin" ) ? APP_OTHER : APP_OPENOFFICE;
}

void KlearlookStyle::polish( QPalette &pal ) {
    int c = QSettings().readNumEntry( "/Qt/KDE/contrast", 7 );
    bool newContrast = false;

    if ( c < 0 || c > 10 )
        c = 7;

    if ( c != contrast ) {
        contrast = c;
        newContrast = true;
    }

    if ( newContrast || gray[ NUM_SHADES ] != qApp->palette().active().background() )
        shadeColors( qApp->palette().active().background(), gray );

    if ( newContrast || button[ NUM_SHADES ] != qApp->palette().active().button() )
        shadeColors( qApp->palette().active().button(), button );

    if ( PROFILE_RAISED == pmProfile ) {
        if ( newContrast || menuPbar[ NUM_SHADES ] != qApp->palette().active().highlight() )
            shadeColors( qApp->palette().active().highlight(), menuPbar );
    } else
        if ( qApp->palette().active().highlight() != menuPbar[ GRADIENT_BASE ] )
            shadeGradient( qApp->palette().active().highlight(), menuPbar );

    const QColorGroup &actGroup = pal.active(),
                                  &inactGroup = pal.inactive();
    const QColor *use = backgroundColors( actGroup );
    QColorGroup newAct( actGroup.foreground(), actGroup.button(),
        use[ 0 ], use[ 5 ], actGroup.mid(), actGroup.text(),
        actGroup.brightText(), actGroup.base(), actGroup.background() );

    newAct.setColor( QColorGroup::Highlight, actGroup.color( QColorGroup::Highlight ) );
    pal.setActive( newAct );

    use = backgroundColors( inactGroup );

    QColorGroup newInact( inactGroup.foreground(), inactGroup.button(),
        use[ 0 ], use[ 5 ], inactGroup.mid(), inactGroup.text(),
        inactGroup.brightText(), inactGroup.base(), inactGroup.background() );

    newInact.setColor( QColorGroup::Highlight, inactGroup.color( QColorGroup::Highlight ) );
    pal.setInactive( newInact );
}

static const char * kdeToolbarWidget = "kde toolbar widget";

void KlearlookStyle::polish( QWidget *widget ) {
    if ( ::qt_cast<QRadioButton *>( widget )
      || ::qt_cast<QCheckBox *>( widget )
      || ::qt_cast<QSpinWidget *>( widget )
      || widget->inherits( "QSplitterHandle" ) ) {
#if QT_VERSION >= 0x030200
        widget->setMouseTracking( true );
#endif

        widget->installEventFilter( this );
    } else if ( ::qt_cast<QButton *>( widget ) || ::qt_cast<QComboBox *>( widget ) ||
                widget->inherits( "QToolBarExtensionWidget" ) ) {
        widget->setBackgroundMode( QWidget::PaletteBackground );
        widget->installEventFilter( this );
        
    } else if ( ::qt_cast<QMenuBar *>( widget )
             || ::qt_cast<QToolBar *>( widget )
             || ::qt_cast<QPopupMenu *>( widget ) )
        widget->setBackgroundMode( QWidget::PaletteBackground );
        
    else if ( widget->inherits( "KToolBarSeparator" ) ) {
        widget->setBackgroundMode( QWidget::NoBackground );
        widget->installEventFilter( this );
        
    } else if ( ::qt_cast<QScrollBar *>( widget ) ) {
        widget->setMouseTracking( true );
        widget->installEventFilter( this );
        widget->setBackgroundMode( QWidget::NoBackground );
        
    } else if ( ::qt_cast<QSlider *>( widget ) || ::qt_cast<QHeader *>( widget ) ) {
        widget->setMouseTracking( true );
        widget->installEventFilter( this );
        
    } else if ( 0 == qstrcmp( widget->name(), kdeToolbarWidget ) ) {
        widget->installEventFilter( this );
        widget->setBackgroundMode( QWidget::NoBackground );  // We paint the whole background.
    }

    KStyle::polish( widget );
}

void KlearlookStyle::unPolish( QWidget *widget ) {
    if ( ::qt_cast<QRadioButton *>( widget ) ||
         ::qt_cast<QCheckBox *>( widget ) ||
         ::qt_cast<QSpinWidget *>( widget ) ||
            widget->inherits( "QSplitterHandle" ) ) {
#if QT_VERSION >= 0x030200
        widget->setMouseTracking( false );
#endif

        widget->removeEventFilter( this );
    } else if ( ::qt_cast<QButton *>( widget ) || ::qt_cast<QComboBox *>( widget ) ||
                widget->inherits( "QToolBarExtensionWidget" ) ) {
        widget->setBackgroundMode( QWidget::PaletteButton );
        widget->removeEventFilter( this );
        
    } else if ( ::qt_cast<QMenuBar *>( widget ) ||
                ::qt_cast<QToolBar *>( widget ) ||
                ::qt_cast<QPopupMenu *>( widget ) )
        widget->setBackgroundMode( QWidget::PaletteBackground );
        
    else if ( widget->inherits( "KToolBarSeparator" ) ) {
        widget->setBackgroundMode( PaletteBackground );
        widget->removeEventFilter( this );
        
    } else if ( ::qt_cast<QScrollBar *>( widget ) ) {
        widget->setMouseTracking( false );
        widget->removeEventFilter( this );
        widget->setBackgroundMode( QWidget::PaletteButton );
        
    } else if ( ::qt_cast<QSlider *>( widget ) ||
                ::qt_cast<QHeader *>( widget ) ) {
        widget->setMouseTracking( false );
        widget->removeEventFilter( this );
        
    } else if ( 0 == qstrcmp( widget->name(), kdeToolbarWidget ) ) {
        widget->removeEventFilter( this );
        widget->setBackgroundMode( PaletteBackground );
    }
    
    KStyle::unPolish( widget );
}

bool KlearlookStyle::eventFilter( QObject *object, QEvent *event ) {
    if ( object->parent() && 0 == qstrcmp( object->name(), kdeToolbarWidget ) ) {
        // Draw background for custom widgets in the toolbar that have specified a "kde toolbar widget" name.
        if ( QEvent::Paint == event->type() ) {
            QWidget * widget = static_cast<QWidget*>( object ),
                    *parent = static_cast<QWidget*>( object->parent() );
#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS
            // Find the top-level toolbar of this widget, since it may be nested in other
            // widgets that are on the toolbar.
            int x_offset = widget->x(),
                           y_offset = widget->y();

            while ( parent && parent->parent() && !qstrcmp( parent->name(), kdeToolbarWidget ) ) {
                x_offset += parent->x();
                y_offset += parent->y();
                parent = static_cast<QWidget*>( parent->parent() );
            }

            QRect pr( parent->rect() );
            bool horiz_grad = pr.width() < pr.height();

            // Check if the parent is a QToolbar, and use its orientation, else guess.
            QToolBar *toolbar = dynamic_cast<QToolBar*>( parent );

            if ( toolbar )
                horiz_grad = toolbar->orientation() == Qt::Vertical;

            drawBevelGradient( parent->colorGroup().background(), true, 1, &QPainter( widget ),
                               QRect( x_offset, y_offset, pr.width(), pr.height() ),
                               horiz_grad, SHADE_BAR_LIGHT, SHADE_BAR_DARK );
#else

            QPainter( widget ).fillRect( widget->rect(), parent->colorGroup().background() );
#endif

            return false;   // Now draw the contents
        }
    } else if ( object->inherits( "KToolBarSeparator" ) && QEvent::Paint == event->type() ) {
        QFrame * frame = dynamic_cast<QFrame*>( object );

        if ( frame && QFrame::NoFrame != frame->frameShape() ) {
            QPainter painter( frame );
            if ( QFrame::VLine == frame->frameShape() )
                drawPrimitive( PE_DockWindowSeparator, &painter,
                    frame->rect(), frame->colorGroup(), Style_Horizontal );
            else if ( QFrame::HLine == frame->frameShape() )
                drawPrimitive( PE_DockWindowSeparator, &painter,
                    frame->rect(), frame->colorGroup() );
            else
                return false;
            return true; // been drawn!
        }
    }
    switch ( event->type() ) {
        case QEvent::Enter:
            if ( object->isWidgetType() ) {
                hoverWidget = ( QWidget * ) object;
                if ( hoverWidget && hoverWidget->isEnabled() ) {
                    if ( redrawHoverWidget() ) {
                        hoverWidget->repaint( false );
                        if ( APP_KICKER == themedApp )
                            hover = HOVER_NONE;
                    }
                    oldCursor = QCursor::pos();
                } else
                    hoverWidget = NULL;
            }
            break;
        case QEvent::Leave:
            if ( hoverWidget && object == hoverWidget ) {
                oldCursor.setX( -1 );
                oldCursor.setY( -1 );
                hoverWidget = NULL;
                ( ( QWidget * ) object ) ->repaint( false );
            }
            break;
        case QEvent::MouseMove:
            if ( hoverWidget && object->isWidgetType() ) {
                if ( redrawHoverWidget() ) {
                    hoverWidget->repaint( false );
                    if ( APP_KICKER == themedApp )
                        hover = HOVER_NONE;
                }
                oldCursor = QCursor::pos();
            }
            break;
        default:
            break;
    }

    return KStyle::eventFilter( object, event );
}

void KlearlookStyle::drawLightBevelButton(
    QPainter *p,
    const QRect &r,
    const QColorGroup &cg,
    QStyle::SFlags flags,
    bool useGrad,
    ERound round,
    const QColor &fill,
    const QColor *custom,
    bool light ) const
{
    QRect br( r );
    bool sunken = ( flags & ( QStyle::Style_Down | QStyle::Style_On | QStyle::Style_Sunken ) );
    int dark = borderButton ? 4 : 5,
        c1   = sunken       ? dark : light ? 6 : 0;

    p->save();

    if ( !borderButton )
        br.addCoords( -1, -1, 1, 1 );

    if ( ( sunken && !borderButton ) || ( !sunken && flags & QStyle::Style_Raised ) ) {
        p->setPen( custom ? custom[ c1 ] : gray[ c1 ] );
        if ( APPEARANCE_LIGHT_GRADIENT != appearance ) {
            int c2 = sunken ? 0 : dark;


            p->drawLine( br.x() + 1, br.y() + 2, br.x() + 1, br.y() + br.height() - 3 ); // left
            p->drawLine( br.x() + 1, br.y() + 1, br.x() + br.width() - 2, br.y() + 1 ); // top

            p->setPen( custom ? custom[ c2 ] : gray[ c2 ] );
            p->drawLine( br.x() + br.width() - 2, br.y() + 1,
                br.x() + br.width() - 2, br.y() + br.height() - 3 ); // right
            p->drawLine( br.x() + 1, br.y() + br.height() - 2,
                br.x() + br.width() - 2, br.y() + br.height() - 2 ); // bottom

            br.addCoords( 2, 2, -2, -2 );
        } else {
            p->drawLine( br.x() + 1, br.y() + 2, br.x() + 1, br.y() + br.height() - 2 ); // left
            p->drawLine( br.x() + 1, br.y() + 1, br.x() + br.width() - 2, br.y() + 1 ); // top

            br.addCoords( 2, 2, -1, -1 );
        }
    } else
        br.addCoords( 1, 1, -1, -1 );

    // fill
    if ( useGrad && APPEARANCE_FLAT != appearance ) {
        drawBevelGradient( fill.dark( 100 ), !sunken, 0, p,
            QRect( br.left() - 1, br.top() - 1, br.width() + 2, br.height() + 2 ), flags & Style_Horizontal,
                sunken ?
                    SHADE_BEVEL_BUTTON_GRAD_LIGHT( appearance ) :
                        SHADE_BEVEL_BUTTON_GRAD_LIGHT( appearance ),
                   sunken ?
                        SHADE_BEVEL_BUTTON_GRAD_DARK( appearance ) :
                    SHADE_BEVEL_BUTTON_GRAD_DARK( appearance ) );
    } else
        p->fillRect( br, fill );

    if ( borderButton )
        if ( rounded && ROUNDED_NONE != round ) {
            bool wide = r.width() >= QTC_MIN_BTN_SIZE,
                        tall = r.height() >= QTC_MIN_BTN_SIZE;
            QColor border( flags & Style_ButtonDefault && IND_FONT_COLOUR == defBtnIndicator & flags & Style_Enabled
                           ? cg.text() : custom ? custom[ 5 ] : gray[ 5 ] );

            p->setPen( border.light(80) );
            switch ( round ) {
                case ROUNDED_ALL:
                    p->drawLine( r.x() + 2, r.y(), r.x() + r.width() - 3, r.y() );
                    p->drawLine( r.x() + 2, r.y() + r.height() - 1, r.x() + r.width() - 3,
                        r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 3 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 2, r.x() + r.width() - 1,
                        r.y() + r.height() - 3 );
                    if ( tall && wide ) {
                        p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !( tall && wide ) ) {
                        p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                        p->drawLine( r.x(), r.y() + r.height() - 2, r.x() + 1, r.y() + r.height() - 1 );
                        p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1,
                            r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( tall && wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                            p->drawPoint( r.x(), r.y() );
                            p->drawPoint( r.x() + r.width() - 1, r.y() );
                            p->drawPoint( r.x(), r.y() + r.height() - 1 );
                            p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;
                case ROUNDED_TOP:
                    p->drawLine( r.x() + 2, r.y(), r.x() + r.width() - 3, r.y() );
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1,
                        r.x() + r.width() - 2, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 1 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 2,
                        r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    if ( wide ) {
                        p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !wide ) {
                        p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                    }
                    if ( !formMode ) {
                        if ( wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() );
                        p->drawPoint( r.x() + r.width() - 1, r.y() );
                    }
                    break;
                case ROUNDED_BOTTOM:
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 2, r.y() );
                    p->drawLine( r.x() + 2, r.y() + r.height() - 1,
                        r.x() + r.width() - 3, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y(), r.x(), r.y() + r.height() - 3 );
                    p->drawLine( r.x() + r.width() - 1, r.y(),
                        r.x() + r.width() - 1, r.y() + r.height() - 3 );
                    if ( wide ) {
                        p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !wide ) {
                        p->drawLine( r.x(), r.y() + r.height() - 2, r.x() + 1, r.y() + r.height() - 1 );
                        p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1,
                            r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() + r.height() - 1 );
                        p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;
                case ROUNDED_LEFT:
                    p->drawLine( r.x() + 2, r.y(), r.x() + r.width() - 2, r.y() );
                    p->drawLine( r.x() + 2, r.y() + r.height() - 1,
                        r.x() + r.width() - 2, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 3 );
                    p->drawLine( r.x() + r.width() - 1, r.y(), r.x() + r.width() - 1,
                        r.y() + r.height() - 1 );
                    if ( tall ) {
                        p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !tall ) {
                        p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        p->drawLine( r.x(), r.y() + r.height() - 2,
                            r.x() + 1, r.y() + r.height() - 1 );
                    }
                    if ( !formMode ) {
                        if ( tall )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() );
                        p->drawPoint( r.x(), r.y() + r.height() - 1 );
                    }
                    break;
                case ROUNDED_RIGHT:
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 3, r.y() );
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1,
                        r.x() + r.width() - 3, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y(), r.x(), r.y() + r.height() - 1 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 2,
                        r.x() + r.width() - 1, r.y() + r.height() - 3 );
                    if ( tall ) {
                        p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !tall ) {
                        p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                        p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1,
                            r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( tall )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x() + r.width() - 1, r.y() );
                        p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;
                default:
                    break;
            }
        } else {
            p->setPen( flags & Style_ButtonDefault &&
                       IND_FONT_COLOUR == defBtnIndicator ?
                       cg.text() : custom ? custom[ 5 ] : gray[ 5 ] );
            p->setBrush( NoBrush );
            p->drawRect( r );
        }

    p->restore();
}
void KlearlookStyle::drawLightBevel(
    QPainter *p,
    const QRect &r,
    const QColorGroup &cg,
    QStyle::SFlags flags,
    bool useGrad,
    ERound round,
    const QColor &fill,
    const QColor *custom,
    bool light ) const
{
    QRect br( r );
    bool sunken = ( flags & ( QStyle::Style_Down | QStyle::Style_On | QStyle::Style_Sunken ) );
    int dark = borderButton ? 4 : 5, c1 = sunken ? dark : light ? 6 : 0;

    p->save();

    if ( !borderButton )
        br.addCoords( -1, -1, 1, 1 );

    if ( ( sunken && !borderButton ) || ( !sunken && flags & QStyle::Style_Raised ) ) {
        p->setPen( custom ? custom[ c1 ] : gray[ c1 ] );
        if ( APPEARANCE_LIGHT_GRADIENT != appearance ) {
            int c2 = sunken ? 0 : dark;

            p->drawLine( br.x() + 1, br.y() + 2, br.x() + 1, br.y() + br.height() - 3 ); // left
            p->drawLine( br.x() + 1, br.y() + 1, br.x() + br.width() - 2, br.y() + 1 ); // top

            p->setPen( custom ? custom[ c2 ] : gray[ c2 ] );
            p->drawLine( br.x() + br.width() - 2, br.y() + 1,
                br.x() + br.width() - 2, br.y() + br.height() - 3 ); // right
            p->drawLine( br.x() + 1, br.y() + br.height() - 2,
                br.x() + br.width() - 2, br.y() + br.height() - 2 ); // bottom

            br.addCoords( 2, 2, -2, -2 );
        } else {
            p->drawLine( br.x() + 1, br.y() + 2, br.x() + 1, br.y() + br.height() - 2 ); // left
            p->drawLine( br.x() + 1, br.y() + 1, br.x() + br.width() - 2, br.y() + 1 ); // top

            br.addCoords( 2, 2, -1, -1 );
        }
    } else
        br.addCoords( 1, 1, -1, -1 );

    // fill
    if ( useGrad && APPEARANCE_FLAT != appearance ) {
        drawBevelGradient( fill, !sunken, 0, p,
            QRect( br.left() - 1, br.top() - 1, br.width() + 2, br.height() + 2 ),
                flags & Style_Horizontal,
                sunken ?
                    SHADE_BEVEL_GRAD_SEL_LIGHT( appearance ) :
                    SHADE_BEVEL_GRAD_LIGHT( appearance ),
                sunken ?
                    SHADE_BEVEL_GRAD_SEL_DARK( appearance ) :
                    SHADE_BEVEL_GRAD_DARK( appearance ) );
    } else {
        p->fillRect( br, fill );
    }

    if ( borderButton )
        if ( rounded && ROUNDED_NONE != round ) {
            bool wide = r.width() >= QTC_MIN_BTN_SIZE,
                        tall = r.height() >= QTC_MIN_BTN_SIZE;

            QColor border = menuPbar[ GRADIENT_BASE ].dark( 130 );
            
            p->setPen( border );

            switch ( round ) {
                case ROUNDED_ALL:
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 2, r.y() ); // top
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1, r.x() + r.width() - 2, r.y() + r.height() - 1 ); // bottom
                    p->drawLine( r.x(),r.y() + 1, r.x(),r.y() + r.height() - 2 ); // left
                    p->drawLine( r.x() + r.width() - 1, r.y() + 1, r.x() + r.width() - 1, r.y() + r.height() - 2 ); // right

                    //p->drawLine( r.x() + 2, r.y() + r.height() - 1, r.x() + r.width() - 3, r.y() + r.height() - 1 );

                    //p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 3 );
                    //p->drawLine( r.x() + r.width() - 1, r.y() + 2, r.x() + r.width() - 1, r.y() + r.height() - 3 );

                    if ( tall && wide ) {
                        //p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        //p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        //p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        //p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !( tall && wide ) ) {
                        //p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        //p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                        //p->drawLine( r.x(), r.y() + r.height() - 2, r.x() + 1, r.y() + r.height() - 1 );
                        //p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1, r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( tall && wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );

                        //p->drawPoint( r.x(), r.y() );
                        //p->drawPoint( r.x() + r.width() - 1, r.y() );
                        //p->drawPoint( r.x(), r.y() + r.height() - 1 );
                        //p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;

                case ROUNDED_TOP:
                    p->drawLine( r.x() + 2, r.y(), r.x() + r.width() - 3, r.y() );
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1, r.x() + r.width() - 2, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 1 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 2, r.x() + r.width() - 1, r.y() + r.height() - 1 );

                    if ( wide ) {
                        p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !wide ) {
                        p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                    }
                    if ( !formMode ) {
                        if ( wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() );
                        p->drawPoint( r.x() + r.width() - 1, r.y() );
                    }
                    break;
                case ROUNDED_BOTTOM:
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 2, r.y() );
                    p->drawLine( r.x() + 2, r.y() + r.height() - 1, r.x() + r.width() - 3, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y(), r.x(), r.y() + r.height() - 3 );
                    p->drawLine( r.x() + r.width() - 1, r.y(), r.x() + r.width() - 1, r.y() + r.height() - 3 );
                    if ( wide ) {
                        p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !wide ) {
                        p->drawLine( r.x(), r.y() + r.height() - 2, r.x() + 1, r.y() + r.height() - 1 );
                        p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1,
                            r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( wide )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() + r.height() - 1 );
                        p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;
                case ROUNDED_LEFT:
                    p->drawLine( r.x() + 2, r.y(), r.x() + r.width() - 2, r.y() );
                    p->drawLine( r.x() + 2, r.y() + r.height() - 1,
                        r.x() + r.width() - 2, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 2, r.x(), r.y() + r.height() - 3 );
                    p->drawLine( r.x() + r.width() - 1, r.y(), r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    if ( tall ) {
                        p->drawPoint( r.x() + 1, r.y() + r.height() - 2 );
                        p->drawPoint( r.x() + 1, r.y() + 1 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !tall ) {
                        p->drawLine( r.x(), r.y() + 1, r.x() + 1, r.y() );
                        p->drawLine( r.x(), r.y() + r.height() - 2, r.x() + 1, r.y() + r.height() - 1 );
                    }
                    if ( !formMode ) {
                        if ( tall )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x(), r.y() );
                        p->drawPoint( r.x(), r.y() + r.height() - 1 );
                    }
                    break;
                case ROUNDED_RIGHT:
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 3, r.y() );
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1,
                        r.x() + r.width() - 3, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y(), r.x(), r.y() + r.height() - 1 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 2, r.x() + r.width() - 1,
                        r.y() + r.height() - 3 );
                    if ( tall ) {
                        p->drawPoint( r.x() + r.width() - 2, r.y() + 1 );
                        p->drawPoint( r.x() + r.width() - 2, r.y() + r.height() - 2 );
                        p->setPen( midColor( border, cg.background() ) );
                    }
                    if ( !formMode || !tall ) {
                        p->drawLine( r.x() + r.width() - 2, r.y(), r.x() + r.width() - 1, r.y() + 1 );
                        p->drawLine( r.x() + r.width() - 2, r.y() + r.height() - 1,
                            r.x() + r.width() - 1, r.y() + r.height() - 2 );
                    }
                    if ( !formMode ) {
                        if ( tall )
                            p->setPen( cg.background() );
                        else
                            p->setPen( midColor( custom ? custom[ 3 ] : gray[ 3 ], cg.background() ) );
                        p->drawPoint( r.x() + r.width() - 1, r.y() );
                        p->drawPoint( r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    }
                    break;
                default:
                    break;
            }
        } else {
            p->setPen( flags & Style_ButtonDefault &&
                IND_FONT_COLOUR == defBtnIndicator ? cg.text() : custom ? custom[ 5 ] : gray[ 5 ] );
            p->setBrush( NoBrush );
            p->drawRect( r );
        }

    p->restore();
}

void KlearlookStyle::drawArrow( QPainter *p, const QRect &r, const QColorGroup &cg, QStyle::SFlags flags,
                                QStyle::PrimitiveElement pe, bool small, bool checkActive ) const {
    QPointArray a;
    const QColor &col = flags & Style_Enabled
                        ? checkActive && flags & Style_Active
                        ? cg.highlightedText()
                        : cg.text()
                                : cg.mid();

    if ( vArrow )
        if ( small )
            switch ( pe ) {
                case QStyle::PE_ArrowUp:
                    a.setPoints( 7, 2, 1, 2, 0, 0, -2, -2, 0, -2, 1, -2, 0, 2, 0 );
                    break;
                case QStyle::PE_ArrowDown:
                    a.setPoints( 7, 2, -1, 2, 0, 0, 2, -2, 0, -2, -1, -2, 0, 2, 0 );
                    break;
                case QStyle::PE_ArrowRight:
                    a.setPoints( 7, 1, -2, 0, -2, -2, 0, 0, 2, 1, 2, 0, 2, 0, -2 );
                    break;
                case QStyle::PE_ArrowLeft:
                    a.setPoints( 7, -1, -2, 0, -2, 2, 0, 0, 2, -1, 2, 0, 2, 0, -2 );
                    break;
                default:
                    return ;
            }
        else
            switch ( pe ) {
                case QStyle::PE_ArrowUp:
                    a.setPoints( 7, 3, 1, 0, -2, -3, 1, -2, 2, -1, 1, 1, 1, 2, 2 );
                    break;
                case QStyle::PE_ArrowDown:
                    a.setPoints( 7, 3, -1, 0, 2, -3, -1, -2, -2, -1, -1, 1, -1, 2, -2 );
                    break;
                case QStyle::PE_ArrowRight:
                    a.setPoints( 7, -1, -3, 2, 0, -1, 3, -2, 2, -1, 1, -1, -1, -2, -2 );
                    break;
                case QStyle::PE_ArrowLeft:
                    a.setPoints( 7, 1, -3, -2, 0, 1, 3, 2, 2, 1, 1, 1, -1, 2, -2 );
                    break;
                default:
                    return ;
            }
    else
        if ( small )
            switch ( pe ) {
                case QStyle::PE_ArrowUp:
                    a.setPoints( 4, 2, 0, 0, -2, -2, 0, 2, 0 );
                    break;
                case QStyle::PE_ArrowDown:
                    a.setPoints( 4, 2, 0, 0, 2, -2, 0, 2, 0 );
                    break;
                case QStyle::PE_ArrowRight:
                    a.setPoints( 4, 0, -2, -2, 0, 0, 2, 0, -2 );
                    break;
                case QStyle::PE_ArrowLeft:
                    a.setPoints( 4, 0, -2, 2, 0, 0, 2, 0, -2 );
                    break;
                default:
                    return ;
            }
        else
            switch ( pe ) {
                case QStyle::PE_ArrowUp:
                    a.setPoints( 4, 3, 1, 0, -2, -3, 1, 3, 1 );
                    break;
                case QStyle::PE_ArrowDown:
                    a.setPoints( 4, 3, -1, 0, 2, -3, -1, 3, -1 );
                    break;
                case QStyle::PE_ArrowRight:
                    a.setPoints( 4, -1, -3, 2, 0, -1, 3, -1, -3 );
                    break;
                case QStyle::PE_ArrowLeft:
                    a.setPoints( 4, 1, -3, -2, 0, 1, 3, 1, -3 );
                    break;
                default:
                    return ;
            }

    if ( a.isNull() )
        return ;

    p->save();
    a.translate( ( r.x() + ( r.width() >> 1 ) ), ( r.y() + ( r.height() >> 1 ) ) );
    p->setBrush( col );
    p->setPen( col );
    p->drawPolygon( a );
    p->restore();
}

void KlearlookStyle::drawPrimitiveMenu( PrimitiveElement pe, QPainter *p, const QRect &r, const QColorGroup &cg,
                                        SFlags flags, const QStyleOption &data ) const {
    switch ( pe ) {
        case PE_CheckMark:
            if ( flags & Style_On || !( flags & Style_Off ) )      // !(flags&Style_Off) is for tri-state
            {
                QPointArray check;
                int x = r.center().x() - 3,
                        y = r.center().y() - 3;

                check.setPoints( 6,
                                 x, y + 2,
                                 x + 2, y + 4,
                                 x + 6, y,
                                 x + 6, y + 2,
                                 x + 2, y + 6,
                                 x, y + 4 );

                if ( flags & Style_On ) {
                    if ( flags & Style_Active ) {
                        p->setBrush( cg.highlightedText() );
                        p->setPen( cg.highlightedText() );
                    } else {
                        p->setBrush( cg.text() );
                        p->setPen( cg.text() );
                    }
                } else {
                    p->setBrush( cg.text() );
                    p->setPen( cg.text() );
                }
                p->drawPolygon( check );
            }
            break;

        default:
            KStyle::drawPrimitive( pe, p, r, cg, flags, data );
    }
}

void KlearlookStyle::drawPrimitive( PrimitiveElement pe, QPainter *p, const QRect &r, const QColorGroup &cg,
                                    SFlags flags, const QStyleOption &data ) const {
    int x, y, w, h;

    r.rect(&x, &y, &w, &h);
                                    
    switch ( pe ) {
        case PE_HeaderSection: {
                const QColor * use = buttonColors( cg );


                if ( APP_KICKER == themedApp ) {
                    if ( flags & Style_Down )
                        flags = ( ( flags | Style_Down ) ^ Style_Down ) | Style_Sunken;
                    flags |= Style_Enabled;
#if KDE_VERSION >= 0x30200
#if KDE_VERSION >= 0x30400

                    if ( HOVER_KICKER == hover && hoverWidget )   //  && hoverWidget==p->device())
                        flags |= Style_MouseOver;
#endif

                    formMode = isTransKicker;
#endif

                    drawLightBevelButton( p, r, cg, flags | Style_Horizontal,
                        true, ROUNDED_ALL, getFill( flags, use ), use );
#if KDE_VERSION >= 0x30200

                    formMode = false;
#endif

                } else {
                    flags = ( ( flags | Style_Sunken ) ^ Style_Sunken ) | Style_Raised;

                    if ( QTC_NO_SECT != hoverSect && HOVER_HEADER == hover && hoverWidget ) {
                        QHeader * hd = dynamic_cast<QHeader *>( hoverWidget );

                        if ( hd && hd->isClickEnabled( hoverSect ) && r == hd->sectionRect( hoverSect ) )
                            flags |= Style_MouseOver;
                    }
                    drawLightBevelButton( p, r, cg, flags | Style_Horizontal,
                        true, ROUNDED_NONE, getFill( flags, use ), use );
                }
                break;
            }
        case PE_HeaderArrow:
            drawArrow( p, r, cg, flags, flags & Style_Up ? PE_ArrowUp : PE_ArrowDown );
            break;
        case PE_ButtonCommand:
        case PE_ButtonBevel:
        case PE_ButtonTool:
        case PE_ButtonDropDown: {
                const QColor *use = buttonColors( cg );

                if ( !( flags & QStyle::Style_Sunken ) )   // If its not sunken, its raised-don't want flat buttons.
                    flags |= QStyle::Style_Raised;

                drawLightBevelButton( p, r, cg, flags | Style_Horizontal, true,
                                      r.width() < 16 || r.height() < 16
#if KDE_VERSION >= 0x30200
                                      || ( APP_KORN == themedApp && isTransKicker && PE_ButtonTool == pe )
#endif
                                      ? ROUNDED_NONE : ROUNDED_ALL,
                                      getFill( flags, use ), use );
                break;
            }
        case PE_ButtonDefault:
            switch ( defBtnIndicator ) {
                case IND_BORDER:
                    p->setBrush( NoBrush );
                    if ( rounded )   // borderButton)      CPD Only use color[4] for rounded def buttons!
                    {
                        const QColor * use = buttonColors( cg );

                        p->setPen( use[ 4 ] );
                        int offset = r.width() >= QTC_MIN_BTN_SIZE && r.height() >= QTC_MIN_BTN_SIZE ? 4 : 3;

                        p->drawLine( r.x() + offset, r.y(), r.x() + r.width() - ( 1 + offset ), r.y() );
                        p->drawLine( r.x() + offset, r.y() + r.height() - 1,
                            r.x() + r.width() - ( 1 + offset ), r.y() + r.height() - 1 );
                        p->drawLine( r.x(), r.y() + offset, r.x(), r.y() + r.height() - ( 1 + offset ) );
                        p->drawLine( r.x() + r.width() - 1, r.y() + offset,
                            r.x() + r.width() - 1, r.y() + r.height() - ( 1 + offset ) );
                    } else {
                        p->setPen( cg.text() );
                        p->drawRect( r );
                    }
                    break;
                case IND_CORNER: {
                        const QColor *use = buttonColors( cg );
                        QPointArray points;
                        bool sunken = flags & Style_Down || flags & QStyle::Style_Sunken;
                        int offset = sunken ? 4 : 3;

                        points.setPoints( 3, r.x() + offset, r.y() + offset, r.x() + offset + 6, r.y() + offset,
                                          r.x() + offset, r.y() + offset + 6 );

                        p->setBrush( use[ sunken ? 0 : borderButton ? 4 : 5 ] );
                        p->setPen( use[ sunken ? 0 : borderButton ? 4 : 5 ] );
                        p->drawPolygon( points );
                        break;
                    }
                default:
                    break;
            }
            break;
        case PE_IndicatorMask:
            if ( rounded ) {
                p->fillRect( r, color0 );
                p->fillRect( r.x() + 1, r.y() + 1, r.width() - 2, r.height() - 2, color1 );
                p->setPen( color1 );
                p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 2, r.y() );
                p->drawLine( r.x() + 1, r.y() + r.height() - 1, r.x() + r.width() - 2, r.y() + r.height() - 1 );
                p->drawLine( r.x(), r.y() + 1, r.x(), r.y() + r.height() - 2 );
                p->drawLine( r.x() + r.width() - 1, r.y() + 1, r.x() + r.width() - 1, r.y() + r.height() - 2 );
            } else
                p->fillRect( r, color1 );
            break;
        case PE_CheckMark:
            if ( flags & Style_On || !( flags & Style_Off ) )      // !(flags&Style_Off) is for tri-state
            {
                QPointArray check;
                int x = r.center().x() - 3,
                        y = r.center().y() - 3;

                check.setPoints( 6,
                                 x, y + 2,
                                 x + 2, y + 4,
                                 x + 6, y,
                                 x + 6, y + 2,
                                 x + 2, y + 6,
                                 x, y + 4 );
                p->setBrush( flags & Style_On
                             ? flags & Style_Enabled
                             ? flags & Style_Selected
                             ? cg.highlightedText()
                             : cg.text()
                                     : cg.mid()
                                     : cg.light() );
                p->setPen( flags & Style_Enabled
                           ? flags & Style_Selected
                           ? cg.highlightedText()
                           : cg.text()
                                   : cg.mid() );
                p->drawPolygon( check );
            }
            break;
        case PE_CheckListController: {
                QCheckListItem *item = data.checkListItem();

                if ( item ) {
                    QListView * lv = item->listView();
                    int x = r.x(), y = r.y(), w = r.width(), h = r.height(), marg = lv->itemMargin();

                    p->setPen( QPen( flags & Style_Enabled ? cg.text()
                                     : lv->palette().color( QPalette::Disabled, QColorGroup::Text ) ) );

                    if ( flags & Style_Selected && !lv->rootIsDecorated() &&
                         !( ( item->parent() && 1 == item->parent() ->rtti() &&
                         QCheckListItem::Controller == ( ( QCheckListItem* ) item->parent() ) ->type() ) )) {
                            p->fillRect( 0, 0, x + marg + w + 4, item->height(),
                                cg.brush( QColorGroup::Highlight ) );
                            if ( item->isEnabled() )
                                p->setPen( QPen( cg.highlightedText() ) );
                    }

                    if ( flags & Style_NoChange )
                        p->setBrush( cg.brush( QColorGroup::Button ) );
                    p->drawRect( x + marg + 2, y + 4 + 2, w - 7, h - 8 );
                    p->drawRect( x + marg, y + 4, w - 7, h - 8 );
                }
                break;
            }
        case PE_CheckListIndicator: {
                QCheckListItem *item = data.checkListItem();

                if ( item ) {
                    QListView * lv = item->listView();

                    p->setPen( QPen( flags & Style_Enabled ? cg.text()
                                     : lv->palette().color( QPalette::Disabled, QColorGroup::Text ), 2 ) );
                    if ( flags & Style_Selected ) {
                        flags -= Style_Selected;
                        if ( !lv->rootIsDecorated() &&
                             !( ( item->parent() && 1 == item->parent() ->rtti() &&
                             QCheckListItem::Controller ==
                             ( ( QCheckListItem* ) item->parent() ) ->type() ) ) ) {
                            p->fillRect( 0, 0, r.x() + lv->itemMargin() + r.width() + 4, item->height(),
                                         cg.brush( QColorGroup::Highlight ) );
                            if ( item->isEnabled() ) {
                                p->setPen( QPen( cg.highlightedText(), 2 ) );
                                flags += Style_Selected;
                            }
                        }
                    }

                    if ( flags & Style_NoChange )
                        p->setBrush( cg.brush( QColorGroup::Button ) );
                    p->drawRect( r.x() + lv->itemMargin(), r.y() + 2, r.width() - 4, r.width() - 4 );
                    if ( flags & QStyle::Style_On || !( flags & Style_Off ) )
                        drawPrimitive( PE_CheckMark, p, QRect( r.x() + lv->itemMargin(),
                            r.y() + 2, r.width() - 4, r.width() - 4 ), cg, flags );
                }
                break;
            }
        case PE_Indicator: {
                const QColor *use = buttonColors( cg );
                bool on = flags & QStyle::Style_On || !( flags & Style_Off );

                if ( APPEARANCE_FLAT != appearance )
                    drawPrimitive( PE_ButtonTool, p, r, cg, flags );
                else {
                    p->fillRect( r.x() + 1, r.y() + 2, QTC_CHECK_SIZE - 2, QTC_CHECK_SIZE - 2,
                                 flags & Style_Enabled ? cg.base() : cg.background() );
                    p->setPen( use[ 4 ] );
                    p->drawLine( r.x() + 1, r.y() + QTC_CHECK_SIZE - 1, r.x() + 1, r.y() + 1 );
                    p->drawLine( r.x() + 1, r.y() + 1, r.x() + QTC_CHECK_SIZE - 2, r.y() + 1 );
                }
                p->setPen( use[ 5 ] );
                p->setBrush( NoBrush );
                if ( rounded ) {
                    p->drawLine( r.x() + 1, r.y(), r.x() + r.width() - 2, r.y() );
                    p->drawLine( r.x() + 1, r.y() + r.height() - 1, r.x() + r.width() - 2, r.y() + r.height() - 1 );
                    p->drawLine( r.x(), r.y() + 1, r.x(), r.y() + r.height() - 2 );
                    p->drawLine( r.x() + r.width() - 1, r.y() + 1, r.x() + r.width() - 1, r.y() + r.height() - 2 );

                    p->setPen( midColor( use[ 3 ], cg.background() ) );
                    p->drawPoint( r.x(), r.y() );
                    p->drawPoint( r.x(), r.y() + r.width() - 1 );
                    p->drawPoint( r.x() + r.height() - 1, r.y() );
                    p->drawPoint( r.x() + r.height() - 1, r.y() + r.width() - 1 );
                } else if ( APPEARANCE_FLAT == appearance || borderButton )
                    p->drawRect( r.x(), r.y(), QTC_CHECK_SIZE, QTC_CHECK_SIZE );

                if ( on )
                    drawPrimitive( PE_CheckMark, p, r, cg, flags );
                break;
            }
        case PE_CheckListExclusiveIndicator: {
                QCheckListItem *item = data.checkListItem();

                if ( item ) {
                    const QColor & bgnd = cg.background(),
                                          &on = flags & Style_Enabled
                                                ? cg.text()
                                                : cg.mid();
                    bool set
                        = flags & QStyle::Style_On;
                    QPointArray outer,
                    inner,
                    aa;
                    int x = r.x(), y = r.y() + 2;

                    outer.setPoints( 24, x, y + 8, x, y + 4, x + 1, y + 3, x + 1, y + 2,
                                     x + 2, y + 1, x + 3, y + 1, x + 4, y, x + 8, y,
                                     x + 9, y + 1, x + 10, y + 1, x + 11, y + 2, x + 11, y + 3,
                                     x + 12, y + 4, x + 12, y + 8, x + 11, y + 9, x + 11, y + 10,
                                     x + 10, y + 11, x + 9, y + 11, x + 8, y + 12, x + 4, y + 12,
                                     x + 3, y + 11, x + 2, y + 11, x + 1, y + 10, x + 1, y + 9 );
                    inner.setPoints( 20, x + 1, y + 8, x + 1, y + 4, x + 2, y + 3, x + 2, y + 2,
                                     x + 3, y + 2, x + 4, y + 1, x + 8, y + 1, x + 9, y + 2,
                                     x + 10, y + 2, x + 10, y + 3, x + 11, y + 4, x + 11, y + 8,
                                     x + 10, y + 9, x + 10, y + 10, x + 9, y + 10, x + 8, y + 11,
                                     x + 4, y + 11, x + 3, y + 10, x + 2, y + 10, x + 2, y + 9 );
                    aa.setPoints( 16, x + 2, y + 4, x + 4, y + 2, x + 8, y + 2, x + 10, y + 4,
                                  x + 10, y + 8, x + 8, y + 10, x + 4, y + 10, x + 2, y + 8,
                                  x, y + 3, x + 3, y, x + 9, y, x + 12, y + 3,
                                  x + 12, y + 9, x + 9, y + 12, x + 3, y + 12, x, y + 9 );
                    p->setBrush( on );
                    p->drawPolyline( outer );
                    p->drawPolyline( inner );
                    p->setPen( midColor( on, bgnd, 1.5 ) );
                    p->drawPoints( aa );

                    if ( set
                           ) {
                            p->setPen( midColor( on, bgnd ) );
                            p->drawLine( x + 5, y + 4, x + 7, y + 4 );
                            p->drawLine( x + 5, y + 8, x + 7, y + 8 );
                            p->drawLine( x + 4, y + 5, x + 4, y + 7 );
                            p->drawLine( x + 8, y + 5, x + 8, y + 7 );
                            p->setBrush( on );
                            p->setPen( NoPen );
                            p->drawRect( x + 5, y + 5, 3, 3 );
                        }
                }
                break;
            }
        case PE_ExclusiveIndicator:
        case PE_ExclusiveIndicatorMask: {
                int x = r.x(), y = r.y();
                QPointArray outer;
                outer.setPoints( 24, x, y + 8, x, y + 4, x + 1, y + 3, x + 1, y + 2,
                                 x + 2, y + 1, x + 3, y + 1, x + 4, y, x + 8, y,
                                 x + 9, y + 1, x + 10, y + 1, x + 11, y + 2, x + 11, y + 3,
                                 x + 12, y + 4, x + 12, y + 8, x + 11, y + 9, x + 11, y + 10,
                                 x + 10, y + 11, x + 9, y + 11, x + 8, y + 12, x + 4, y + 12,
                                 x + 3, y + 11, x + 2, y + 11, x + 1, y + 10, x + 1, y + 9 );

                if ( PE_ExclusiveIndicatorMask == pe ) {
                    p->fillRect( r, color0 );
                    p->setPen( Qt::color1 );
                    p->setBrush( Qt::color1 );
                    p->drawPolygon( outer );
                } else {
                    QPointArray shadow;
                    const QColor &bgnd = flags & Style_Enabled ? cg.base() : cg.background(),
                                         &on = flags & Style_Enabled
                                               ? flags & Style_Selected
                                               ? cg.highlightedText()
                                               : cg.text()
                                                       : cg.mid();
                    QColor indBgnd = bgnd;
                    const QColor *use = buttonColors( cg );
                    QColor leftShadowColor,
                    rightShadowColor,
                    outerLeftColor,
                    outerRightColor;
                    bool set
                        = flags & QStyle::Style_On;

                    if ( APPEARANCE_FLAT != appearance && !borderButton )
                        shadow.setPoints( 14, x + 1, y + 10, x + 1, y + 9, x, y + 8, x, y + 4,
                                          x + 1, y + 3, x + 1, y + 2, x + 2, y + 1, x + 3, y + 1,
                                          x + 4, y, x + 8, y, x + 9, y + 1, x + 10, y + 1,
                                          x + 11, y + 2, x + 11, y + 3 );
                    else
                        shadow.setPoints( 9, x + 2, y + 11, x + 2, y + 9, x + 1, y + 8, x + 1, y + 4,
                                          x + 2, y + 3, x + 2, y + 2, x + 3, y + 2, x + 4, y + 1,
                                          x + 8, y + 1 );

                    p->fillRect( r, crLabelHighlight && flags & Style_MouseOver
                                 ? cg.background().light( QTC_HIGHLIGHT_FACTOR ) : cg.background() );

                    if ( APPEARANCE_FLAT != appearance ) {
                        indBgnd = getFill( flags, use );
                        p->setClipRegion( QRegion( outer ) );
                        drawBevelGradient( indBgnd, !set, 0, p,
                            QRect( x + 1, y + 1, r.width() - 2, r.height() - 2 ), true,
                            set ? SHADE_BEVEL_GRAD_SEL_LIGHT( appearance ) : SHADE_BEVEL_GRAD_LIGHT( appearance ),
                            set ? SHADE_BEVEL_GRAD_SEL_DARK( appearance ) : SHADE_BEVEL_GRAD_DARK( appearance ) );

                        p->setClipping( false );

                        if ( ( !set
                                && !( flags & Style_Down ) ) || !borderButton ) {
                            leftShadowColor = set
                                              ? !borderButton ? use[ 5 ] : use[ 4 ] : use[ 0 ];
                            p->setPen( leftShadowColor );
                            p->drawPolyline( shadow );

                            if ( APPEARANCE_LIGHT_GRADIENT == appearance )
                                rightShadowColor = indBgnd;
                            else {
                                if ( !borderButton )
                                    shadow.setPoints( 10, x + 12, y + 4, x + 12, y + 8, x + 11, y + 9,
                                                      x + 11, y + 10, x + 10, y + 11, x + 9, y + 11,
                                                      x + 8, y + 12, x + 4, y + 12, x + 3, y + 11,
                                                      x + 2, y + 11 );
                                else
                                    shadow.setPoints( 9, x + 10, y + 2, x + 10, y + 3, x + 11, y + 4,
                                                      x + 11, y + 8, x + 10, y + 9, x + 10, y + 10,
                                                      x + 9, y + 10, x + 8, y + 11, x + 4, y + 11 );
                                rightShadowColor = set
                                                       ? use[ 0 ] : !borderButton ? use[ 5 ] : use[ 4 ];
                                p->setPen( rightShadowColor );
                                p->drawPolyline( shadow );
                            }
                        }
                        else
                            leftShadowColor = rightShadowColor = indBgnd;
                    } else {
                        rightShadowColor = bgnd;
                        p->setBrush( bgnd );
                        p->setPen( bgnd );
                        p->drawEllipse( x, y, QTC_RADIO_SIZE, QTC_RADIO_SIZE );
                        p->setPen( use[ 4 ] );
                        leftShadowColor = use[ 4 ];
                        p->drawPolyline( shadow );
                    }

                    if ( APPEARANCE_FLAT == appearance || borderButton ) {
                        p->setPen( use[ 5 ] );
                        p->drawPolyline( outer );
                        shade( use[ 5 ], &outerRightColor, 1.1 );
                    } else {
                        shade( leftShadowColor, &outerLeftColor, 1.1 );
                        shade( rightShadowColor, &outerRightColor, 1.1 );
                    }
                    if ( set
                           ) {
                            p->setPen( midColor( on, indBgnd ) );
                            p->drawLine( x + 5, y + 4, x + 7, y + 4 );
                            p->drawLine( x + 5, y + 8, x + 7, y + 8 );
                            p->drawLine( x + 4, y + 5, x + 4, y + 7 );
                            p->drawLine( x + 8, y + 5, x + 8, y + 7 );
                            p->setBrush( on );
                            p->setPen( NoPen );
                            p->drawRect( x + 5, y + 5, 3, 3 );
                        }

                    if ( !formMode ) {
                        QPointArray outerAaLeft,
                        outerAaRight;

                        outerAaLeft.setPoints( 8, x, y + 3, x + 1, y + 1, x + 3, y,
                                               x + 9, y, x + 11, y + 1, x + 12, y + 3,
                                               x + 1, y + 11, x, y + 9 );
                        outerAaRight.setPoints( 4, x + 12, y + 9, x + 11, y + 11, x + 9, y + 12,
                                                x + 3, y + 12 );

                        p->setPen( midColor( outerRightColor, cg.background() ) );
                        p->drawPoints( outerAaRight );
                        if ( APPEARANCE_FLAT != appearance && !borderButton )
                            p->setPen( midColor( outerLeftColor, cg.background() ) );
                        p->drawPoints( outerAaLeft );
                        if ( APPEARANCE_LIGHT_GRADIENT == appearance )
                            p->setPen( midColor( indBgnd, use[ 5 ], 1.75 ) );
                        else
                            p->setPen( midColor( use[ 5 ], indBgnd, 1.5 ) );

                        if ( APPEARANCE_FLAT != appearance ) {
                            QPointArray innerAa;

                            if ( !set
                                    && !( flags & Style_Down ) ) {
                                if ( borderButton ) {
                                    innerAa.setPoints( 3, x + 1, y + 4, x + 2, y + 2, x + 4, y + 1 );
                                    p->drawPoints( innerAa );
                                    p->setPen( midColor( outerRightColor, cg.background() ) );
                                    p->drawPoint( x + 2, y + 10 );
                                } else {
                                    innerAa.setPoints( 4, x + 4, y + 11, x + 8, y + 11, x + 10, y + 10,
                                                       x + 11, y + 8 );
                                    p->drawPoints( innerAa );
                                }
                            }
                        } else {
                            QPointArray innerAa;

                            innerAa.setPoints( 6, x + 4, y + 11, x + 8, y + 11, x + 10, y + 10,
                                               x + 11, y + 8, x + 11, y + 4, x + 10, y + 2 );
                            p->drawPoints( innerAa );
                        }
                    }
                }
                break;
            }
        case PE_DockWindowSeparator: {
                QPoint p1,
                p2;
                //const QColor *use=backgroundColors(cg);

                if ( flags & Style_Horizontal ) {
                    int offset = r.height() > 18 ? 6 : r.height() > 12 ? 4 : r.height() > 6 ? 2 : 0;

                    p1 = QPoint( r.width() >> 1, 0 + offset );
                    p2 = QPoint( p1.x(), r.height() - offset );
                } else {
                    int offset = r.width() > 18 ? 6 : r.width() > 12 ? 4 : r.width() > 6 ? 2 : 0;

                    p1 = QPoint( 0 + offset, r.height() >> 1 );
                    p2 = QPoint( r.width() - offset, p1.y() );
                }
                p->fillRect( r, cg.background() );
                p->setPen( cg.background().dark( 111 ) );
                p->drawLine( p1, p2 );

                break;
            }
        case PE_Splitter: {
                const QColor *use = buttonColors( cg );

                if ( hoverWidget && hoverWidget == p->device() )
                    flags |= Style_MouseOver;

                if ( borderSplitter )
                    drawLightBevelButton( p, r, cg, QStyle::Style_Raised, false,
                        ROUNDED_NONE, getFill( flags, use ), use );
                else {
                    p->fillRect( r,
                        QColor( flags & Style_MouseOver ?
                        cg.background().light( QTC_HIGHLIGHT_FACTOR ) :
                        cg.background() ) );
                    drawLines( p, r, flags & Style_Horizontal, 70, 1, use, 0, TRUE,
                        APPEARANCE_LIGHT_GRADIENT == appearance );
                }
                break;
            }
        case PE_DockWindowResizeHandle:
            p->fillRect( r, cg.background() );
            if ( flags & Style_Horizontal ) {
                p->setPen( cg.highlight().light() );
                p->drawLine( r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1 );
                p->setPen( cg.highlight() );
                p->drawLine( r.left() + 1, r.top() + 2, r.right() - 1, r.top() + 2 );
                p->setPen( cg.highlight().dark() );
                p->drawLine( r.left() + 1, r.top() + 3, r.right() - 1, r.top() + 3 );
            } else {
                p->setPen( cg.highlight().light() );
                p->drawLine( r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 1 );
                p->setPen( cg.highlight() );
                p->drawLine( r.left() + 2, r.top() + 1, r.left() + 2, r.bottom() - 1 );
                p->setPen( cg.highlight().dark() );
                p->drawLine( r.left() + 3, r.top() + 1, r.left() + 3, r.bottom() - 1 );
            }
            break;
            
        case PE_StatusBarSection: {
                p->setPen( cg.background().dark(120) );
                p->drawRect(x-2, y-2, r.width()+3, r.height()+3);
                break;
        }
            
	case PE_PanelLineEdit: {
                const QColor *use = backgroundColors( cg );
		p->setPen( use[ 4 ].light(80) );
		p->drawRect( r );
		break;
	}
            
        case PE_PanelPopup: {
                const QColor *use = backgroundColors( cg );

                if ( borderFrame && ( data.isDefault() || data.lineWidth() > 1 ) ) {
                    p->setPen( use[ 4 ].light(70) );
                    p->setBrush( NoBrush );
		    drawPopupRect (p, r, cg);
                    //p->drawRect( r );
#ifdef MENU_POPUP_SHADOW

                    qDrawShadePanel( p, r.x() + 1, r.y() + 1, r.width() - 2, r.height() - 2,
                                     QColorGroup( use[ 4 ], use[ NUM_SHADES ], use[ 0 ], use[ 4 ], use[ 2 ],
                                     cg.text(), use[ NUM_SHADES ] ),
                                     flags & Style_Sunken,
                                     data.isDefault() ? QTC_BORDERED_FRAME_WIDTH - 1 : data.lineWidth() - 1 );
#endif

                } else
                    qDrawShadePanel( p, r,
                        QColorGroup(
                            use[ 5 ], use[ NUM_SHADES ], use[ 0 ], use[ 5 ], use[ 2 ],
                            cg.text(), use[ NUM_SHADES ]
                        ),
                        flags & Style_Sunken, data.isDefault() ? QTC_DEF_FRAME_WIDTH : data.lineWidth() );
                break;
            }
        case PE_PanelTabWidget: {
                const QColor *use = backgroundColors( cg );

                if ( borderFrame && ( data.isDefault() || data.lineWidth() > 1 ) ) {
                    p->setPen( use[ 4 ] );
                    p->setBrush( NoBrush );
                    p->drawRect( r );
#ifdef MENU_POPUP_SHADOW

                    qDrawShadePanel( p, r.x() + 1, r.y() + 1, r.width() - 2, r.height() - 2,
                                     QColorGroup( use[ 4 ], use[ NUM_SHADES ], use[ 0 ], use[ 4 ], use[ 2 ],
                                     cg.text(), use[ NUM_SHADES ] ),
                                     flags & Style_Sunken,
                                     data.isDefault() ? QTC_BORDERED_FRAME_WIDTH - 1 : data.lineWidth() - 1 );
#endif

                } else
                    qDrawShadePanel( p, r,
                        QColorGroup(
                            use[ 5 ], use[ NUM_SHADES ], use[ 0 ], use[ 5 ], use[ 2 ],
                            cg.text(), use[ NUM_SHADES ]
                        ),
                        flags & Style_Sunken, data.isDefault() ? QTC_DEF_FRAME_WIDTH : data.lineWidth() );
                break;
            }
        case PE_PanelDockWindow:
        case PE_PanelMenuBar: {
                const QColor *use = backgroundColors( cg );
                switch ( toolbarBorders ) {
                    case TB_DARK:
                        qDrawShadePanel( p, 
                            r.x(), r.y(), r.width(), r.height(),
                            QColorGroup(
                                use[ 5 ].dark( 120 ), use[ NUM_SHADES ], use[ 0 ],
                                use[ 5 ].dark( 120 ), use[ 2 ],
                                cg.text(), use[ NUM_SHADES ] ),
                            flags & Style_Sunken, 1
                        );
                            
#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS

                        if ( APPEARANCE_FLAT != appearance )
                            drawBevelGradient( use[ NUM_SHADES ],
                                               true, 1, p, r, true,
                                               SHADE_BAR_LIGHT, SHADE_BAR_DARK
                            );
#endif

                        break;
                    case TB_LIGHT:
                        qDrawShadePanel( p,
                            r.x(), r.y(), r.width(), r.height(),
                            QColorGroup(
                                use[ 3 ], use[ NUM_SHADES ], use[ 0 ],
                                use[ 3 ], use[ 2 ],
                                cg.text(), use[ NUM_SHADES ]
                            ),
                            flags & Style_Sunken, 1
                        );
                        
#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS

                        if ( APPEARANCE_FLAT != appearance )
                            drawBevelGradient( use[ NUM_SHADES ],
                                               true, 1, p, r, true,
                                               SHADE_BAR_LIGHT, SHADE_BAR_DARK );
#endif

                        break;
                    case TB_NONE:
                        break;

                } /* switch */

                break;
            }
        case PE_ScrollBarAddLine:
        case PE_ScrollBarSubLine: {
                bool down = ( flags & ( QStyle::Style_Down | QStyle::Style_On | QStyle::Style_Sunken ) );
                const QColor *use = buttonColors( cg );

                pe = flags & Style_Horizontal
                     ? PE_ScrollBarAddLine == pe
                     ? PE_ArrowRight
                     : PE_ArrowLeft
                 : PE_ScrollBarAddLine == pe
                     ? PE_ArrowDown
                     : PE_ArrowUp;

                drawLightBevelButton( p, r, cg,
                    down ? flags : flags | ( ( ( flags & Style_Enabled ) ? Style_Raised : Style_Default ) ),
                                      true,
                                      PE_ArrowRight == pe ? ROUNDED_RIGHT :
                                      PE_ArrowLeft == pe ? ROUNDED_LEFT :
                                      PE_ArrowDown == pe ? ROUNDED_BOTTOM :
                                      PE_ArrowUp == pe ? ROUNDED_TOP : ROUNDED_NONE,
                                      getFill( flags, use ), use );
                drawPrimitive( pe, p, r, cg, flags );
                break;
            }
        case PE_ScrollBarSubPage:
        case PE_ScrollBarAddPage: {
                const QColor *use = backgroundColors( cg );

                if ( borderButton ) {
                    if ( flags & Style_Horizontal ) {
                        p->fillRect( r.x(), r.y() + 1, r.width(), r.height() - 2, use[ 2 ] );
                        p->setPen( use[ 5 ] );
                        p->drawLine( r.left(), r.top(), r.right(), r.top() );
                        p->drawLine( r.left(), r.bottom(), r.right(), r.bottom() );
                    } else {
                        p->fillRect( r.x() + 1, r.y(), r.width() - 2, r.height(), use[ 2 ] );
                        p->setPen( use[ 5 ] );
                        p->drawLine( r.left(), r.top(), r.left(), r.bottom() );
                        p->drawLine( r.right(), r.top(), r.right(), r.bottom() );
                    }
                } else
                    p->fillRect( r.x(), r.y(), r.width(), r.height(), use[ 2 ] );
                break;
            }
        case PE_ScrollBarSlider: {
                const QColor *use = buttonColors( cg );



                if ( flags & Style_Down )
                    flags -= Style_Down;
                flags |= flags & Style_Enabled ? Style_Raised : Style_Default;

                drawLightBevelButton( p, r, cg, flags, true, ROUNDED_NONE, getFill( flags, use ), use );

                if ( GROOVE_NONE != sliderThumbs &&
                   ( ( flags & Style_Horizontal && r.width() >= 20 ) || r.height() >= 20 ) )
                    drawLines( p, r, !( flags & Style_Horizontal ), 3, 4, use, 0, GROOVE_SUNKEN == sliderThumbs,
                               APPEARANCE_LIGHT_GRADIENT == appearance );
                break;
            }
        case PE_FocusRect: {
                p->drawWinFocusRect( r, cg.background() );
                break;
            }
        case PE_ArrowUp:
        case PE_ArrowDown:
        case PE_ArrowRight:
        case PE_ArrowLeft:
            drawArrow( p, r, cg, flags, pe );
            break;
        case PE_SpinWidgetUp:
        case PE_SpinWidgetDown: {
                QRect sr( r );
                const QColor *use = buttonColors( cg );

                drawLightBevelButton( p, sr, cg,
                    flags | Style_Horizontal, true, PE_SpinWidgetDown == pe ? ROUNDED_BOTTOM : ROUNDED_TOP,
                    getFill( flags, use ), use );

                if ( vArrow ) {
                    if ( PE_SpinWidgetDown == pe )
                        sr.setY( sr.y() - 1 );
                } else
                    sr.setY( sr.y() + ( PE_SpinWidgetDown == pe ? -2 : 1 ) );

                drawArrow( p, sr, cg, flags, pe == PE_SpinWidgetUp ? PE_ArrowUp : PE_ArrowDown, true );
                break;
            }
        default:
            KStyle::drawPrimitive( pe, p, r, cg, flags, data );
    }
}

void KlearlookStyle::drawKStylePrimitive( KStylePrimitive kpe, QPainter *p, const QWidget *widget, const QRect &r,
        const QColorGroup &cg, SFlags flags, const QStyleOption &opt ) const {
    switch ( kpe ) {
        case KPE_ToolBarHandle:
        case KPE_GeneralHandle:
            drawLines( p, r, !( flags & Style_Horizontal ), 2,
                APP_KICKER == themedApp ? 1 : KPE_ToolBarHandle == kpe ? 4 : 2, gray,
                APP_KICKER == themedApp ? 1 : KPE_ToolBarHandle == kpe ? -2 : 0, GROOVE_SUNKEN == handles,
                APPEARANCE_LIGHT_GRADIENT == appearance );
            break;
        case KPE_SliderGroove:
            drawSliderGroove( p, r, flags, widget );
            break;
        case KPE_SliderHandle:
            drawSliderHandle( p, r, cg, flags );
            break;
        case KPE_ListViewExpander: {
                int lvSize = QTC_LV_SIZE( lvExpander );
                QRect ar( r.x() + ( ( r.width() - ( lvSize + 4 ) ) >> 1 ),
                    r.y() + ( ( r.height() - ( lvSize + 4 ) ) >> 1 ), lvSize + 4, lvSize + 4 );

                p->setPen(  /*lvDark ? cg.text() : */cg.mid() );

                if ( LV_LINES_NONE != lvLines ) {
                    int lo = rounded ? 2 : 0;

                    p->drawLine( ar.x() + lo, ar.y(), ( ar.x() + ar.width() - 1 ) - lo, ar.y() );
                    p->drawLine( ar.x() + lo, ar.y() + ar.height() - 1,
                        ( ar.x() + ar.width() - 1 ) - lo, ar.y() + ar.height() - 1 );
                    p->drawLine( ar.x(), ar.y() + lo, ar.x(), ( ar.y() + ar.height() - 1 ) - lo );
                    p->drawLine( ar.x() + ar.width() - 1,
                        ar.y() + lo, ar.x() + ar.width() - 1, ( ar.y() + ar.height() - 1 ) - lo );

                    if ( rounded ) {
                        p->drawPoint( ar.x() + 1, ar.y() + 1 );
                        p->drawPoint( ar.x() + 1, ar.y() + ar.height() - 2 );
                        p->drawPoint( ar.x() + ar.width() - 2, ar.y() + 1 );
                        p->drawPoint( ar.x() + ar.width() - 2, ar.y() + ar.height() - 2 );
                        p->setPen( midColor(  /*lvDark ? cg.text() : */cg.mid(), cg.background() ) );
                        p->drawLine( ar.x(), ar.y() + 1, ar.x() + 1, ar.y() );
                        p->drawLine( ar.x() + ar.width() - 2, ar.y(), ar.x() + ar.width() - 1, ar.y() + 1 );
                        p->drawLine( ar.x(), ar.y() + ar.height() - 2, ar.x() + 1, ar.y() + ar.height() - 1 );
                        p->drawLine( ar.x() + ar.width() - 2, ar.y() + ar.height() - 1,
                            ar.x() + ar.width() - 1, ar.y() + ar.height() - 2 );
                    }
                }

                if ( LV_EXP_ARR == lvExpander )
                    drawArrow( p, ar, cg, flags | Style_Enabled, flags & Style_On // Collapsed = On
                               ? QApplication::reverseLayout()
                               ? PE_ArrowLeft
                               : PE_ArrowRight
                           : PE_ArrowDown );
                else {
                    int xo = ( ar.width() - lvSize ) >> 1,
                             yo = ( ar.height() - lvSize ) >> 1;
                    int mid = lvSize >> 1;

                    p->setPen( cg.text() );
                    p->drawLine( ar.x() + xo + ( mid - 2 ), ar.y() + yo + mid,
                        ar.x() + xo + lvSize - ( mid - 1 ), ar.y() + yo + mid );
                    if ( flags & Style_On )   // Collapsed = On
                        p->drawLine( ar.x() + xo + mid, ar.y() + yo + ( mid - 2 ),
                            ar.x() + xo + mid, ar.y() + yo + lvSize - ( mid - 1 ) );
                }
                break;
            }
        case KPE_ListViewBranch:
            switch ( lvLines ) {
                case LV_LINES_NONE:
                    break;
                case LV_LINES_DOTTED:
                    // Taken and modified (colour wise) from kstyle.cpp - which in turn comes from
                    // qwindowsstyl.cpp
                    {
                        static QBitmap *verticalLine = 0,
                                                       *horizontalLine = 0;
                        static QCleanupHandler<QBitmap> lvCleanupBitmap;

                        // Create the dotline pixmaps if not already created
                        if ( !verticalLine ) {
                            // make 128*1 and 1*128 bitmaps that can be used for drawing the right sort of lines.
                            verticalLine = new QBitmap( 1, 129, true );
                            horizontalLine = new QBitmap( 128, 1, true );
                            QPointArray a( 64 );
                            QPainter p2;

                            p2.begin( verticalLine );

                            int i;
                            for ( i = 0; i < 64; i++ )
                                a.setPoint( i, 0, i * 2 + 1 );

                            p2.setPen( color1 );
                            p2.drawPoints( a );
                            p2.end();
                            QApplication::flushX();
                            verticalLine->setMask( *verticalLine );

                            p2.begin( horizontalLine );

                            for ( i = 0; i < 64; i++ )
                                a.setPoint( i, i * 2 + 1, 0 );

                            p2.setPen( color1 );
                            p2.drawPoints( a );
                            p2.end();
                            QApplication::flushX();
                            horizontalLine->setMask( *horizontalLine );

                            lvCleanupBitmap.add( &verticalLine );
                            lvCleanupBitmap.add( &horizontalLine );
                        }

                        p->setPen( lvDark ? cg.text() : cg.mid() );  // Wow, my big modification...

                        if ( flags & Style_Horizontal ) {
                            int point = r.x(),
                                        other = r.y(),
                                                end = r.x() + r.width(),
                                                      thickness = r.height();

                            while ( point < end ) {
                                int i = 128;

                                if ( i + point > end )
                                    i = end - point;
                                p->drawPixmap( point, other, *horizontalLine, 0, 0, i, thickness );
                                point += i;
                            }
                        } else {
                            int point = r.y(),
                                other = r.x(),
                                end = r.y() + r.height(),
                                thickness = r.width(),
                                pixmapoffset = ( flags & Style_NoChange ) ? 0 : 1;    // ### Hackish

                            while ( point < end ) {
                                int i = 128;

                                if ( i + point > end )
                                    i = end - point;
                                p->drawPixmap( other, point, *verticalLine, 0, pixmapoffset, thickness, i );
                                point += i;
                            }
                        }
                        break;
                    }
                case LV_LINES_SOLID:
                    p->setPen( cg.mid() );
                    p->drawLine( r.x(), r.y(), r.x() + r.width() - 1, r.y() + r.height() - 1 );
                    break;
            }
            break;
        default:
            KStyle::drawKStylePrimitive( kpe, p, widget, r, cg, flags, opt );
    }
}

void KlearlookStyle::drawControl(
    ControlElement control,
    QPainter *p,
    const QWidget *widget,
    const QRect &r,
    const QColorGroup &cg, SFlags flags, const QStyleOption &data ) const
{
    if ( widget == hoverWidget )
        flags |= Style_MouseOver;

    switch ( control ) {
        case CE_TabBarTab: {
                const QTabBar * tb = ( const QTabBar * ) widget;
                bool cornerWidget = false,
                                    firstTab = 0 == tb->indexOf( data.tab() ->identifier() );

                if ( ::qt_cast<const QTabWidget *>( tb->parent() ) ) {
                    const QTabWidget * tw = ( const QTabWidget* ) tb->parent();

                    // is there a corner widget in the (top) left edge?
                    if ( tw->cornerWidget( Qt::TopLeft ) )
                        cornerWidget = true;
                }

                if ( borderFrame ) {
                    QRect tr( r ),
                    fr( r );
                    int offset = rounded ? 2 : 0;

                    switch ( tb->shape() ) {
                        case QTabBar::TriangularAbove:
                        case QTabBar::RoundedAbove:
                            if ( flags & Style_Selected ) {
                                tr.addCoords( 0, 0, 0, -1 );
                                fr.addCoords( 2, 2, -2, -2 );
                                p->setPen( gray[ 5 ] );
                                p->drawLine( tr.left(),
                                    firstTab && !cornerWidget ?
                                             tr.bottom() + 1 : tr.bottom(), tr.left(),
                                             tr.top() + offset );
                                p->drawLine( tr.left() + offset, tr.top(), tr.right() - offset, tr.top() );
                                p->drawLine( tr.right(), tr.top() + 1, tr.right(), tr.bottom() );

                                p->setPen( gray[ 0 ] );
                                p->drawLine( tr.left() + 1, tr.bottom() + 1, tr.left() + 1, tr.top() + 2 );
                                p->drawLine( tr.left() + 1, tr.top() + 1, tr.right() - 1, tr.top() + 1 );
                                p->drawLine( tr.right() - 1, tr.bottom() + 1, tr.right(), tr.bottom() + 1 );

                                if ( cornerWidget )
                                    p->drawPoint( tr.left(), tr.bottom() + 1 );

                                p->setPen( gray[ 4 ] );
                                p->drawLine( tr.right() - 1, tr.top() + 1, tr.right() - 1, tr.bottom() );

                                if ( rounded ) {
                                    p->setPen( gray[ 5 ] );
                                    p->drawPoint( tr.x() + 1, tr.y() + 1 );
                                    p->drawPoint( tr.x() + tr.width() - 2, tr.y() + 1 );
                                    p->setPen( gray[ 4 ] );
                                    p->drawLine( tr.x(), tr.y() + 1, tr.x() + 1, tr.y() );
                                    p->drawLine( tr.x() + tr.width() - 2,
                                        tr.y(), tr.x() + tr.width() - 1, tr.y() + 1 );
                                }
                            } else {
                                tr.addCoords( 0, 2, 0, -1 );
                                fr.addCoords( 2, 4, -2, -2 );

                                p->setPen( gray[ 5 ] );
                                p->drawLine( tr.left(),
                                    firstTab && !cornerWidget ? tr.bottom() + 1 : tr.bottom(), tr.left(),
                                             tr.top() + 1 );
                                p->drawLine( rounded ? tr.left() + 1 : tr.left(),
                                    tr.top(), rounded ? tr.right() - 1 : tr.right(), tr.top() );
                                p->drawLine( tr.right(), tr.top() + 1, tr.right(), tr.bottom() );
                                p->drawLine( tr.left(), tr.bottom(), tr.right(), tr.bottom() );
                                p->setPen( gray[ 0 ] );
                                p->drawLine( tr.left() + 1, tr.top() + 1, tr.right() - 1, tr.top() + 1 );

                                if ( cornerWidget )
                                    p->drawPoint( tr.left(), tr.bottom() + 1 );

                                if ( !firstTab )
                                    p->setPen( gray[ 2 ] );

                                p->drawLine( tr.left() + 1, tr.bottom() - 1, tr.left() + 1, tr.top() + 2 );
                                p->setPen( gray[ 0 ] );
                                p->drawLine( tr.left() + 1, tr.bottom() + 1, tr.right(), tr.bottom() + 1 );
                                p->setPen( gray[ 4 ] );
                                p->drawLine( tr.right() - 1, tr.top() + 1, tr.right() - 1, tr.bottom() - 1 );
                            }

                            if ( APPEARANCE_FLAT != appearance ) {
                                if ( flags & Style_Selected ) {
                                    drawBevelGradient( cg.background(), true, 0, p, fr, true,
                                        SHADE_TAB_SEL_LIGHT( appearance ),
                                        SHADE_TAB_SEL_DARK( appearance ) );

				                    p->setPen(menuPbar[ GRADIENT_BASE ]);
				                    p->drawLine( fr.left()-1, fr.top()-1, fr.right()+1, fr.top()-1);
				                    p->drawLine( fr.left()-1, fr.top(), fr.right()+1, fr.top());

				                    p->setPen(menuPbar[ GRADIENT_DARK ].dark(118));
				                    p->drawLine( fr.left(), fr.top()-2, fr.right(), fr.top()-2);
                                    p->drawPoint( tr.x() + 1, tr.y() + 1 );
                                    p->drawPoint( tr.x(), tr.y() + 2 );
                                    p->drawPoint( tr.x() + tr.width() - 2, tr.y() + 1 );
                                    p->drawPoint( tr.x() + tr.width() - 1, tr.y() + 2 );
                                
                                } else
                                    drawBevelGradient( gray[ 2 ], true, 0, p, fr, true,
                                        SHADE_TAB_LIGHT( appearance ), SHADE_TAB_DARK( appearance ) );

			    
                            } else
                                p->fillRect( fr, flags & Style_Selected ? cg.background() : gray[ 2 ] );

                            break;
                        case QTabBar::TriangularBelow:
                        case QTabBar::RoundedBelow:
                            if ( flags & Style_Selected ) {
                                fr.addCoords( 3, 2, -3, -3 );
                                p->setPen( gray[ 5 ] );

                                p->drawLine( tr.left(), tr.bottom() - offset, tr.left(),
                                             firstTab && !cornerWidget ? tr.top() : tr.top() + 1 );
                                p->drawLine( rounded ? tr.left() + 1 : tr.left(),
                                    tr.bottom(), tr.right() - offset, tr.bottom() );
                                p->drawLine( tr.right(), tr.top() + 1, tr.right(), tr.bottom() - 1 );
                                p->setPen( gray[ 0 ] );
                                p->drawLine( tr.left() + 1, tr.bottom() - 2, tr.left() + 1,
                                             firstTab && !cornerWidget ? tr.top() : tr.top() - 1 );
                                p->setPen( gray[ 4 ] );
                                p->drawLine( tr.left() + 1, tr.bottom() - 1, tr.right() - 1, tr.bottom() - 1 );
                                p->drawLine( tr.right() - 1, tr.bottom() - 2, tr.right() - 1, tr.top() - 1 );
                                p->drawPoint( tr.right(), tr.top() );
                                if ( cornerWidget )
                                    p->drawPoint( tr.left(), tr.top() );


                                if ( rounded ) {
                                    p->setPen( gray[ 5 ] );
                                    p->drawPoint( tr.x() + 1, tr.y() + tr.height() - 2 );
                                    p->drawPoint( tr.x() + tr.width() - 2, tr.y() + tr.height() - 2 );
                                    p->setPen( gray[ 4 ] );
                                    p->drawLine( tr.x(), tr.y() + tr.height() - 2,
                                        tr.x() + 1, tr.y() + tr.height() - 1 );
                                    p->drawLine( tr.x() + tr.width() - 2,
                                        tr.y() + tr.height() - 1, tr.x() + tr.width() - 1,
                                        tr.y() + tr.height() - 2 );
                                }
                            } else {
                                tr.addCoords( 0, 1, 0, -2 );
                                fr.addCoords( 1, 2, -2, -4 );

                                p->setPen( gray[ 5 ] );
                                p->drawLine( tr.left(), tr.bottom() - 1, tr.left(),
                                             firstTab && !cornerWidget ? tr.top() : tr.top() + 1 );
                                p->drawLine( rounded ? tr.left() + 1 : tr.left(), tr.bottom(),
                                             rounded ? tr.right() - 1 : tr.right(), tr.bottom() );
                                p->drawLine( tr.right(), tr.top() + 1, tr.right(), tr.bottom() - 1 );
                                p->drawLine( tr.right(), tr.top(), tr.left(), tr.top() );
                                p->setPen( gray[ 4 ] );
                                p->drawLine( tr.left(), tr.top() - 1, tr.right(), tr.top() - 1 );
                                p->drawLine( tr.left() + 1, tr.bottom() - 1, tr.right() - 1, tr.bottom() - 1 );
                                p->drawLine( tr.right() - 1, tr.bottom() - 2, tr.right() - 1, tr.top() + 1 );
                            }
                            if ( APPEARANCE_FLAT != appearance )
                                if ( flags & Style_Selected )
                                    drawBevelGradient( cg.background(), false, -1, p, fr, true,
                                        SHADE_BOTTOM_TAB_SEL_DARK( appearance ),
                                        SHADE_BOTTOM_TAB_SEL_LIGHT( appearance ) );
                                else
                                    drawBevelGradient( gray[ 2 ], false, 0, p, fr, true,
                                        SHADE_BOTTOM_TAB_DARK( appearance ),
                                        SHADE_BOTTOM_TAB_LIGHT( appearance ) );
                            else
                                p->fillRect( fr, flags & Style_Selected ? cg.background() : gray[ 2 ] );
                            break;
                        default:
                            KStyle::drawControl( control, p, widget, r, cg, flags, data );
                    }
                } else {
                    QRect br( r );

                    switch ( tb->shape() ) {
                        case QTabBar::TriangularAbove:
                        case QTabBar::RoundedAbove:
                            if ( flags & Style_Selected ) {
                                p->setPen( cg.background() );
                                p->drawLine( br.bottomLeft(), br.bottomRight() );
                                p->setPen( gray[ 0 ] );
                                p->drawPoint( br.bottomLeft() );
                                p->setPen( gray[ 5 ] );
                                p->drawPoint( br.bottomRight() );
                                br.addCoords( 0, 0, 0, -1 );
                            } else {
                                p->setPen( gray[ 0 ] );
                                p->drawLine( br.left(), br.bottom(), br.right(), br.bottom() );
                                br.addCoords( 0, 1, -1, -1 );
                            }

                            p->setPen( gray[ 0 == r.left() || flags & Style_Selected ? 0 : 5 ] );
                            p->drawLine( br.bottomLeft(), br.topLeft() );
                            p->setPen( gray[ 0 ] );
                            p->drawLine( br.left() + 1, br.top(), br.right() - 1, br.top() );
                            p->setPen( gray[ 5 ] );
                            p->drawLine( br.right(), br.top(), br.right(), br.bottom() );
                            br.addCoords( 1, 1, -1, 0 );
                            if ( APPEARANCE_FLAT != appearance )
                                if ( flags & Style_Selected )
                                    drawBevelGradient( cg.background(), true, 0, p, br,
                                        true, SHADE_TAB_SEL_LIGHT( appearance ),
                                        SHADE_TAB_SEL_DARK( appearance ) );
                                else
                                    drawBevelGradient( gray[ 2 ], true, 0, p, br, true,
                                        SHADE_TAB_LIGHT( appearance ), SHADE_TAB_DARK( appearance ) );
                            else
                                p->fillRect( br, flags & Style_Selected ? cg.background() : gray[ 2 ] );
                            break;
                        case QTabBar::TriangularBelow:
                        case QTabBar::RoundedBelow:
                            if ( flags & Style_Selected ) {
                                p->setPen( cg.background() );
                                p->drawLine( br.topLeft(), br.topRight() );
                                p->setPen( gray[ 0 ] );
                                p->drawPoint( br.topLeft() );
                                p->setPen( gray[ 5 ] );
                                p->drawPoint( br.topRight() );
                                br.addCoords( 0, 1, 0, 0 );
                            } else {
                                p->setPen( gray[ 5 ] );
                                p->drawLine( br.left(), br.top(),
                                             br.right(), br.top() );
                                br.addCoords( 0, 1, -1, -1 );
                            }

                            if ( 0 == r.left() || flags & Style_Selected ) {
                                p->setPen( gray[ 0 ] );
                                p->drawLine( br.bottomLeft(), br.topLeft() );
                            }
                            p->setPen( gray[ 5 ] );
                            p->drawLine( br.bottomLeft(), br.bottomRight() );
                            p->drawLine( br.right(), br.top(), br.right(), br.bottom() );
                            br.addCoords( 1, 0, -1, -1 );
                            if ( APPEARANCE_FLAT != appearance )
                                if ( flags & Style_Selected )
                                    drawBevelGradient(
                                        cg.background(), false, 0, p, br, true,
                                        SHADE_BOTTOM_TAB_SEL_DARK( appearance ),
                                        SHADE_BOTTOM_TAB_SEL_LIGHT( appearance ) );
                                else
                                    drawBevelGradient( gray[ 2 ], false, 0, p, br,
                                        true, SHADE_BOTTOM_TAB_DARK( appearance ),
                                        SHADE_BOTTOM_TAB_LIGHT( appearance ) );
                            else
                                p->fillRect( br, flags & Style_Selected ? cg.background() : gray[ 2 ] );
                    }
                }
                break;
            }
#if QT_VERSION >= 0x030200
        case CE_TabBarLabel: {
                if ( data.isDefault() )
                    break;

                const QTabBar *tb = ( const QTabBar * ) widget;
                QTab *t = data.tab();
                QRect tr = r;

                if ( t->identifier() == tb->currentTab() ) {
                    if ( QTabBar::RoundedAbove == tb->shape() || QTabBar::TriangularAbove == tb->shape() )
                        tr.setBottom( tr.bottom() - pixelMetric( QStyle::PM_TabBarTabShiftVertical, tb ) );
                } else
                    if ( QTabBar::RoundedBelow == tb->shape() || QTabBar::TriangularBelow == tb->shape() )
                        tr.setTop( tr.top() + pixelMetric( QStyle::PM_TabBarTabShiftVertical, tb ) );

                drawItem( p, tr, AlignCenter | ShowPrefix, cg, flags & Style_Enabled, 0, t->text() );

                if ( ( flags & Style_HasFocus ) && !t->text().isEmpty() )
                    drawPrimitive( PE_FocusRect, p, r, cg );
                break;
            }
#endif
        case CE_PushButtonLabel:    // Taken from Highcolour and Plastik...
            {
                int x, y, w, h;

                r.rect( &x, &y, &w, &h );

                const QPushButton *button = static_cast<const QPushButton *>( widget );
                bool active = button->isOn() || button->isDown(),
                              cornArrow = false;

                // Shift button contents if pushed.
                if ( active ) {
                    x += pixelMetric( PM_ButtonShiftHorizontal, widget );
                    y += pixelMetric( PM_ButtonShiftVertical, widget );
                    flags |= Style_Sunken;
                }

                // Does the button have a popup menu?
                if ( button->isMenuButton() ) {
                    int dx = pixelMetric( PM_MenuButtonIndicator, widget ),
                        margin = pixelMetric( PM_ButtonMargin, widget );

                    if ( button->iconSet() && !button->iconSet() ->isNull() &&
                       ( dx + button->iconSet() ->pixmap ( QIconSet::Small, QIconSet::Normal,
                            QIconSet::Off ).width() ) >= w )
                        cornArrow = true; //To little room. Draw the arrow in the corner, don't adjust the widget
                    else {
                        drawPrimitive( PE_ArrowDown,
                            p, visualRect( QRect(
                                ( x + w ) - ( dx + margin ), y, dx,
                                h ), r ), cg, flags, data );
                                
                        w -= dx;
                    }
                }

                // Draw the icon if there is one
                if ( button->iconSet() && !button->iconSet() ->isNull() ) {
                    QIconSet::Mode mode = QIconSet::Disabled;
                    QIconSet::State state = QIconSet::Off;

                    if ( button->isEnabled() )
                        mode = button->hasFocus() ? QIconSet::Active : QIconSet::Normal;
                    if ( button->isToggleButton() && button->isOn() )
                        state = QIconSet::On;

                    QPixmap pixmap = button->iconSet() ->pixmap( QIconSet::Small, mode, state );

                    static const int constSpace = 2;

                    int xo = 0,
                             pw = pixmap.width(),
                                  iw = 0;

                    if ( button->text().isEmpty() && !button->pixmap() )
                        p->drawPixmap( x + ( w >> 1 ) - ( pixmap.width() >> 1 ),
                                       y + ( h >> 1 ) - ( pixmap.height() >> 1 ),
                                       pixmap );
                    else {
                        iw = button->pixmap() ? button->pixmap() ->width()
                             : widget->fontMetrics().size( Qt::ShowPrefix, button->text() ).width();

                        int cw = iw + pw + constSpace;

                        xo = cw < w ? ( w - cw ) >> 1 : constSpace;
                        p->drawPixmap( x + xo, y + ( h >> 1 ) - ( pixmap.height() >> 1 ), pixmap );
                        xo += pw;
                    }

                    if ( cornArrow )   //Draw over the icon
                        drawPrimitive( PE_ArrowDown, p, visualRect( QRect( x + w - 6, x + h - 6, 7, 7 ), r ),
                                       cg, flags, data );

                    if ( xo && iw ) {
                        x += xo + constSpace;
                        w = iw;
                    } else {
                        x += pw + constSpace;
                        w -= pw + constSpace;
                    }
                }

                // Make the label indicate if the button is a default button or not
                int i,
                j = boldDefText && button->isDefault() ? 2 : 1;

                for ( i = 0; i < j; i++ )
                    drawItem( p, QRect( x + i, y, w, h ),
                              AlignCenter | ShowPrefix,
                              button->colorGroup(),
                              button->isEnabled(),
                              button->pixmap(),
                              button->text(), -1,
                              &button->colorGroup().buttonText() );

                //Draw a focus rect if the button has focus
                if ( flags & Style_HasFocus )
                    drawPrimitive( PE_FocusRect, p,
                        QStyle::visualRect( subRect( SR_PushButtonFocusRect, widget ), widget ), cg, flags );
                        
                break;
            }
        case CE_PopupMenuItem: {
                if ( !widget || data.isDefault() )
                    break;

                const QPopupMenu *popupmenu = ( const QPopupMenu * ) widget;
                QMenuItem *mi = data.menuItem();
                int tab = data.tabWidth(),
                    maxpmw = data.maxIconWidth(),
                    x, y, w, h;

                r.rect( &x, &y, &w, &h );

                if ( ( flags & Style_Active ) && ( flags & Style_Enabled ) ) {
                    drawPBarOrMenu( p, r, true, cg, true );
                } else if ( widget->erasePixmap() && !widget->erasePixmap() ->isNull() )
                    p->drawPixmap( x, y, *widget->erasePixmap(), x, y, w, h );
                else {
                    // lighter background in popup menu
                   p->fillRect( r, cg.background().light( 100 + popupmenuHighlightLevel ) );
                }

                if ( !mi )
                    break;

                if ( mi->isSeparator() ) {
                    const QColor * use = backgroundColors( cg );
                    p->setPen( cg.background().dark(105) );
                    p->drawLine( r.left() + 5, r.top() + 3, r.right() - 5, r.top() + 3 );
                    break;
                }

                maxpmw = QMAX( maxpmw, 16 );

                QRect cr, ir, tr, sr;
		if (menuIcons) {
		    // check column
		    cr.setRect( r.left(), r.top(), maxpmw, r.height() );
		    // submenu indicator column
		    sr.setCoords( r.right() - maxpmw, r.top(), r.right(), r.bottom() );
		    // tab/accelerator column
		    tr.setCoords( sr.left() - tab - 4, r.top(), sr.left(), r.bottom() );
		    // item column
		    ir.setCoords( cr.right() + 2, r.top(), tr.right() - 4, r.bottom() );
		} else {
		    // item column
		    ir.setCoords( r.left() + 4, r.top(), r.width() , r.bottom() );
		    // check column
		    cr.setCoords( r.right() - maxpmw, r.top(), r.right(), r.bottom() );
		    // submenu indicator column
		    sr.setCoords( r.right() - maxpmw, r.top(), r.right(), r.bottom() );
		    // tab/accelerator column
		    tr.setCoords( sr.left() - tab - 4, r.top(), sr.left(), r.bottom() );
		}

                bool reverse = QApplication::reverseLayout();

                if ( reverse ) {
                    cr = visualRect( cr, r );
                    sr = visualRect( sr, r );
                    tr = visualRect( tr, r );
                    ir = visualRect( ir, r );
                }

                if ( mi->iconSet() && menuIcons ) {
                    // Select the correct icon from the iconset
                    QIconSet::Mode mode = flags & Style_Active
                            ? ( mi->isEnabled() ? QIconSet::Active : QIconSet::Disabled )
                            : ( mi->isEnabled() ? QIconSet::Normal : QIconSet::Disabled );
                    cr = visualRect( QRect( x, y, maxpmw, h ), r );

                    // Do we have an icon and are checked at the same time?
                    // Then draw a "pressed" background behind the icon
                    if ( popupmenu->isCheckable() && mi->isChecked() ) {

                        QBrush brush( gray[ 3 ] );

                        qDrawShadePanel( p,
                             cr.x() + 1, cr.y() + 2, cr.width() - 2, cr.height() - 4,
                             QColorGroup( gray[ 5 ], gray[ NUM_SHADES ], gray[ 0 ], gray[ 5 ],
                                          gray[ 2 ], cg.text(), gray[ NUM_SHADES ] ),
                             true, 1, &brush );
                    }
                    // Draw the icon
                    QPixmap pixmap = mi->iconSet() ->pixmap( QIconSet::Small, mode );
                    QRect pmr( 0, 0, pixmap.width(), pixmap.height() );

                    pmr.moveCenter( cr.center() );
                    p->drawPixmap( pmr.topLeft(), pixmap );
                    
                } else if ( popupmenu->isCheckable() && mi->isChecked() ) {

		    // check column
		    cr.setRect( r.left(), r.top(), maxpmw, r.height() );
		    // submenu indicator column
		    sr.setCoords( r.right() - maxpmw, r.top(), r.right(), r.bottom() );
		    // tab/accelerator column
		    tr.setCoords( sr.left() - tab - 4, r.top(), sr.left(), r.bottom() );
		    // item column
		    ir.setCoords( cr.right() + 2, r.top(), tr.right() - 4, r.bottom() );

                    QBrush brush( mi->isEnabled() ? cg.highlightedText() : cg.background() );
                    drawPrimitiveMenu( PE_CheckMark, p, cr, cg,
                        ( flags & ( Style_Enabled | Style_Active ) ) | Style_On );
                }

                QColor textcolor, embosscolor;

                if ( flags & Style_Active ) {
                    if ( !( flags & Style_Enabled ) ) {
                        textcolor = cg.text();
                        embosscolor = cg.light();
                    } else {
                        textcolor = cg.highlightedText();
                        embosscolor = cg.midlight().light();
                    }
                } else if ( !( flags & Style_Enabled ) ) {
                    textcolor = cg.text();
                    embosscolor = cg.light();
                } else
                    textcolor = embosscolor = cg.buttonText();
                p->setPen( textcolor );

                if ( mi->custom() ) {
                    p->save();
                    if ( !( flags & Style_Enabled ) ) {
                        p->setPen( cg.light() );
                        mi->custom() ->paint( p, cg, ( flags & Style_Enabled ) ? ( flags & Style_Active ) : 0,
                                              flags & Style_Enabled, ir.x() + 1, ir.y() + 1, ir.width() - 1,
                                              ir.height() - 1 );
                        p->setPen( textcolor );
                    }
                    mi->custom() ->paint( p, cg, ( flags & Style_Enabled ) ? ( flags & Style_Active ) : 0,
                                          flags & Style_Enabled, ir.x(), ir.y(), ir.width(), ir.height() );
                    p->restore();
                }

                QString text = mi->text();

                if ( !text.isNull() ) {
                    int t = text.find( '\t' );

                    // draw accelerator/tab-text
                    if ( t >= 0 ) {
                        int alignFlag = AlignVCenter | ShowPrefix | DontClip | SingleLine;

                        alignFlag |= ( reverse ? AlignLeft : AlignRight );

                        if ( !( flags & Style_Enabled ) ) {
                            p->setPen( embosscolor );
                            tr.moveBy( 1, 1 );
                            p->drawText( tr, alignFlag, text.mid( t + 1 ) );
                            tr.moveBy( -1, -1 );
                            p->setPen( textcolor );
                        }

                        p->drawText( tr, alignFlag, text.mid( t + 1 ) );
                    }

                    int alignFlag = AlignVCenter | ShowPrefix | DontClip | SingleLine;

                    alignFlag |= ( reverse ? AlignRight : AlignLeft );

                    if ( !( flags & Style_Enabled ) ) {
                        p->setPen( embosscolor );
                        ir.moveBy( 1, 1 );
                        p->drawText( ir, alignFlag, text, t );
                        ir.moveBy( -1, -1 );
                        p->setPen( textcolor );
                    }

                    p->drawText( ir, alignFlag, text, t );
                } else if ( mi->pixmap() ) {
                    QPixmap pixmap = *mi->pixmap();

                    if ( 1 == pixmap.depth() )
                        p->setBackgroundMode( OpaqueMode );
                    p->drawPixmap( ir.x(), ( ir.height() - pixmap.height() ) >> 1, pixmap );
                    if ( pixmap.depth() == 1 )
                        p->setBackgroundMode( TransparentMode );
                }

                if ( mi->popup() )
                    drawArrow( p, sr, cg, flags, reverse ? PE_ArrowLeft : PE_ArrowRight, false, true );
                break;
            }
        case CE_MenuBarItem: {
                if ( ( flags & Style_Enabled ) &&
                     ( flags & Style_Active ) &&
                     ( flags & Style_Down ) ) {                    
                    drawPBarOrMenu2( p, QRect(r.x(), r.y(), r.width(), r.height()),
                        true, cg, true );
                } else
#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS
                    if ( APPEARANCE_FLAT != appearance )
                        drawBevelGradient( cg.background(), true, 0, p,
                        r,
                        true, SHADE_BAR_LIGHT, SHADE_BAR_DARK );
                    else
#endif
                        if (darkMenubar) {
			    drawBevelGradient( cg.background(), true, 1, p,
				    QRect(r.x()-1, r.y()-1, r.width()+2, r.height()+2), 
				    true, 
				    SHADE_BEVEL_MENU_GRAD_LIGHT( appearance ), SHADE_BEVEL_MENU_GRAD_DARK( appearance ));
                        } else
                            p->fillRect( r, cg.background() );

                if ( data.isDefault() )
                    break;

                QMenuItem *mi = data.menuItem();

                if ( flags & Style_Active && ( flags & Style_Down ) )
                    drawItem( p, r, AlignCenter | ShowPrefix | DontClip | SingleLine,
                        cg, flags & Style_Enabled, mi->pixmap(), mi->text(), -1, &cg.highlightedText() );
                else
                    drawItem( p, r, AlignCenter | ShowPrefix | DontClip | SingleLine, cg,
                        flags & Style_Enabled, mi->pixmap(), mi->text(), -1, &cg.buttonText() );
                break;
            }
        case CE_MenuBarEmptyArea:
            if (darkMenubar) {
                //p->fillRect( r, cg.background().dark( 106 ) );
		QColor b;
		b.setRgb(cg.background().red(), cg.background().green(), cg.background().blue());
                drawBevelGradient( b, true, 1, p, 
			QRect(r.x()-1, r.y()-1, r.width()+2, r.height()+2 ), 
			true, 
			SHADE_BEVEL_MENU_GRAD_LIGHT( appearance ), SHADE_BEVEL_MENU_GRAD_DARK( appearance ));
            } else
                p->fillRect( r, cg.background() );
            break;

        case CE_DockWindowEmptyArea:
#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS

            if ( APPEARANCE_FLAT != appearance )
                drawBevelGradient( cg.background(), true, 1, p, r, true, SHADE_BAR_LIGHT, SHADE_BAR_DARK );
            else
#endif

                p->fillRect( r, cg.background() );
            break;
        case CE_ProgressBarGroove:
            p->setBrush( gray[ NUM_SHADES ] );
            p->drawRect( r );
            qDrawShadePanel( p, r,
                QColorGroup( gray[ 5 ], gray[ NUM_SHADES ], gray[ 0 ], gray[ 5 ], gray[ 2 ],
                cg.text(), gray[ NUM_SHADES ] ), true, 1 );
                
            break;
            
        case CE_ProgressBarContents: {
                // ### Take into account totalSteps()for busy indicator
                const QProgressBar *pb = ( const QProgressBar* ) widget;
                QRect cr = subRect( SR_ProgressBarContents, widget );
                double progress = pb->progress();
                bool reverse = QApplication::reverseLayout();
                int steps = pb->totalSteps();

                if ( cr.isValid() && ( progress > 0 || steps == 0 ) ) {
                    double pg = ( steps == 0 ) ? 0.1 : progress / steps;
                    int width = QMIN( cr.width(), ( int ) ( pg * cr.width() ) );

                    if ( 0 == steps )  //Busy indicator
                    {
                        if ( width < 1 )
                            width = 1; //A busy indicator with width 0 is kind of useless

                        int remWidth = cr.width() - width; //Never disappear completely

                        if ( remWidth <= 0 )
                            remWidth = 1; //Do something non-crashy when too small...

                        int pstep = int( progress ) % ( 2 * remWidth );

                        if ( pstep > remWidth ) {
                            //Bounce about.. We're remWidth +some delta, we want to be remWidth-delta...
                            //-((remWidth +some delta)-2* remWidth)=-(some deleta-remWidth)=remWidth-some delta..
                            pstep = -( pstep - 2 * remWidth );
                        }

                        if ( reverse )
                            drawPBarOrMenu( p, QRect( cr.x() + cr.width() - width - pstep,
                                cr.y(), width, cr.height() ), true, cg );
                        else
                            drawPBarOrMenu( p, QRect( cr.x() + pstep, cr.y(), width,
                                cr.height() ), true, cg );
                    } else
                        if ( reverse )
                            drawPBarOrMenu( p, QRect( cr.x() + ( cr.width() - width ),
                                cr.y(), width, cr.height() ), true, cg );
                        else
                            drawPBarOrMenu( p, QRect( cr.x(), cr.y(), width, cr.height() ), true, cg );
                }
                break;
            }
        case CE_PushButton: {
                const QPushButton *button = static_cast<const QPushButton *>( widget );
                QRect br( r );
                int dbi = pixelMetric( PM_ButtonDefaultIndicator, widget );

                if ( rounded && isFormWidget( widget ) )
                    formMode = true;

                if ( widget == hoverWidget )
                    flags |= Style_MouseOver;

                if ( IND_BORDER == defBtnIndicator )
                    br.setCoords( br.left() + dbi, br.top() + dbi, br.right() - dbi, br.bottom() - dbi );
                else if ( IND_FONT_COLOUR == defBtnIndicator && button->isDefault() )
                    flags |= Style_ButtonDefault;

                p->save();
                p->setBrushOrigin( -widget->backgroundOffset().x(), -widget->backgroundOffset().y() );
                // draw button
                drawPrimitive( PE_ButtonCommand, p, br, cg, flags );
                if ( button->isDefault() && IND_FONT_COLOUR != defBtnIndicator )
                    drawPrimitive( PE_ButtonDefault, p, r, cg, flags );
                p->restore();
                formMode = false;
                break;
            }
        case CE_CheckBox:
            drawPrimitive( PE_Indicator, p, r, cg, flags, data );
            break;
        case CE_CheckBoxLabel:
            if ( crLabelHighlight ) {
                const QCheckBox * checkbox = ( const QCheckBox * ) widget;

                if ( flags & Style_MouseOver &&
#if QT_VERSION >= 0x030200
                        HOVER_CHECK == hover && hoverWidget == widget &&
#endif
                        !isFormWidget( widget ) ) {
#if QT_VERSION >= 0x030200
                    QRect cr( checkbox->rect() );
                    QRegion r( QRect( cr.x(), cr.y(),
                        visualRect( subRect( SR_CheckBoxFocusRect, widget ), widget ).width() +
                                    pixelMetric( PM_IndicatorWidth ) + 4, cr.height() ) );

#else

                    QRegion r( checkbox->rect() );
#endif

                    r -= visualRect( subRect( SR_CheckBoxIndicator, widget ), widget );
                    p->setClipRegion( r );
                    p->fillRect( checkbox->rect(), cg.background().light( QTC_HIGHLIGHT_FACTOR ) );
                    p->setClipping( false );
                }
                int alignment = QApplication::reverseLayout() ? AlignRight : AlignLeft;

                drawItem( p, r, alignment | AlignVCenter | ShowPrefix, cg,
                    flags & Style_Enabled, checkbox->pixmap(), checkbox->text() );

                if ( checkbox->hasFocus() )
                    drawPrimitive( PE_FocusRect, p,
                        visualRect( subRect( SR_CheckBoxFocusRect, widget ), widget ), cg, flags );
            } else
                KStyle::drawControl( control, p, widget, r, cg, flags, data );
            break;
        case CE_RadioButton:
            formMode = isFormWidget( widget );
            drawPrimitive( PE_ExclusiveIndicator, p, r, cg, flags, data );
            formMode = false;
            break;
        case CE_RadioButtonLabel:
            if ( crLabelHighlight ) {
                const QRadioButton * radiobutton = ( const QRadioButton * ) widget;

                if ( flags & Style_MouseOver &&
#if QT_VERSION >= 0x030200
                        HOVER_RADIO == hover && hoverWidget == widget &&
#endif
                        !isFormWidget( widget ) ) {
#if QT_VERSION >= 0x030200
                    QRect rb( radiobutton->rect() );
                    QRegion r( QRect( rb.x(), rb.y(),
                                      visualRect( subRect( SR_RadioButtonFocusRect, widget ), widget ).width() +
                                      pixelMetric( PM_ExclusiveIndicatorWidth ) + 4, rb.height() ) );
#else

                    QRegion r( radiobutton->rect() );
#endif

                    r -= visualRect( subRect( SR_RadioButtonIndicator, widget ), widget );
                    p->setClipRegion( r );
                    p->fillRect( radiobutton->rect(), cg.background().light( QTC_HIGHLIGHT_FACTOR ) );
                    p->setClipping( false );
                }

                int alignment = QApplication::reverseLayout() ? AlignRight : AlignLeft;

                drawItem( p, r, alignment | AlignVCenter | ShowPrefix, cg,
                    flags & Style_Enabled, radiobutton->pixmap(), radiobutton->text() );

                if ( radiobutton->hasFocus() )
                    drawPrimitive( PE_FocusRect, p,
                        visualRect( subRect( SR_RadioButtonFocusRect, widget ), widget ), cg, flags );
                break;
            }
        default:
            KStyle::drawControl( control, p, widget, r, cg, flags, data );
    }
}

void KlearlookStyle::drawControlMask( ControlElement control, QPainter *p, const QWidget *widget, const QRect &r,
                                      const QStyleOption &data ) const {
    switch ( control ) {
        case CE_PushButton:
            if ( rounded ) {
                int offset = r.width() < QTC_MIN_BTN_SIZE || r.height() < QTC_MIN_BTN_SIZE ? 1 : 2;

                p->fillRect( r, color0 );
                p->fillRect( r.x() + 1, r.y() + 1, r.width() - 2, r.height() - 2, color1 );
                p->setPen( color1 );
                p->drawLine( r.x() + offset, r.y(), r.x() + r.width() - ( offset + 1 ), r.y() );
                p->drawLine( r.x() + offset, r.y() + r.height() - 1,
                    r.x() + r.width() - ( offset + 1 ), r.y() + r.height() - 1 );
                p->drawLine( r.x(), r.y() + offset, r.x(), r.y() + r.height() - ( offset + 1 ) );
                p->drawLine( r.x() + r.width() - 1, r.y() + offset,
                    r.x() + r.width() - 1, r.y() + r.height() - ( offset + 1 ) );
            } else
                p->fillRect( r, color1 );
            break;
        default:
            KStyle::drawControlMask( control, p, widget, r, data );
    }
}

void KlearlookStyle::drawComplexControlMask( ComplexControl control, QPainter *p, const QWidget *widget, const QRect &r,
        const QStyleOption &data ) const {
    switch ( control ) {
        case CC_ToolButton:
        case CC_ComboBox:
            drawControlMask( CE_PushButton, p, widget, r, data );
            break;
        default:
            KStyle::drawComplexControlMask( control, p, widget, r, data );
    }
}

QRect KlearlookStyle::subRect( SubRect subrect, const QWidget *widget ) const {
    QRect rect,
    wrect( widget->rect() );

    switch ( subrect ) {
        case SR_PushButtonFocusRect: {
                //            const QPushButton *button=(const QPushButton *)widget;
                int dbw1 = 0,
                           dbw2 = 0;

                //            if(button->isDefault() || button->autoDefault())
                //            {
                dbw1 = pixelMetric( PM_ButtonDefaultIndicator, widget );
                dbw2 = dbw1 * 2;
                //            }

                rect.setRect( wrect.x() + 3 + dbw1,
                              wrect.y() +3  + dbw1,
                              wrect.width() - 6 - dbw2,
                              wrect.height() - 6 - dbw2 );
                break;
            }
        case SR_CheckBoxIndicator: {
                int h = pixelMetric( PM_IndicatorHeight );

                rect.setRect( ( widget->rect().height() - h ) >> 1,
                              ( widget->rect().height() - h ) >> 1,
                              pixelMetric( PM_IndicatorWidth ),
                              h );
                break;
            }
        case SR_RadioButtonIndicator: {
                int h = pixelMetric( PM_ExclusiveIndicatorHeight );

                rect.setRect( ( widget->rect().height() - h ) >> 1,
                              ( widget->rect().height() - h ) >> 1,
                              pixelMetric( PM_ExclusiveIndicatorWidth ), h );
                break;
            }
        case SR_ProgressBarContents:
            rect = QRect( wrect.x() + 1,
                          wrect.y() + 1,
                          wrect.width() - 2,
                          wrect.height() - 2 );
            break;
        default:
            rect = KStyle::subRect( subrect, widget );
    }

    return rect;
}

void KlearlookStyle::drawComplexControl(
        ComplexControl control,
        QPainter *p,
        const QWidget *widget,
        const QRect &r,
        const QColorGroup &cg,
        SFlags flags,
        SCFlags controls,
        SCFlags active,
        const QStyleOption &data ) const
{
    if ( widget == hoverWidget )
        flags |= Style_MouseOver;

    switch ( control ) {
        case CC_ToolButton: {
        
                const QToolButton * toolbutton = ( const QToolButton * ) widget;
                QRect button  ( querySubControlMetrics( control, widget, SC_ToolButton, data ) ),
                      menuarea( querySubControlMetrics( control, widget, SC_ToolButtonMenu, data ) );
                
                SFlags bflags = flags, mflags = flags;

                if ( APP_KORN == themedApp ) {
                    drawPrimitive( PE_ButtonTool, p, button, cg, bflags, data );
                    break;
                }

                bool onControlButtons = false,
                     onToolbar = widget->parentWidget() && ::qt_cast<QToolBar *>( widget->parentWidget() ),
                     onExtender = !onToolbar &&
                                  widget->parentWidget() &&
                                  widget->parentWidget() ->inherits( "QToolBarExtensionWidget" ) &&
                                  ::qt_cast<QToolBar *>( widget->parentWidget() ->parentWidget() );

                if ( !onToolbar && !onExtender && widget->parentWidget() &&
                        !qstrcmp( widget->parentWidget() ->name(), "qt_maxcontrols" ) )
                    onControlButtons = true;

                if ( active & SC_ToolButton )
                    bflags |= Style_Down;
                    
                if ( active & SC_ToolButtonMenu )
                    mflags |= Style_Down;

                if ( controls & SC_ToolButton ) {
                    // If we're pressed, on, or raised...
#if KDE_VERSION >= 0x30200
                    if ( bflags & ( Style_Down | Style_On | Style_Raised ) || onControlButtons )
#else
                    // CPD: Style_MouseOver obove is *needed* for KDE's KToggleActions...
                    if ( bflags & ( Style_Down | Style_On | Style_Raised | Style_MouseOver ) || onControlButtons )
#endif

                    {
                        //Make sure the standalone toolbuttons have a gradient in the right direction
                        if ( !onToolbar && !onControlButtons )
                            bflags |= Style_Horizontal;

                        drawPrimitive( PE_ButtonTool, p, button, cg, bflags, data );
                    }

                    // Check whether to draw a background pixmap
                    else if ( toolbutton->parentWidget() &&
                              toolbutton->parentWidget() ->backgroundPixmap() &&
                              !toolbutton->parentWidget() ->backgroundPixmap() ->isNull() ) {
                        p->drawTiledPixmap( r,
                            *( toolbutton->parentWidget() ->backgroundPixmap() ), toolbutton->pos() );
                    } else if ( widget->parent() ) {
                        if ( ::qt_cast<const QToolBar *>( widget->parent() ) ) {
                            QToolBar * parent = ( QToolBar* ) widget->parent();

#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS

                            if ( APPEARANCE_FLAT != appearance )
                                drawBevelGradient( cg.background(), true, 0,
                                    p, parent->rect(), true, SHADE_BAR_LIGHT, SHADE_BAR_DARK );
                            else
#endif

                                p->fillRect( parent->rect(), cg.background() );
                        } else if ( widget->parent() ->inherits( "QToolBarExtensionWidget" ) ) {
                            QWidget * parent = ( QWidget* ) widget->parent();
                            QToolBar *toolbar = ( QToolBar* ) parent->parent();

#ifdef QTC_GRADIENT_TOOLBARS_AND_MENUBARS

                            if ( APPEARANCE_FLAT != appearance )
                                drawBevelGradient( cg.background(), true, 0, p, toolbar->rect(),
                                    true, SHADE_BAR_LIGHT, SHADE_BAR_DARK );
                            else
#endif

                                p->fillRect( toolbar->rect(), cg.background() );
                        }
                    }
                }

                if ( controls & SC_ToolButtonMenu ) {
                    if ( mflags & ( Style_Down | Style_On | Style_Raised ) )
                        drawPrimitive( PE_ButtonDropDown, p, menuarea, cg, mflags, data );
                    drawPrimitive( PE_ArrowDown, p, menuarea, cg, mflags, data );
                }

                if ( toolbutton->hasFocus() && !toolbutton->focusProxy() ) {
                    QRect fr = toolbutton->rect();
                    fr.addCoords( 3, 3, -3, -3 );
                    drawPrimitive( PE_FocusRect, p, fr, cg );
                }
                break;
            }
        case CC_ComboBox: {
                const QComboBox *combobox = ( const QComboBox * ) widget;
                
                QRect frame( QStyle::visualRect( querySubControlMetrics( CC_ComboBox,
                             widget,SC_ComboBoxFrame,data ),widget ) ),
                             
                      arrow( QStyle::visualRect( querySubControlMetrics( CC_ComboBox,
                             widget,SC_ComboBoxArrow,data),widget)),
                             
                      field( QStyle::visualRect( querySubControlMetrics(CC_ComboBox,
                             widget,SC_ComboBoxEditField,data),widget));
                             
                const QColor *use = buttonColors( cg );

                if ( rounded && isFormWidget( widget ) )
                    formMode = true;

                if ( controls & SC_ComboBoxFrame && frame.isValid() ) {
                    if ( controls & SC_ComboBoxEditField && field.isValid() && combobox->editable() ) {
                        QRect f2( field );
                        QRegion reg( r );

                        f2.addCoords( -1, -1, 1, 1 );
                        reg -= f2;
                        p->setClipRegion( reg );
                    }
                    drawLightBevelButton( p, r, cg, flags | Style_Raised | Style_Horizontal,
                        true, ROUNDED_ALL, getFill( flags, use ),
                                          use );
                    p->setClipping( false );
                }

                if ( controls & SC_ComboBoxArrow && arrow.isValid() ) {
                    drawPrimitive( PE_ArrowDown, p, arrow, cg, flags & ~Style_MouseOver );
		    p->setPen( use[ 4 ].light(70) );
                    arrow.addCoords( -1, -1, -1, 1 );
		    p->drawLine( arrow.left(), arrow.top(), arrow.left(), arrow.bottom() );
		 }

                if ( controls & SC_ComboBoxEditField && field.isValid() ) {
                    if ( ( flags & Style_HasFocus ) && ( ! combobox->editable() ) ) {
                        QRect fr = QStyle::visualRect( subRect( SR_ComboBoxFocusRect, widget ), widget );

                        fr.addCoords( 0, 0, -2, 0 );
                        drawPrimitive( PE_FocusRect,
                            p, fr, cg, flags | Style_FocusAtBorder, QStyleOption( cg.highlight() ) );
                    }
                }

                p->setPen( flags & Style_Enabled ? cg.buttonText() : cg.mid() );
                formMode = false;
                break;
            }
        case CC_SpinWidget: {
                const QSpinWidget *spinwidget = ( const QSpinWidget * ) widget;
                QRect frame( querySubControlMetrics( CC_SpinWidget, widget, SC_SpinWidgetFrame, data ) ),
                up( spinwidget->upRect() ),
                down( spinwidget->downRect() );

                if ( hoverWidget && spinwidget == hoverWidget )
                    flags |= Style_MouseOver;

                if ( ( controls & SC_SpinWidgetFrame ) && frame.isValid() )
                    qDrawShadePanel(
                        p, r, QColorGroup( gray[ 5 ], gray[ NUM_SHADES ], gray[ 0 ],
                        gray[ 5 ], gray[ 2 ], cg.text(), gray[ NUM_SHADES ] ),
                        true, pixelMetric( PM_SpinBoxFrameWidth )
                        );

                if ( ( controls & SC_SpinWidgetUp ) && up.isValid() ) {
                    PrimitiveElement pe = PE_SpinWidgetUp;
                    SFlags upflags = flags;

                    if ( spinwidget->buttonSymbols() == QSpinWidget::PlusMinus )
                        pe = PE_SpinWidgetPlus;
                    if ( !spinwidget->isUpEnabled() )
                        upflags ^= Style_Enabled;
                    drawPrimitive(
                        pe, p, up, cg,
                        upflags | ( ( active == SC_SpinWidgetUp ) ? Style_On | Style_Sunken : Style_Raised )
                    );
                }

                if ( ( controls & SC_SpinWidgetDown ) && down.isValid() ) {
                    PrimitiveElement pe = PE_SpinWidgetDown;
                    SFlags downflags = flags;

                    if ( spinwidget->buttonSymbols() == QSpinWidget::PlusMinus )
                        pe = PE_SpinWidgetMinus;
                    if ( !spinwidget->isDownEnabled() )
                        downflags ^= Style_Enabled;
                    drawPrimitive(
                        pe, p, down, cg,
                        downflags | ( ( active == SC_SpinWidgetDown ) ? Style_On | Style_Sunken : Style_Raised )
                    );
                }
                const QColor *use = backgroundColors( cg );
		p->setPen( use[ 4 ].light(80) );
		p->drawRect( r );
                break;
            }
        case CC_ScrollBar: {
                const QScrollBar *scrollbar = ( const QScrollBar * ) widget;
                bool hw = hoverWidget == scrollbar;
                QRect subline( querySubControlMetrics( control, widget, SC_ScrollBarSubLine, data ) ),
                addline( querySubControlMetrics( control, widget, SC_ScrollBarAddLine, data ) ),
                subpage( querySubControlMetrics( control, widget, SC_ScrollBarSubPage, data ) ),
                addpage( querySubControlMetrics( control, widget, SC_ScrollBarAddPage, data ) ),
                slider( querySubControlMetrics( control, widget, SC_ScrollBarSlider, data ) ),
                first( querySubControlMetrics( control, widget, SC_ScrollBarFirst, data ) ),
                last( querySubControlMetrics( control, widget, SC_ScrollBarLast, data ) );

                if ( ( controls & SC_ScrollBarSubLine ) && subline.isValid() )
                    drawPrimitive(
                        PE_ScrollBarSubLine, p, subline, cg,
                        ( hw && HOVER_SB_SUB == hover ? Style_MouseOver : Style_Default ) |
                        Style_Enabled |
                        ( ( active == SC_ScrollBarSubLine ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default )
                    );
                if ( ( controls & SC_ScrollBarAddLine ) && addline.isValid() )
                    drawPrimitive(
                        PE_ScrollBarAddLine, p, addline, cg,
                        ( hw && HOVER_SB_ADD == hover ? Style_MouseOver : Style_Default ) |
                        Style_Enabled |
                        ( ( active == SC_ScrollBarAddLine ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default )
                    );
                if ( ( controls & SC_ScrollBarSubPage ) && subpage.isValid() )
                    drawPrimitive( PE_ScrollBarSubPage, p, subpage, cg,
                        Style_Enabled |
                        ( ( active == SC_ScrollBarSubPage ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default ) );
                if ( ( controls & SC_ScrollBarAddPage ) && addpage.isValid() )
                    drawPrimitive( PE_ScrollBarAddPage, p, addpage, cg,
                        ( ( scrollbar->minValue() == scrollbar->maxValue() ) ? Style_Default : Style_Enabled ) |
                        ( ( active == SC_ScrollBarAddPage ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default ) );
                if ( ( controls & SC_ScrollBarFirst ) && first.isValid() )
                    drawPrimitive( PE_ScrollBarFirst, p, first, cg,
                        Style_Enabled |
                        ( ( active == SC_ScrollBarFirst ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default ) );
                if ( ( controls & SC_ScrollBarLast ) && last.isValid() )
                    drawPrimitive( PE_ScrollBarLast, p, last, cg,
                        Style_Enabled |
                        ( ( active == SC_ScrollBarLast ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default ) );
                if ( ( controls & SC_ScrollBarSlider ) && slider.isValid() ) {
                    drawPrimitive( PE_ScrollBarSlider, p, slider, cg,
                        ( hw && HOVER_SB_SLIDER == hover ? Style_MouseOver : Style_Default ) |
                        Style_Enabled |
                        ( ( active == SC_ScrollBarSlider ) ? Style_Down : Style_Default ) |
                        ( ( scrollbar->orientation() == Qt::Horizontal ) ? Style_Horizontal : Style_Default ) );

                    // ### perhaps this should not be able to accept focus if maxedOut?
                    if ( scrollbar->hasFocus() )
                        drawPrimitive( PE_FocusRect, p,
                            QRect( slider.x() + 2, slider.y() + 2, slider.width() - 5, slider.height() - 5 ),
                                   cg, Style_Default );
                }
                break;
            }
        case CC_Slider: {
                QRect groove = querySubControlMetrics( CC_Slider, widget, SC_SliderGroove, data ),
                               handle = querySubControlMetrics( CC_Slider, widget, SC_SliderHandle, data );

                if ( ( controls & SC_SliderGroove ) && groove.isValid() )
                    drawSliderGroove( p, groove, flags, widget );
                if ( ( controls & SC_SliderHandle ) && handle.isValid() )
                    drawSliderHandle( p, handle, cg, flags );
                if ( controls & SC_SliderTickmarks )
                    QCommonStyle::drawComplexControl(
                        control, p, widget, r, cg, flags, SC_SliderTickmarks, active, data
                    );
                break;
            }
        default:
            KStyle::drawComplexControl( control, p, widget, r, cg, flags, controls, active, data );
    }
}

QRect KlearlookStyle::querySubControlMetrics( ComplexControl control, const QWidget *widget, SubControl sc,
        const QStyleOption &data ) const {
    switch ( control ) {
        case CC_SpinWidget: {
                if ( !widget )
                    return QRect();

                int fw = pixelMetric( PM_SpinBoxFrameWidth, 0 );
                QSize bs;

                bs.setHeight( widget->height() >> 1 );
                if ( bs.height() < 8 )
                    bs.setHeight( 8 );
                bs.setWidth( QMIN( bs.height() * 8 / 6, widget->width() / 4 ) );
                bs = bs.expandedTo( QApplication::globalStrut() );

                if ( !( bs.width() % 2 ) )
                    bs.setWidth( bs.width() + 1 );

                int extra = bs.height() * 2 == widget->height() ? 0 : 1;
                int y = 0,
                        x = widget->width() - y - bs.width(),
                            lx = fw,
                                 rx = x - fw * 2;

                switch ( sc ) {
                    case SC_SpinWidgetUp:
                        return QRect( x, y, bs.width(), bs.height() );
                    case SC_SpinWidgetDown:
                        return QRect( x, y + bs.height(), bs.width(), bs.height() + extra );
                    case SC_SpinWidgetButtonField:
                        return QRect( x, y, bs.width(), widget->height() - 2 * fw );
                    case SC_SpinWidgetEditField:
                        return QRect( lx, fw, rx, widget->height() - 2 * fw );
                    case SC_SpinWidgetFrame:
                        return QRect( widget->x(), widget->y(), widget->width() - bs.width(), widget->height() );
                }
            }
        default:
            return KStyle::querySubControlMetrics( control, widget, sc, data );
    }
}

int KlearlookStyle::pixelMetric( PixelMetric metric, const QWidget *widget ) const {
    switch ( metric ) {
        case PM_MenuButtonIndicator:
            return 7;
        case PM_MenuBarItemSpacing: {
                return 5;
            }
        case PM_ButtonMargin:
            return 5;
#if QT_VERSION >= 0x030200

        case PM_TabBarTabShiftVertical: {
                const QTabBar *tb = ::qt_cast<const QTabBar *>( widget );

                return QTabBar::RoundedAbove == tb->shape() || QTabBar::TriangularAbove == tb->shape()
                       ? 1
                       : -1;
            }
        case PM_TabBarTabShiftHorizontal:
            return 0;
#endif

        case PM_TabBarTabVSpace: {
                const QTabBar * tb = ( const QTabBar * ) widget;
                if ( tb->shape() == QTabBar::RoundedAbove ||
                        tb->shape() == QTabBar::RoundedBelow )
                    return 12;
                else
                    return 4;
            }


        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            return 1;
        case PM_ButtonDefaultIndicator:
            return IND_BORDER == defBtnIndicator ? 1 : 0;
        case PM_DefaultFrameWidth:
            return borderFrame && widget && ( ::qt_cast<const QTabBar *>( widget ) ||
                                              ::qt_cast<const QWidgetStack *>( widget ) ||
                                              ::qt_cast<const QPopupMenu *>( widget ) )
                   ? 2
                   : QTC_DEF_FRAME_WIDTH;
        case PM_SpinBoxFrameWidth:
            return 1;
        case PM_MenuBarFrameWidth:
            return 1;
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
            return QTC_CHECK_SIZE;
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
            return QTC_RADIO_SIZE;
        case PM_TabBarTabOverlap:
            return 1;
        case PM_ProgressBarChunkWidth:
            return 2;
        case PM_DockWindowSeparatorExtent:
            return 4;
        case PM_DockWindowHandleExtent:
            return 10;
        case PM_SplitterWidth:
            return 4;
        case PM_ScrollBarSliderMin:
            return 16;
        case PM_ScrollBarExtent:
        case PM_SliderControlThickness:
        case PM_SliderThickness:
            return 15;
        case PM_SliderLength:
            return 24;
        case PM_MaximumDragDistance:
            return -1;
        default:
            return KStyle::pixelMetric( metric, widget );
    }
}

int KlearlookStyle::kPixelMetric( KStylePixelMetric kpm, const QWidget *widget ) const {
    switch ( kpm ) {
        case KPM_MenuItemSeparatorHeight:
            return 4;
        default:
            return KStyle::kPixelMetric( kpm, widget );
    }
}

QSize KlearlookStyle::sizeFromContents( ContentsType t,
                                        const QWidget *widget,
                                        const QSize &s,
                                        const QStyleOption &opt ) const {
    switch ( t ) {
        case CT_PopupMenuItem: {
                if ( !widget || opt.isDefault() )
                    return s;

                const QPopupMenu *popup = dynamic_cast<const QPopupMenu *>( widget );
                QMenuItem *mi = opt.menuItem();
                int maxpmw = opt.maxIconWidth();
                int w = s.width(), h = s.height();
                bool checkable = popup->isCheckable();

                if ( mi->custom() ) {
                    w = mi->custom() ->sizeHint().width();
                    h = mi->custom() ->sizeHint().height();
                    if ( !mi->custom() ->fullSpan() )
                        h += 4;
                } else if ( mi->widget() ) {
                    // don't change the size in this case.
                } else if ( mi->isSeparator() ) {
                    w = 20;
                    h = 8;
                } else {
                    if ( mi->pixmap() ) {
                        h = QMAX( h, mi->pixmap() ->height() + 2 );
                    } else {
                        h = QMAX( h, 21 );
                        QSettings s;
                        if ( menuIcons )
                            h = QMAX( h, popup->fontMetrics().height() + MENU_POPUP_ITEM_HIGH_HI );
                        else
                            h = QMAX( h, popup->fontMetrics().height() + MENU_POPUP_ITEM_HIGH_LO );
                    }

                    if ( mi->iconSet() ) {
                        h = QMAX( h, mi->iconSet() ->pixmap( QIconSet::Small, QIconSet::Normal ).height() + 2 );
                    }
                }

                if ( !mi->text().isNull() && ( mi->text().find( '\t' ) >= 0 ) ) {
                    w += itemHMargin + itemFrame * 2 + 7;
                } else if ( mi->popup() ) {
                    w += 2 * arrowHMargin;
                }

                if ( maxpmw ) {
                    w += maxpmw + 6;
                }
                if ( checkable && maxpmw < 20 ) {
                    w += 20 - maxpmw;
                }
                if ( checkable || maxpmw > 0 ) {
                    w += 12;
                }

                w += rightBorder;

                return QSize( w-25, h );
            }

        case CT_PushButton: {
                const QPushButton* btn = static_cast<const QPushButton*>( widget );

                int w = s.width() + 2 * pixelMetric( PM_ButtonMargin, widget );
                int h = s.height() + 2 * pixelMetric( PM_ButtonMargin, widget );
                if ( btn->text().isEmpty() && s.width() < 32 )
                    return QSize( w, h );
                // return button size
                return QSize( w + 25, h + 3 );
            }

        case CT_ToolButton: {
                if ( widget->parent() && ::qt_cast<QToolBar*>( widget->parent() ) )
                    return QSize( s.width() + 2 * 4, s.height() + 2 * 4 );
                else {
                    return KStyle::sizeFromContents ( t, widget, s, opt );
                }
            }

        default:
            return KStyle::sizeFromContents ( t, widget, s, opt );
    }

    return KStyle::sizeFromContents ( t, widget, s, opt );
}



int KlearlookStyle::styleHint( StyleHint stylehint, const QWidget *widget, const QStyleOption &option, QStyleHintReturn *returnData ) const {
    switch ( stylehint ) {
        case SH_EtchDisabledText:
        case SH_Slider_SnapToValue:
        case SH_PrintDialog_RightAlignButtons:
        case SH_FontDialog_SelectAssociatedText:
        case SH_MenuBar_AltKeyNavigation:
        case SH_MenuBar_MouseTracking:
        case SH_PopupMenu_MouseTracking:
        case SH_PopupMenu_SpaceActivatesItem:
        case SH_ComboBox_ListMouseTracking:
        case SH_ScrollBar_MiddleClickAbsolutePosition:
            return 1;
        case SH_MainWindow_SpaceBelowMenuBar:
            return 0;
        case SH_ComboBox_Popup:
            return 0;
        case SH_PopupMenu_SubMenuPopupDelay:
            return 300;
        case SH_PopupMenu_AllowActiveAndDisabled:
            return 0;
        default:
            return KStyle::styleHint( stylehint, widget, option, returnData );
    }
}

void KlearlookStyle::drawPBarOrMenu(
    QPainter *p,
    QRect const &r,
    bool horiz,
    const QColorGroup &cg,
    bool menu ) const
{
    switch ( pmProfile ) {
        case PROFILE_SUNKEN:
            drawGradientWithBorder( p, r, horiz );
            break;
        case PROFILE_RAISED: {
                int flags = QStyle::Style_Raised;

                if ( horiz )
                    flags |= Style_Horizontal;

                drawLightBevel( p, r,
                    cg, flags, true,
                    menu ? ROUNDED_ALL : ROUNDED_NONE,
                    getFill( flags, menuPbar ),
                    menuPbar, true
                );
                
                break;
            }
        default:
            p->fillRect( r, menuPbar[ GRADIENT_BASE ] );
            break;
    }
}
void KlearlookStyle::drawPBarOrMenu2(
    QPainter *p,
    QRect const &r,
    bool horiz,
    const QColorGroup &cg,
    bool menu ) const
{
    switch ( pmProfile ) {
        case PROFILE_SUNKEN:
            drawGradientWithBorder( p, r, horiz );
            break;
        case PROFILE_RAISED: {
                int flags = QStyle::Style_Raised;

                if ( horiz )
                    flags |= Style_Horizontal;

                drawLightBevel( p, r,
                    cg, flags, true,
                    menu ? ROUNDED_TOP : ROUNDED_NONE,
                    getFill( flags, menuPbar ),
                    menuPbar, true
                );
                
                break;
            }
        default:
            p->fillRect( r, menuPbar[ GRADIENT_BASE ] );
            break;
    }
}

void KlearlookStyle::drawGradientWithBorder(
    QPainter *p,
    QRect const &r,
    bool horiz ) const
{
    QRect r2( r );

    drawGradient( menuPbar[ GRADIENT_TOP ],
        menuPbar[ GRADIENT_BOTTOM ], true, borderFrame ? 2 : 1, p, r, horiz );
    // 3d border effect...
    if ( borderFrame ) {
        p->setPen( menuPbar[ GRADIENT_BASE ] );
        p->setBrush( NoBrush );
        p->drawRect( r );
    } else
        r2.addCoords( -1, -1, 1, 1 );

    p->setPen( menuPbar[ GRADIENT_LIGHT ] );
    p->drawLine( r2.left() + 1, r2.top() + 1, r2.right() - 1, r2.top() + 1 );
    p->drawLine( r2.left() + 1, r2.top() + 1, r2.left() + 1, r2.bottom() - 1 );
    p->setPen( menuPbar[ GRADIENT_DARK ] );
    p->drawLine( r2.left() + 1, r2.bottom() - 1, r2.right() - 1, r2.bottom() - 1 );
    p->drawLine( r2.right() - 1, r2.bottom() - 1, r2.right() - 1, r2.top() + 1 );
}

void KlearlookStyle::drawBevelGradient( 
    const QColor &base, 
    bool increase, 
    int border, 
    QPainter *p, 
    QRect const &r,
    bool horiz, double shadeTop, double shadeBot ) 
const {
    //CPD TODO: Store last settings to make faster!
    QColor top, bot;

    if ( equal( 1.0, shadeTop ) )
        top = base;
    else
        shade( base, &top, shadeTop );
    if ( equal( 1.0, shadeBot ) )
        bot = base;
    else
        shade( base, &bot, shadeBot );

    drawGradient( top, bot, increase, border, p, r, horiz );
}

void KlearlookStyle::drawGradient( 
	const QColor &top, 
	const QColor &bot, 
	bool increase, 
	int border, 
	QPainter *p, 
	QRect const &r, 
	bool horiz ) const 
{
    if ( r.width() > 0 && r.height() > 0 ) {
        QRect grad(
            r.left() + border,
            r.top() + border,
            r.width() - ( border * 2 ),
            r.height() - ( border * 2 )
        );

        if ( top == bot )
           p->fillRect( grad, top );
        else {
            QRect grad(
                r.left() + border,
                r.top() + border,
                r.width() - ( border * 2 ),
                r.height() - ( border * 2 )
            );
            
            int i,
            s = horiz ? grad.top() : grad.left(),
                e = horiz ? grad.bottom() : grad.right();

            double amt = ( horiz ? grad.height() : grad.width() ) ,
                   dr = ( ( double ) ( bot.red() - top.red() ) ) / amt ,
                   dg = ( ( double ) ( bot.green() - top.green() ) ) / amt ,
                   db = ( ( double ) ( bot.blue() - top.blue() ) ) / amt,
                   rc = 0, gc = 0, bc = 0;

            if ( increase )
                for ( i = s; i <= e ; i++ ) {
                    p->setPen( QColor(
                            limit( top.red() + rc ),
                            limit( top.green() + gc ),
                            limit( top.blue() + bc )
                        ));
                      
                    if ( horiz )
                        p->drawLine( grad.left(), i, grad.right(), i );
                    else
                        p->drawLine( i, grad.top(), i, grad.bottom() );
                    rc += dr;
                    gc += dg;
                    bc += db;
                }
            else
                for ( i = e; i >= s; i-- ) {
                    p->setPen(
                        QColor(
                            limit( top.red() + rc ),
                            limit( top.green() + gc ),
                            limit( top.blue() + bc )
                         ) );
                         
                    if ( horiz )
                        p->drawLine( grad.left(), i, grad.right(), i );
                        
                    else
                        p->drawLine( i, grad.top(), i, grad.bottom() );
                        
                    rc += dr;
                    gc += dg;
                    bc += db;
                }
        }
    }
}


void KlearlookStyle::drawPopupRect( QPainter *p, const QRect &r, const QColorGroup &cg ) const
{
    const QColor *use = backgroundColors( cg );
    p->setPen( use[ 4 ].light(70) );
    p->setBrush( NoBrush );
    p->drawRect( r );
}

void KlearlookStyle::drawSliderHandle(
    QPainter *p,
    const QRect &r,
    const QColorGroup &cg, QStyle::SFlags flags
) const
{
    const QColor * use = buttonColors( cg );

    if ( r.width() > r.height() )
        flags |= Style_Horizontal;
    flags |= Style_Raised;

    drawLightBevelButton( p, r, cg, flags, true, ROUNDED_ALL, getFill( flags, use ), use );

    if ( GROOVE_NONE != sliderThumbs &&
       ( ( flags & Style_Horizontal && r.width() >= 14 ) || r.height() >= 14 ) )
        drawLines( p, r,
            r.width() < r.height(),
            4, 3, use, 0,
            GROOVE_SUNKEN == sliderThumbs,
            APPEARANCE_LIGHT_GRADIENT == appearance );
}

void KlearlookStyle::drawSliderGroove
    ( QPainter *p,
      const QRect &r,
      QStyle::SFlags flags,
      const QWidget *widget ) const
{
    const QSlider * slider = ( const QSlider * ) widget;
    QRect groove( r );

    if ( flags & Style_HasFocus ) {
        QRect fr( groove );

        fr.addCoords( -1, -1, 1, 1 );
        drawPrimitive( PE_FocusRect, p, fr, QColorGroup() );
    }

    if ( Qt::Horizontal == slider->orientation() ) {
        int dh = ( groove.height() - 5 ) >> 1;

        groove.addCoords( 0, dh, 0, -dh );
    } else {
        int dw = ( groove.width() - 5 ) >> 1;

        groove.addCoords( dw, 0, -dw, 0 );
    }
    p->setBrush( gray[ 2 ] );
    p->setPen( gray[ 5 ] );
    p->drawRect( groove );
    p->setPen( gray[ 4 ] );
    p->drawLine( groove.x() + 1, groove.y() + 1, groove.x() + groove.width() - 2, groove.y() + 1 );
    p->drawLine( groove.x() + 1, groove.y() + 1, groove.x() + 1, groove.y() + groove.height() - 2 );
}

void KlearlookStyle::shadeColors( const QColor &base, QColor *vals ) const {
    QTC_SHADES

    int i;

    for ( i = 0; i < NUM_SHADES; ++i )
        shade( base, &vals[ i ], QTC_SHADE( appearance, contrast, i ) );
        
    vals[ NUM_SHADES ] = base;
}

const QColor * KlearlookStyle::buttonColors( const QColorGroup &cg ) const {
    if ( cg.button() != button[ NUM_SHADES ] ) {
        shadeColors( cg.button(), buttonColoured );
        return buttonColoured;
    }

    return button;
}

const QColor * KlearlookStyle::backgroundColors( const QColorGroup &cg ) const {
    if ( cg.background() != gray[ NUM_SHADES ] ) {
        shadeColors( cg.background(), backgroundColoured );
        return backgroundColoured;
    }

    return gray;
}

bool KlearlookStyle::redrawHoverWidget() {
    if ( !hoverWidget )
        return false;

    QPoint cursor( QCursor::pos() ),
    widgetZero( hoverWidget->mapToGlobal( QPoint( 0, 0 ) ) );

#if QT_VERSION >= 0x030200

    //
    // Qt>=3.2 sets the sensitive part of a check/radio to the image + label -> anything else
    // is not sensitive. But,
    // the widget can ocupy a larger area - and this whole are will react to mouse over.
    // This needs to be coounteracted
    // so that it looks as if only the sensitive area mouse-overs...
    QRadioButton *rb = dynamic_cast<QRadioButton *>( hoverWidget );

    if ( rb ) {
        QRect rect( widgetZero.x(), widgetZero.y(),
            visualRect( subRect( SR_RadioButtonFocusRect, rb ), rb ).width() +
            pixelMetric( PM_ExclusiveIndicatorWidth ) + 4, hoverWidget->height() );

        hover = rect.contains( cursor ) ? HOVER_RADIO : HOVER_NONE;
        return ( HOVER_NONE != hover && !rect.contains( oldCursor ) ) ||
               ( HOVER_NONE == hover && rect.contains( oldCursor ) );
    } else {
        QCheckBox *cb = dynamic_cast<QCheckBox *>( hoverWidget );

        if ( cb ) {
            QRect rect( widgetZero.x(), widgetZero.y(),
                visualRect( subRect( SR_CheckBoxFocusRect, cb ), cb ).width() +
                        pixelMetric( PM_IndicatorWidth ) + 4, hoverWidget->height() );

            hover = rect.contains( cursor ) ? HOVER_CHECK : HOVER_NONE;
            return ( HOVER_NONE != hover && !rect.contains( oldCursor ) ) ||
                   ( HOVER_NONE == hover && rect.contains( oldCursor ) );
        } else {
#endif
            QScrollBar *sb = dynamic_cast<QScrollBar *>( hoverWidget );

            if ( sb )    // So, are we over add button, sub button, slider, or none?
            {
                QRect subline( querySubControlMetrics( CC_ScrollBar, hoverWidget, SC_ScrollBarSubLine ) ),
                addline( querySubControlMetrics( CC_ScrollBar, hoverWidget, SC_ScrollBarAddLine ) ),
                slider( querySubControlMetrics( CC_ScrollBar, hoverWidget, SC_ScrollBarSlider ) );

                subline.moveLeft( subline.x() + widgetZero.x() );
                subline.moveTop( subline.y() + widgetZero.y() );
                addline.moveLeft( addline.x() + widgetZero.x() );
                addline.moveTop( addline.y() + widgetZero.y() );
                slider.moveLeft( slider.x() + widgetZero.x() );
                slider.moveTop( slider.y() + widgetZero.y() );

                if ( slider.contains( cursor ) )
                    hover = HOVER_SB_SLIDER;
                else if ( subline.contains( cursor ) )
                    hover = HOVER_SB_SUB;
                else if ( addline.contains( cursor ) )
                    hover = HOVER_SB_ADD;
                else
                    hover = HOVER_NONE;

                return ( HOVER_SB_SLIDER == hover && !slider.contains( oldCursor ) ) ||
                       ( HOVER_SB_SLIDER != hover && slider.contains( oldCursor ) ) ||
                       ( HOVER_SB_SUB == hover && !subline.contains( oldCursor ) ) ||
                       ( HOVER_SB_SUB != hover && subline.contains( oldCursor ) ) ||
                       ( HOVER_SB_ADD == hover && !addline.contains( oldCursor ) ) ||
                       ( HOVER_SB_ADD != hover && addline.contains( oldCursor ) );
            } else {
#if KDE_VERSION >= 0x30400
                QToolButton *tb = dynamic_cast<QToolButton *>( hoverWidget );

                if ( tb ) {
                    hover = APP_KICKER == themedApp ? HOVER_KICKER : HOVER_NONE;
                    return HOVER_KICKER == hover;
                } else {
#endif
                    QHeader *hd = dynamic_cast<QHeader *>( hoverWidget );

                    if ( hd ) {
                        // Hmm... this ones tricky, as there's only 1 widget - but it has different sections...
                        // and the ones that aren't clickable should not highlight on mouse over!

                        QRect rect(
                            widgetZero.x(),
                            widgetZero.y(),
                            hoverWidget->width(),
                            hoverWidget->height()
                        );
                        
                        int s = 0;
                        bool redraw = false;

                        hover = rect.contains( cursor ) ? HOVER_HEADER : HOVER_NONE;
                        hoverSect = QTC_NO_SECT;

                        for ( s = 0; s < hd->count() && ( QTC_NO_SECT == hoverSect || !redraw ); ++s ) {
                            QRect r( hd->sectionRect( s ) );

                            r.moveLeft( r.x() + widgetZero.x() );
                            r.moveTop( r.y() + widgetZero.y() );

                            bool hasNew = r.contains( cursor );

                            if ( hasNew )
                                hoverSect = s;

                            if ( !redraw ) {
                                bool hasOld = r.contains( oldCursor );

                                if ( ( hasNew && !hasOld ) || ( !hasNew && hasOld ) )
                                    redraw = true;
                            }
                        }
                        return redraw;
                    } else
                        return oldCursor == QPoint( -1, -1 );
#if KDE_VERSION >= 0x30400

                }
#endif

            }
#if QT_VERSION >= 0x030200

        }
    }
#endif

    return false;
}

#define gdouble double

EDefBtnIndicator qtc_to_ind( const char *str ) {
    if ( 0 == memcmp( str, "fontcolour", 10 ) )
        return IND_FONT_COLOUR;
    if ( 0 == memcmp( str, "border", 6 ) )
        return IND_BORDER;
    if ( 0 == memcmp( str, "none", 4 ) )
        return IND_NONE;
    return IND_CORNER;
}

EGroove qtc_to_groove( const char *str ) {
    if ( 0 == memcmp( str, "raised", 6 ) )
        return GROOVE_RAISED;
    if ( 0 == memcmp( str, "none", 4 ) )
        return GROOVE_NONE;
    return GROOVE_SUNKEN;
}

ETBarBorder qtc_to_tbar_border( const char *str ) {
    if ( 0 == memcmp( str, "dark", 4 ) )
        return TB_DARK;
    if ( 0 == memcmp( str, "none", 4 ) )
        return TB_NONE;
    if ( 0 == memcmp( str, "light", 5 ) )
        return TB_LIGHT;
    return TB_LIGHT;
}

ELvExpander qtc_to_lv_expander( const char *str ) {
    return 0 == memcmp( str, "arrow", 5 ) ? LV_EXP_ARR : LV_EXP_PM;
}

ELvLines qtc_to_lv_lines( const char *str ) {
    if ( 0 == memcmp( str, "none", 4 ) )
        return LV_LINES_NONE;
    if ( 0 == memcmp( str, "dotted", 6 ) )
        return LV_LINES_DOTTED;
    return LV_LINES_SOLID;
}

EProfile qtc_to_profile( const char *str ) {
    if ( 0 == memcmp( str, "flat", 4 ) )
        return PROFILE_FLAT;
    if ( 0 == memcmp( str, "raised", 6 ) )
        return PROFILE_RAISED;
    return PROFILE_SUNKEN;
}

EAppearance qtc_to_appearance( const char *str ) {
    if ( 0 == memcmp( str, "flat", 4 ) )
        return APPEARANCE_FLAT;
    if ( 0 == memcmp( str, "gradient", 8 ) )
        return APPEARANCE_GRADIENT;
    return APPEARANCE_LIGHT_GRADIENT;
}


#include "klearlook.moc"
