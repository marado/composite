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

#include <stdio.h>

#include <QPainter>

#include "../InstrumentEditor/InstrumentEditor.hpp"
#include "../widgets/Fader.hpp"
#include "../CompositeApp.hpp"
#include "../Skin.hpp"
#include "../widgets/Rotary.hpp"
#include "../widgets/Button.hpp"
#include "../widgets/LCD.hpp"

#include <Tritium/Engine.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

using namespace Tritium;

#include "MixerLine.hpp"

#define MIXERLINE_WIDTH			56
#define MIXERLINE_HEIGHT		254
#define MASTERMIXERLINE_WIDTH	126
#define MASTERMIXERLINE_HEIGHT	284
#define MIXERLINE_LABEL_H		115
#define MASTERMIXERLINE_FADER_H	75

using namespace Tritium;

MixerLine::MixerLine(QWidget* parent)
 : PixmapWidget( parent, "MixerLine" )
{
//	DEBUGLOG( "INIT" );

	m_nWidth = MIXERLINE_WIDTH;
	m_nHeight = MIXERLINE_HEIGHT;
	m_fMaxPeak = 0.0;
	m_nActivity = 0;
	m_bIsSelected = false;
	m_nPeakTimer = 0;

	resize( m_nWidth, m_nHeight );
	setFixedSize( m_nWidth, m_nHeight );

	setPixmap( "/mixerPanel/mixerline_background.png" );

	// Play sample button
	m_pPlaySampleBtn = new Button(
			this,
			"/mixerPanel/btn_play_on.png",
			"/mixerPanel/btn_play_off.png",
			"/mixerPanel/btn_play_over.png",
			QSize( 18, 13 )
	);
	m_pPlaySampleBtn->move( 8, 2 );
	m_pPlaySampleBtn->setToolTip( trUtf8( "Play sample" ) );
	connect(m_pPlaySampleBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));
	connect(m_pPlaySampleBtn, SIGNAL(rightClicked(Button*)), this, SLOT(rightClick(Button*)));

	// Trigger sample LED
	m_pTriggerSampleLED = new Button(
			this,
			"/mixerPanel/led_trigger_on.png",
			"/mixerPanel/led_trigger_off.png",
			"/mixerPanel/led_trigger_off.png",
			QSize( 5, 13 )
	);
	m_pTriggerSampleLED->move( 26, 2 );
	connect(m_pTriggerSampleLED, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// Mute button
	m_pMuteBtn = new ToggleButton(
			this,
			"/mixerPanel/btn_mute_on.png",
			"/mixerPanel/btn_mute_off.png",
			"/mixerPanel/btn_mute_over.png",
			QSize( 18, 13 )
	);
	m_pMuteBtn->move( 8, 17 );
	m_pMuteBtn->setToolTip( trUtf8( "Mute" ) );
	connect(m_pMuteBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// Solo button
	m_pSoloBtn = new ToggleButton(
			this,
			"/mixerPanel/btn_solo_on.png",
			"/mixerPanel/btn_solo_off.png",
			"/mixerPanel/btn_solo_over.png",
			QSize( 18, 13 )
	);
	m_pSoloBtn->move( 30, 17);
	m_pSoloBtn->setToolTip( trUtf8( "Solo" ) );
	connect(m_pSoloBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// pan rotary
	m_pPanRotary = new Rotary( this, Rotary::TYPE_CENTER, trUtf8( "Pan" ), false, true);
	m_pPanRotary->move( 14, 32 );
	connect( m_pPanRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( panChanged(Rotary*) ) );

	// FX send
	uint y = 0;
	for (uint i = 0; i < MAX_FX; i++) {
		m_pKnob[i] = new Knob(this);
		if ( (i % 2) == 0 ) {
			m_pKnob[i]->move( 9, 63 + (20 * y) );
		}
		else {
			m_pKnob[i]->move( 30, 63 + (20 * y) );
			y++;
		}
		connect( m_pKnob[i], SIGNAL( valueChanged(Knob*) ), this, SLOT( knobChanged(Knob*) ) );
	}

	T<Preferences>::shared_ptr pref = g_engine->get_preferences();

	QString family = pref->getMixerFontFamily();
	int size = pref->getMixerFontPointSize();
	QFont mixerFont( family, size );
	float m_fFalloffTemp = pref->getMixerFalloffSpeed();
	m_fFalloffTemp = (m_fFalloffTemp * 20) - 2;
	m_nFalloff = (int)m_fFalloffTemp;

	QPixmap textBackground;
	bool ok = textBackground.load( Skin::getImagePath() + "/mixerPanel/mixerline_text_background.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	m_pNameWidget->setToolTip( trUtf8( "Instrument name (double click to edit)" ) );
	connect( m_pNameWidget, SIGNAL( doubleClicked () ), this, SLOT( nameClicked() ) );
	connect( m_pNameWidget, SIGNAL( clicked () ), this, SLOT( nameSelected() ) );

	// m_pFader
	m_pFader = new Fader( this, false, false );
	m_pFader->move( 23, 128 );
	m_pFader->setMinValue( 0.0 );
	m_pFader->setMaxValue( 1.5 );
	connect( m_pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );


	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
	m_pPeakLCD->move( 10, 106 );
	m_pPeakLCD->setText( "0.00" );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Background, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );
}



MixerLine::~MixerLine()
{
//	DEBUGLOG( "DESTROY" );
	//delete m_pFader;
}



void MixerLine::updateMixerLine()
{
	if ( m_nPeakTimer > m_nFalloff ) {
		if ( m_fMaxPeak > 0.05f ) {
			m_fMaxPeak = m_fMaxPeak - 0.05f;
		}
		else {
			m_fMaxPeak = 0.0f;
			m_nPeakTimer = 0;
		}
		m_pPeakLCD->setText( QString("%1").arg(m_fMaxPeak, 0, 'f', 2) );
		if ( m_fMaxPeak > 1.0 ) {
			m_pPeakLCD->setSmallRed();
		}
		else {
			m_pPeakLCD->setSmallBlue();
		}
	}
	m_nPeakTimer++;
}



void MixerLine::click(Button *ref) {
	T<Song>::shared_ptr song = (g_engine)->getSong();

	if (ref == m_pMuteBtn) {
		song->set_modified( true );
		emit muteBtnClicked(this);
	}
	else if (ref == m_pSoloBtn) {
		song->set_modified( true );
		emit soloBtnClicked(this);
	}
	else if (ref == m_pPlaySampleBtn) {
		emit noteOnClicked(this);
	}
}



void MixerLine::rightClick(Button *ref)
{
	if (ref == m_pPlaySampleBtn) {
		emit noteOffClicked(this);
	}

}



void MixerLine::faderChanged(Fader *ref)
{
	T<Song>::shared_ptr song = (g_engine)->getSong();
	song->set_modified( true );
	emit volumeChanged(this);

	float value = ref->getValue();
	QString msg = QString( trUtf8("Set instrument volume [%1]") )
	    .arg(value, 0, 'f', 2);
	( CompositeApp::get_instance() )->setStatusBarMessage( msg, 2000 );
}



bool MixerLine::isMuteClicked() {
	return m_pMuteBtn->isPressed();
}



void MixerLine::setMuteClicked(bool isClicked) {
	m_pMuteBtn->setPressed(isClicked);
}



bool MixerLine::isSoloClicked() {
	return m_pSoloBtn->isPressed();
}



void MixerLine::setSoloClicked(bool isClicked) {
	m_pSoloBtn->setPressed(isClicked);
}



float MixerLine::getVolume()
{
	return m_pFader->getValue();
}



void MixerLine::setVolume( float value )
{
	m_pFader->setValue( value );
}



void MixerLine::setPeak_L( float peak ) {
	if (peak != getPeak_L() ) {
		m_pFader->setPeak_L( peak );
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			QString tmp;
			tmp.setNum(peak, 'f', '2');
			m_pPeakLCD->setText( tmp );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}



float MixerLine::getPeak_L() {
	return m_pFader->getPeak_L();
}



void MixerLine::setPeak_R( float peak ) {
	if (peak != getPeak_R() ) {
		m_pFader->setPeak_R( peak );
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString("%1").arg(peak, 0, 'f', 2) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}



float MixerLine::getPeak_R() {
	return m_pFader->getPeak_R();
}





void MixerLine::nameClicked() {
	emit instrumentNameClicked(this);
}



void MixerLine::nameSelected() {
	emit instrumentNameSelected(this);
}





void MixerLine::panChanged(Rotary *ref)
{
	T<Song>::shared_ptr song = g_engine->getSong();
	song->set_modified( true );
	emit panChanged( this );

	float panValue = ref->getValue();
	float pan_L, pan_R;
	if (panValue > 0.5) {
		pan_L = (1.0 - panValue) * 2.0;
		pan_R = 1.0;
	} else {
		pan_L = 1.0;
		pan_R = panValue * 2.0;
	}

	QString tmp = QString( trUtf8("Set instr. pan [%1L, %2R]") )
		.arg(pan_L, 0, 'f', 2)
		.arg(pan_R, 0, 'f', 2);
	CompositeApp::get_instance()->setStatusBarMessage(
		QString(trUtf8("Set instr. pan [%1]")).arg(tmp),
		2000 );

	m_pPanRotary->setToolTip( trUtf8("Pan ") + tmp );
}



float MixerLine::getPan()
{
	return m_pPanRotary->getValue();
}



void MixerLine::setPan(float fValue)
{
	if ( fValue != m_pPanRotary->getValue() ) {
		m_pPanRotary->setValue( fValue );
		float pan_L, pan_R;
		if (fValue > 0.5) {
			pan_L = (1.0 - fValue) * 2.0;
			pan_R = 1.0;
		} else {
			pan_L = 1.0;
			pan_R = fValue * 2.0;
		}
		QString msg = QString( trUtf8("Pan %1L, %2R") )
			.arg(pan_L, 0, 'f', 2)
			.arg(pan_R, 0, 'f', 2);
		m_pPanRotary->setToolTip( msg );
	}
}


void MixerLine::setPlayClicked( bool clicked ) {
	m_pTriggerSampleLED->setPressed( clicked );
}


void MixerLine::knobChanged(Knob* pRef)
{
//	infoLog( "knobChanged" );
	for (uint i = 0; i < MAX_FX; i++) {
		if (m_pKnob[i] == pRef) {
			emit knobChanged( this, i );
			break;
		}
	}
}


void MixerLine::setFXLevel( uint nFX, float fValue )
{
	if (nFX > MAX_FX) {
		ERRORLOG( QString("[setFXLevel] nFX > MAX_FX (nFX=%1)").arg(nFX) );
	}
	m_pKnob[nFX]->setValue( fValue );
}

float MixerLine::getFXLevel(uint nFX)
{
	if (nFX > MAX_FX) {
		ERRORLOG( QString("[setFXLevel] nFX > MAX_FX (nFX=%1)").arg(nFX) );
	}
	return m_pKnob[nFX]->getValue();
}


void MixerLine::setSelected( bool bIsSelected )
{
	if (m_bIsSelected == bIsSelected )	return;

	m_bIsSelected = bIsSelected;
	if (m_bIsSelected) {
		setPixmap( "/mixerPanel/mixerline_background_on.png" );
	}
	else {
		setPixmap( "/mixerPanel/mixerline_background.png" );
	}

}





// ::::::::::::::::::::::::::::




MasterMixerLine::MasterMixerLine(QWidget* parent)
 : PixmapWidget( parent, "MasterMixerLine" )
{
	m_nWidth = MASTERMIXERLINE_WIDTH;
	m_nHeight = MASTERMIXERLINE_HEIGHT;
	m_nPeakTimer = 0;

	setMinimumSize( m_nWidth, m_nHeight );
	setMaximumSize( m_nWidth, m_nHeight );
	resize( m_nWidth, m_nHeight );
	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	// Background image
	setPixmap( "/mixerPanel/masterMixerline_background.png" );

	T<Preferences>::shared_ptr pref = g_engine->get_preferences();
	int size = pref->getMixerFontPointSize();
	QString family = pref->getMixerFontFamily();
	float m_fFalloffTemp = pref->getMixerFalloffSpeed();
	m_fFalloffTemp = (m_fFalloffTemp * 20) - 2;
	m_nFalloff = (int)m_fFalloffTemp;

	m_pMasterFader = new MasterFader( this );
	m_pMasterFader->setMin( 0.0 );
	m_pMasterFader->setMax( 1.5 );
	m_pMasterFader->move( 24, MASTERMIXERLINE_FADER_H );
	connect( m_pMasterFader, SIGNAL( valueChanged(MasterFader*) ), this, SLOT( faderChanged(MasterFader*) ) );

	QFont mixerFont( family, size );

	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
	m_pPeakLCD->move( 23, 53 );
	m_pPeakLCD->setText( "0.00" );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Background, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );

	m_pHumanizeVelocityRotary = new Rotary( this, Rotary::TYPE_NORMAL, trUtf8( "Humanize velocity" ), false, false );
	m_pHumanizeVelocityRotary->move( 74, 88 );
	connect( m_pHumanizeVelocityRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pHumanizeTimeRotary = new Rotary( this, Rotary::TYPE_NORMAL, trUtf8( "Humanize time" ), false, false );
	m_pHumanizeTimeRotary->move( 74, 125 );
	connect( m_pHumanizeTimeRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pSwingRotary = new Rotary( this,  Rotary::TYPE_NORMAL, trUtf8( "Swing" ), false, false );
	m_pSwingRotary->move( 74, 162 );
	connect( m_pSwingRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	// Mute btn
	m_pMuteBtn = new ToggleButton(
			this,
			"/mixerPanel/master_mute_on.png",
			"/mixerPanel/master_mute_off.png",
			"/mixerPanel/master_mute_over.png",
			QSize( 42, 13 )
	);
	m_pMuteBtn->move( 20, 32 );
	connect( m_pMuteBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteClicked(Button*) ) );

}




MasterMixerLine::~MasterMixerLine()
{
//	cout << "MixerLine destroy" << endl;
	m_fMaxPeak = 0.0;
}



void MasterMixerLine::muteClicked(Button* pBtn)
{
	g_engine->getSong()->set_mute( pBtn->isPressed() );
}



void MasterMixerLine::faderChanged(MasterFader *ref)
{
	m_pMasterFader->setValue( ref->getValue() );

	emit volumeChanged(this);

	T<Song>::shared_ptr song = g_engine->getSong();
	song->set_modified( true );

	float value = ref->getValue();
	QString msg = QString( trUtf8("Set master volume [%1]") )
		.arg(value, 0, 'f', 2);
	( CompositeApp::get_instance() )->setStatusBarMessage( msg, 2000 );
}




float MasterMixerLine::getVolume()
{
	return m_pMasterFader->getValue();
}



void MasterMixerLine::setVolume( float value )
{
	m_pMasterFader->setValue( value );
}



void MasterMixerLine::setPeak_L(float peak)
{
	if ( peak != getPeak_L() ) {
		m_pMasterFader->setPeak_L(peak);
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString("%1").arg(peak, 0, 'f', 2) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}




float MasterMixerLine::getPeak_L() {
	return m_pMasterFader->getPeak_L();
}



void MasterMixerLine::setPeak_R(float peak) {
	if ( peak != getPeak_R() ) {
		m_pMasterFader->setPeak_R(peak);
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString("%1").arg(peak, 0, 'f', 2) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}



float MasterMixerLine::getPeak_R() {
	return m_pMasterFader->getPeak_R();
}



void MasterMixerLine::updateMixerLine()
{

	if ( m_nPeakTimer > m_nFalloff ) {
		if ( m_fMaxPeak  > 0.05f ) {
			m_fMaxPeak = m_fMaxPeak - 0.05f;
		}
		else {
			m_fMaxPeak = 0.0f;
			m_nPeakTimer = 0;
		}
		m_pPeakLCD->setText( QString("%1").arg(m_fMaxPeak, 0, 'f', 2) );
		if ( m_fMaxPeak > 1.0 ) {
			m_pPeakLCD->setSmallRed();
		}
		else {
			m_pPeakLCD->setSmallBlue();
		}
	}
	m_nPeakTimer++;

	T<Song>::shared_ptr pSong = g_engine->getSong();
	if ( pSong ) {
		m_pHumanizeTimeRotary->setValue( pSong->get_humanize_time_value() );
		m_pHumanizeVelocityRotary->setValue( pSong->get_humanize_velocity_value() );
		m_pSwingRotary->setValue( pSong->get_swing_factor() );
	}
	else {
		WARNINGLOG( "pSong == NULL ");
	}
}


void MasterMixerLine::rotaryChanged( Rotary *pRef )
{
	QString sMsg;
	float fVal = pRef->getValue();
	QString sVal = QString("%1").arg(fVal, 0, 'f', 2);

	Engine *pEngine = g_engine;
	g_engine->lock( RIGHT_HERE );

	if ( pRef == m_pHumanizeTimeRotary ) {
		pEngine->getSong()->set_humanize_time_value( fVal );
		sMsg = trUtf8( "Set humanize time parameter [%1]").arg( sVal );
	}
	else if ( pRef == m_pHumanizeVelocityRotary ) {
		pEngine->getSong()->set_humanize_velocity_value( fVal );
		sMsg = trUtf8( "Set humanize velocity parameter [%1]").arg( sVal );
	}
	else if ( pRef == m_pSwingRotary ) {
		pEngine->getSong()->set_swing_factor( fVal );
		sMsg = trUtf8( "Set swing factor [%1]").arg( sVal );
	}
	else {
		ERRORLOG( "[knobChanged] Unhandled knob" );
	}

	g_engine->unlock();

	( CompositeApp::get_instance() )->setStatusBarMessage( sMsg, 2000 );
}




/////////////////////////////////////////


FxMixerLine::FxMixerLine(QWidget* parent)
 : PixmapWidget( parent, "FxMixerLine" )
{
	m_nWidth = MIXERLINE_WIDTH;
	m_nHeight = MIXERLINE_HEIGHT;

	setMinimumSize( m_nWidth, m_nHeight );
	setMaximumSize( m_nWidth, m_nHeight );
	resize( m_nWidth, m_nHeight );
//	QPalette defaultPalette;
//	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
//	this->setPalette( defaultPalette );
	m_fMaxPeak = 0.0;

	// MixerLine Background image
	setPixmap( "/mixerPanel/mixerline_background.png" );

	// MixerLine LABEL Background image
//	QPixmap mixerLineLabelBackground;
//	ok = mixerLineLabelBackground.load(Skin::getImagePath() + "/mixerPanel/mixerline_label_background.png");
//	if( ok == false ){
//		ERRORLOG( "Error loading pixmap" );
//	}

//	QPixmap textBackground;
//	ok = textBackground.load( Skin::getImagePath() + "/mixerPanel/mixerline_text_background.png" );
//	if( ok == false ){
//		ERRORLOG( "Error loading pixmap" );
//	}


	// active button
	activeBtn = new ToggleButton(
			this,
			"/mixerPanel/btn_on_on.png",
			"/mixerPanel/btn_on_off.png",
			"/mixerPanel/btn_on_over.png",
			QSize( 18, 12 )
	);
	activeBtn->move( 2, 5 );
	activeBtn->setToolTip( trUtf8( "FX on/off") );
	connect( activeBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	T<Preferences>::shared_ptr pref = g_engine->get_preferences();

	// m_pFader
	m_pFader = new Fader( this, false, false );
	m_pFader->move( 22, 106 );
	connect( m_pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );

	QString family = pref->getMixerFontFamily();
	int size = pref->getMixerFontPointSize();
	QFont mixerFont( family, size );


	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 2, 106 );
	m_pNameWidget->setText( trUtf8( "Master output" ) );

	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
	m_pPeakLCD->move( 2, MIXERLINE_LABEL_H );
	m_pPeakLCD->setText( "0.00" );
}



FxMixerLine::~FxMixerLine()
{
	delete m_pFader;
}



void FxMixerLine::click(Button *ref) {
	T<Song>::shared_ptr song = g_engine->getSong();

	if (ref == activeBtn ) {
		song->set_modified( true );
		emit activeBtnClicked( this );
	}
}



void FxMixerLine::faderChanged(Fader * /*ref*/)
{
	m_fMaxPeak = 0.0;
	m_pPeakLCD->setText( QString("%1").arg(m_fMaxPeak, 0, 'f', 2) );
	if ( m_fMaxPeak > 1.0 ) {
		m_pPeakLCD->setSmallRed();
	}
	else {
		m_pPeakLCD->setSmallBlue();
	}


	T<Song>::shared_ptr song = g_engine->getSong();
	song->set_modified( true );
	emit volumeChanged( this );

}



float FxMixerLine::getVolume()
{
	return m_pFader->getValue();
}



void FxMixerLine::setVolume( float value )
{
	m_pFader->setValue( value );
}



void FxMixerLine::setPeak_L( float peak )
{
	if (peak != getPeak_L() ) {
		m_pFader->setPeak_L( peak );
		if (peak > m_fMaxPeak) {
			m_pPeakLCD->setText( QString("%1").arg(peak, 0, 'f', 2) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}

			m_fMaxPeak = peak;
		}
	}
}



float FxMixerLine::getPeak_L()
{
	return m_pFader->getPeak_L();
}




void FxMixerLine::setPeak_R(float peak)
{
	if (peak != getPeak_R() ) {
		m_pFader->setPeak_R( peak );
		if (peak > m_fMaxPeak) {
			m_pPeakLCD->setText( QString("%1").arg(peak, 0, 'f', 2) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}

			m_fMaxPeak = peak;
		}
	}
}




float FxMixerLine::getPeak_R()
{
	return m_pFader->getPeak_R();
}




bool FxMixerLine::isFxActive()
{
	return activeBtn->isPressed();
}




void FxMixerLine::setFxActive( bool active )
{
	activeBtn->setPressed( active );
}






////////////////////////////////

//QPixmap* InstrumentNameWidget::m_pBackground = NULL;


InstrumentNameWidget::InstrumentNameWidget(QWidget* parent)
 : PixmapWidget( parent, "InstrumentNameWidget" )
{
//	infoLog( "INIT" );
	m_nWidgetWidth = 17;
	m_nWidgetHeight = 116;

	T<Preferences>::shared_ptr pref = g_engine->get_preferences();
	QString family = pref->getMixerFontFamily();
	int size = pref->getMixerFontPointSize();
	m_mixerFont.setFamily( family );
	m_mixerFont.setPointSize( size );
//	m_mixerFont.setBold( true );
//	m_mixerFont.setItalic( true );

	setPixmap( "/mixerPanel/mixerline_label_background.png" );

	this->resize( m_nWidgetWidth, m_nWidgetHeight );
}




InstrumentNameWidget::~InstrumentNameWidget()
{
//	infoLog( "DESTROY" );
}



void InstrumentNameWidget::paintEvent( QPaintEvent* ev )
{
	PixmapWidget::paintEvent( ev );

	QPainter p( this );

	p.setPen( QColor(230, 230, 230) );
	p.setFont( m_mixerFont );
	p.rotate( -90 );
	p.drawText( -m_nWidgetHeight + 5, 0, m_nWidgetHeight - 10, m_nWidgetWidth, Qt::AlignVCenter, m_sInstrName );
}




void InstrumentNameWidget::setText( QString text )
{
	if (m_sInstrName != text ) {
		m_sInstrName = text;
		update();
	}
}



QString InstrumentNameWidget::text()
{
	return m_sInstrName;
}



void InstrumentNameWidget::mousePressEvent( QMouseEvent * /*e*/ )
{
	emit clicked();
}



void InstrumentNameWidget::mouseDoubleClickEvent( QMouseEvent * /*e*/ )
{
	emit doubleClicked();
}



// :::::::::::::::::::::



LadspaFXMixerLine::LadspaFXMixerLine(QWidget* parent)
 : PixmapWidget( parent, "LadspaFXMixerLine" )
{
	resize( 194, 43 );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	setPixmap( "/mixerPanel/fxline_background.png" );

	// active button
	m_pActiveBtn = new ToggleButton(
			this,
			"/mixerPanel/bypass_on.png",
			"/mixerPanel/bypass_off.png",
			"/mixerPanel/bypass_over.png",
			QSize( 30, 13 )
	);
	m_pActiveBtn->move( 55, 25 );
	m_pActiveBtn->setToolTip( trUtf8( "FX bypass") );
	connect( m_pActiveBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	// edit button
	m_pEditBtn = new Button(
			this,
			"/mixerPanel/edit_on.png",
			"/mixerPanel/edit_off.png",
			"/mixerPanel/edit_over.png",
			QSize( 30, 13 )
	);
	m_pEditBtn->move( 87, 25 );
	m_pEditBtn->setToolTip( trUtf8( "Edit FX parameters") );
	connect( m_pEditBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	// instrument name widget
	m_pNameLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 13 );
	m_pNameLCD->move( 11, 9 );
	m_pNameLCD->setText( "No name" );
	m_pNameLCD->setToolTip( trUtf8( "Ladspa FX name" ) );

	// m_pRotary
	m_pRotary = new Rotary( this,  Rotary::TYPE_NORMAL, trUtf8( "Effect return" ), false, false );
	m_pRotary->move( 132, 4 );
	connect( m_pRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
}



LadspaFXMixerLine::~LadspaFXMixerLine()
{
//	infoLog( "DESTROY" );
}



void LadspaFXMixerLine::setName(QString name)
{
	m_pNameLCD->setText( name );
}

void LadspaFXMixerLine::click(Button *ref)
{
	if ( ref == m_pActiveBtn ) {
		emit activeBtnClicked( this );
	}
	else if( ref == m_pEditBtn ) {
		emit editBtnClicked( this );
	}
}



bool LadspaFXMixerLine::isFxActive()
{
	return !m_pActiveBtn->isPressed();
}




void LadspaFXMixerLine::setFxActive( bool active )
{
	m_pActiveBtn->setPressed( !active );
}



void LadspaFXMixerLine::rotaryChanged(Rotary * /*ref*/)
{
	m_fMaxPeak = 0.0;

	T<Song>::shared_ptr song = g_engine->getSong();
	song->set_modified( true );
	emit volumeChanged(this);
}



void LadspaFXMixerLine::setPeaks( float /*fPeak_L*/, float /*fPeak_R*/ )
{
/*
	m_pPeakmeter->setPeak_L( fPeak_L );
	m_pPeakmeter->setPeak_R( fPeak_R );
	m_pPeakmeter->updateFader();
*/
}



void LadspaFXMixerLine::getPeaks( float * /*fPeak_L*/, float * /*fPeak_R*/ )
{
/*
	(*fPeak_L) = m_pFader->getPeak_L();
	(*fPeak_R) = m_pFader->getPeak_R();
*/
}



float LadspaFXMixerLine::getVolume()
{
	return m_pRotary->getValue();
}



void LadspaFXMixerLine::setVolume(float value)
{
	m_pRotary->setValue( value );
// 	m_pRotary->updateRotary();
}
