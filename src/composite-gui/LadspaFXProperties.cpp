/*
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * This file is part of Composite
 *
 * Composite is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Composite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <Tritium/Engine.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/fx/Effects.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/IO/AudioOutput.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

#include "LadspaFXProperties.hpp"
#include "CompositeApp.hpp"
#include "LadspaFXSelector.hpp"
#include "Skin.hpp"
#include "widgets/Fader.hpp"
#include "widgets/LCD.hpp"

#include "Mixer/Mixer.hpp"
#include "Mixer/MixerLine.hpp"

using namespace std;
using namespace Tritium;

LadspaFXProperties::LadspaFXProperties(QWidget* parent, uint nLadspaFX)
 : QWidget( parent )
{
//	DEBUGLOG( "INIT" );

	m_nLadspaFX = nLadspaFX;

	resize( 500, 200 );
	setMinimumSize( width(), height() );
	setFixedHeight( height() );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );
	setLayout( hbox );


	// Background image
	QPixmap background;
	bool ok = background.load( Skin::getImagePath() + "/mixerPanel/mixer_background.png" );
	if( !ok ){
		ERRORLOG( "Error loading pixmap" );
	}


	m_pScrollArea = new QScrollArea( NULL );
	hbox->addWidget( m_pScrollArea );

	m_pScrollArea->move( 0, 0 );
	m_pScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pScrollArea->resize( width(), height() );

	m_pFrame = new QFrame( this );
	m_pFrame->resize( width(), height() );

	m_pScrollArea->setWidget( m_pFrame );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->move( 10, 10 );
	m_pNameLbl->resize( 270, 24 );

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );

	m_pSelectFXBtn = new QPushButton( trUtf8("Select FX"), this);
	m_pSelectFXBtn->move( 280, 10 );
	m_pSelectFXBtn->resize( 100, 24 );
	connect( m_pSelectFXBtn, SIGNAL(clicked()), this, SLOT(selectFXBtnClicked()) );


	m_pActivateBtn = new QPushButton( trUtf8("Activate"), this);
	m_pActivateBtn->move( 390, 10 );
	m_pActivateBtn->resize( 100, 24 );
	connect( m_pActivateBtn, SIGNAL(clicked()), this, SLOT(activateBtnClicked()) );


	m_pTimer = new QTimer( this );
	connect(m_pTimer, SIGNAL( timeout() ), this, SLOT( updateOutputControls() ) );
}


LadspaFXProperties::~LadspaFXProperties()
{
//	DEBUGLOG( "DESTROY" );
}



void LadspaFXProperties::showEvent ( QShowEvent* )
{
	updateControls();
}



void LadspaFXProperties::closeEvent( QCloseEvent *ev )
{
	ev->accept();
}


void LadspaFXProperties::faderChanged( Fader * ref )
{
	ref->setPeak_L( ref->getValue() );
	ref->setPeak_R( ref->getValue() );

	T<Song>::shared_ptr pSong = (g_engine )->getSong();

#ifdef LADSPA_SUPPORT
	T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX( m_nLadspaFX );

	for ( uint i = 0; i < m_pInputControlFaders.size(); i++ ) {
		if (ref == m_pInputControlFaders[ i ] ) {
			LadspaControlPort *pControl = pFX->inputControlPorts[ i ];

			pControl->fControlValue = ref->getValue();
			//float fInterval = pControl->fUpperBound - pControl->fLowerBound;
			//pControl->fControlValue = pControl->fLowerBound + fValue * fInterval;

			QString sValue;
			if (pControl->fControlValue < 1.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 2 );
			}
			else if ( pControl->fControlValue < 100.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 1 );
			}
			else {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 0 );
			}
			m_pInputControlLabel[ i ]->setText( sValue );
		}
	}
	pSong->set_modified(true);
#endif
}



void LadspaFXProperties::updateControls()
{
#ifdef LADSPA_SUPPORT
	DEBUGLOG( "*** [updateControls] ***" );
	m_pTimer->stop();

	T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX( m_nLadspaFX );

	// svuoto i vettori..
	if ( m_pInputControlNames.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlNames.size(); i++) {
			delete m_pInputControlNames[ i ];
		}
		m_pInputControlNames.clear();
	}
	if ( m_pInputControlLabel.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlLabel.size(); i++) {
			delete m_pInputControlLabel[ i ];
		}
		m_pInputControlLabel.clear();
	}
	if ( m_pInputControlFaders.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlFaders.size(); i++) {
			delete m_pInputControlFaders[ i ];
		}
		m_pInputControlFaders.clear();
	}

	if ( m_pOutputControlFaders.size() != 0 ) {
		for (uint i = 0; i < m_pOutputControlFaders.size(); i++) {
			delete m_pOutputControlFaders[ i ];
		}
		m_pOutputControlFaders.clear();
	}
	if ( m_pOutputControlNames.size() != 0 ) {
		for (uint i = 0; i < m_pOutputControlNames.size(); i++) {
			delete m_pOutputControlNames[ i ];
		}
		m_pOutputControlNames.clear();
	}

	if (pFX) {
		QString sPluginName = pFX->getPluginLabel();
		setWindowTitle( trUtf8( "[%1] LADSPA FX Properties" ).arg( sPluginName ) );

		int nControlsFrameWidth = 10 + 45 * (pFX->inputControlPorts.size() + pFX->outputControlPorts.size()) + 10 + 45;
		if ( nControlsFrameWidth < width() ) {
			nControlsFrameWidth = width();
		}
		m_pFrame->resize( nControlsFrameWidth, height() );

		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( trUtf8("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( trUtf8("Activate") );
		}

		QString mixerline_text_path = Skin::getImagePath() + "/mixerPanel/mixer_background.png";
		QPixmap textBackground;
		if( textBackground.load( mixerline_text_path ) == false ){
			ERRORLOG( "Error loading pixmap"  );
		}

		// input controls
		uint nInputControl_X = 0;
		for (uint i = 0; i < pFX->inputControlPorts.size(); i++) {
			LadspaControlPort *pControlPort = pFX->inputControlPorts[ i ];

			nInputControl_X = 10 + 45 * i;

			if (pControlPort->isToggle){	// toggle button
				WARNINGLOG( "[updateControls] LADSPA toggle controls not implemented yet");
			}

			// peak volume label
			QString sValue;
			if (pControlPort->fControlValue < 1.0 ) {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 2);
			}
			else if ( pControlPort->fControlValue < 100.0 ) {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 1);
			}
			else {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 0);
			}

			LCDDisplay *pLCD = new LCDDisplay( m_pFrame, LCDDigit::SMALL_BLUE, 4 );
			pLCD->move( nInputControl_X, 40 );
			pLCD->setText( sValue );
			pLCD->show();
			QPalette lcdPalette;
			lcdPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
			pLCD->setPalette( lcdPalette );

			m_pInputControlLabel.push_back( pLCD );

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( nInputControl_X, 60 );
			pName->show();
			pName->setText( pControlPort->sName );
			m_pInputControlNames.push_back( pName );
			pName->setToolTip( pName->text() );


			// fader
			Fader *pFader = new Fader( m_pFrame, pControlPort->m_bIsInteger, false );
			connect( pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );
			m_pInputControlFaders.push_back( pFader );
			pFader->move( nInputControl_X + 20, 60 );
			pFader->show();
			pFader->setMaxValue( pControlPort->fUpperBound );
			pFader->setMinValue( pControlPort->fLowerBound );
			pFader->setMaxPeak( pControlPort->fUpperBound );
			pFader->setMinPeak( pControlPort->fLowerBound );
			pFader->setValue( pControlPort->fControlValue );
			pFader->setPeak_L( pControlPort->fControlValue );
			pFader->setPeak_R( pControlPort->fControlValue );

			//float fInterval = pControlPort->fUpperBound - pControlPort->fLowerBound;
			//float fValue = ( pControlPort->fControlValue - pControlPort->fLowerBound ) / fInterval;
			//pFader->setValue( fValue );
			//pFader->setPeak_L( fValue );
			//pFader->setPeak_R( fValue );

			faderChanged( pFader );

			m_pNameLbl->setText( pFX->getPluginName() );
		}

		nInputControl_X += 45;
		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[ i ];

			uint xPos = nInputControl_X + 10 + 45 * i;

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( xPos, 60 );
			pName->show();
			pName->setText( pControl->sName );
			m_pInputControlNames.push_back( pName );
			pName->setToolTip( pName->text() );

			// fader
			Fader *pFader = new Fader( m_pFrame, true, true );	// without knob!
			pFader->move( xPos + 20, 60 );
			//float fInterval = pControl->fUpperBound - pControl->fLowerBound;
			//float fValue = pControl->fControlValue / fInterval;
			pFader->show();
			pFader->setMaxValue( pControl->fUpperBound );
			pFader->setMinValue( pControl->fLowerBound );
			pFader->setMaxPeak( pControl->fUpperBound );
			pFader->setMinPeak( pControl->fLowerBound );
			pFader->setValue( pControl->fControlValue );
			pFader->setPeak_L( pControl->fControlValue );
			pFader->setPeak_R( pControl->fControlValue );

			m_pOutputControlFaders.push_back( pFader );
		}
	}
	else {
		DEBUGLOG( "NULL PLUGIN" );
		setWindowTitle( trUtf8( "LADSPA FX %1 Properties" ).arg( m_nLadspaFX) );
		m_pNameLbl->setText( trUtf8("No plugin") );
		m_pActivateBtn->setEnabled(false);
	}

	m_pTimer->start(100);
#endif
}



void LadspaFXProperties::selectFXBtnClicked()
{
#ifdef LADSPA_SUPPORT
	LadspaFXSelector fxSelector(m_nLadspaFX);
	if (fxSelector.exec() == QDialog::Accepted) {
		QString sSelectedFX = fxSelector.getSelectedFX();
		if ( !sSelectedFX.isEmpty() ) {
			T<LadspaFX>::shared_ptr pFX;

			vector<Tritium::LadspaFXInfo*> pluginList = g_engine->get_effects()->getPluginList();
			for (uint i = 0; i < pluginList.size(); i++) {
				Tritium::LadspaFXInfo *pFXInfo = pluginList[i];
				if (pFXInfo->m_sName == sSelectedFX ) {
					int nSampleRate = g_engine->get_audio_output()->getSampleRate();
					pFX = LadspaFX::load( pFXInfo->m_sFilename, pFXInfo->m_sLabel, nSampleRate );
					pFX->setEnabled( true );
					break;
				}
			}
			//g_engine->lock( RIGHT_HERE );
			T<Song>::shared_ptr pSong = (g_engine )->getSong();
			pSong->set_modified(true);

			g_engine->get_effects()->setLadspaFX( pFX, m_nLadspaFX );

			//g_engine->unlock();
			g_engine->restartLadspaFX();
			updateControls();
		}
		else {	// no plugin selected
			DEBUGLOG( "no plugin selected" );
		}
	}
#endif
}



void LadspaFXProperties::updateOutputControls()
{
#ifdef LADSPA_SUPPORT

//	DEBUGLOG( "[updateOutputControls]" );
//	Song *pSong = (g_engine )->getSong();
	T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX(m_nLadspaFX);

	if (pFX) {
		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( trUtf8("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( trUtf8("Activate") );
		}

		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[i];

			vector<Fader*>::iterator it = m_pOutputControlFaders.begin() + i;
			if (it != m_pOutputControlFaders.end() ) {
				Fader *pFader = *it;
				if (pFader == NULL) {
					ERRORLOG( "[updateOutputControls] pFader = NULL" );
					continue;
				}

				float fValue = pControl->fControlValue;
				float fInterval = pControl->fUpperBound - pControl->fLowerBound;
				fValue = pControl->fControlValue / fInterval;

				if (fValue < 0) fValue = -fValue;

				pFader->setPeak_L( fValue );
				pFader->setPeak_R( fValue );
			}
		}
	}
	else {
		m_pActivateBtn->setEnabled(false);
	}
#endif
}




void LadspaFXProperties::activateBtnClicked()
{
#ifdef LADSPA_SUPPORT
//	Song *pSong = (g_engine )->getSong();
	T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX(m_nLadspaFX);
	if (pFX) {
		g_engine->lock( RIGHT_HERE );
		pFX->setEnabled( !pFX->isEnabled() );
		g_engine->unlock();
	}
#endif
}
