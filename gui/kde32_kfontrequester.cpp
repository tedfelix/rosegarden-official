/*
    Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "kde32_kfontrequester.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <kfontdialog.h>
#include <klocale.h>

namespace KDE32Backport
{


KFontRequester::KFontRequester( QWidget *parent, const char *name,
    bool onlyFixed ) : QWidget( parent, name ),
    m_onlyFixed( onlyFixed )
{
  QHBoxLayout *layout = new QHBoxLayout( this, 0, KDialog::spacingHint() );

  m_sampleLabel = new QLabel( this, "m_sampleLabel" );
  m_button = new QPushButton( i18n( "Choose..." ), this, "m_button" );

  m_sampleLabel->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

  layout->addWidget( m_sampleLabel, 1 );
  layout->addWidget( m_button );

  connect( m_button, SIGNAL( clicked() ), SLOT( buttonClicked() ) );

  displaySampleText();
  setToolTip();
}

void KFontRequester::setFont( const QFont &font, bool onlyFixed )
{
  m_selFont = font;
  m_onlyFixed = onlyFixed;

  displaySampleText();
  emit fontSelected( m_selFont );
}

void KFontRequester::setSampleText( const QString &text )
{
  m_sampleText = text;
  displaySampleText();
}

void KFontRequester::setTitle( const QString &title )
{
  m_title = title;
  setToolTip();
}

void KFontRequester::buttonClicked()
{
  int result = KFontDialog::getFont( m_selFont, m_onlyFixed, parentWidget() );

  if ( result == KDialog::Accepted )
  {
    displaySampleText();
    emit fontSelected( m_selFont );
  }
}

void KFontRequester::displaySampleText()
{
  m_sampleLabel->setFont( m_selFont );

  int size = m_selFont.pointSize();
  if(size == -1)
    size = m_selFont.pixelSize();

  if ( m_sampleText.isEmpty() )
    m_sampleLabel->setText( QString( "%1 %2" ).arg( m_selFont.family() )
      .arg( size ) );
  else
    m_sampleLabel->setText( m_sampleText );
}

void KFontRequester::setToolTip()
{
  QToolTip::remove( m_button );
  QToolTip::add( m_button, i18n( "Click to select a font" ) );

  QToolTip::remove( m_sampleLabel );
  QWhatsThis::remove( m_sampleLabel );

  if ( m_title.isNull() )
  {
    QToolTip::add( m_sampleLabel, i18n( "Preview of the selected font" ) );
    QWhatsThis::add( m_sampleLabel, 
        i18n( "This is a preview of the selected font. You can change it"
        " by clicking the \"Choose...\" button." ) );
  }
  else
  {
    QToolTip::add( m_sampleLabel, 
        i18n( "Preview of the \"%1\" font" ).arg( m_title ) );
    QWhatsThis::add( m_sampleLabel, 
        i18n( "This is a preview of the \"%1\" font. You can change it"
        " by clicking the \"Choose...\" button." ).arg( m_title ) );
  }
}
}

// #include "kfontrequester.moc"

/* vim: et sw=2 ts=2
*/
