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

#include "Mixer.hpp"
#include "MixerLine.hpp"

#include "../Skin.hpp"
#include "../CompositeApp.hpp"
#include "../LadspaFXProperties.hpp"
#include "../InstrumentEditor/InstrumentEditorPanel.hpp"
#include "../widgets/Button.hpp"
#include "../widgets/PixmapWidget.hpp"

#include <Tritium/Engine.hpp>
#include <Tritium/Mixer.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Note.hpp>
#include <Tritium/fx/Effects.hpp>
#include <Tritium/memory.hpp>

#include <cassert>

// These workaround the namespace clash between
// Tritium::Mixer and Composite's ::Mixer.
using Tritium::Engine;
using Tritium::g_engine;
using Tritium::T;
using Tritium::Instrument;
using Tritium::InstrumentList;
using Tritium::Song;
using Tritium::Note;
using Tritium::Preferences;
using Tritium::LadspaFX;

#define MIXER_STRIP_WIDTH	56
#define MASTERMIXER_STRIP_WIDTH	126


Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
// : QWidget( pParent, Qt::WindowStaysOnTopHint )
// : QWidget( pParent, Qt::Tool )
{
	setWindowTitle( trUtf8( "Mixer" ) );
	setMaximumHeight( 284 );
	setMinimumHeight( 284 );
	setFixedHeight( 284 );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setMargin( 0 );

	m_pFaderPanel = new QWidget( NULL );
	m_pFaderPanel->resize( MIXER_STRIP_WIDTH * MAX_INSTRUMENTS, height() );

	m_pFaderPanel->setLayout( m_pFaderHBox );

	m_pFaderScrollArea = new QScrollArea( NULL );
	m_pFaderScrollArea->setFrameShape( QFrame::NoFrame );
	m_pFaderScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pFaderScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pFaderScrollArea->setMinimumWidth( MIXER_STRIP_WIDTH * 4 );
	m_pFaderScrollArea->setWidget( m_pFaderPanel );

	for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
		m_pMixerLine[ i ] = NULL;
	}

//~ fader panel


// fX frame
	m_pFXFrame = new PixmapWidget( NULL );
	m_pFXFrame->setFixedSize( 213, height() );
	m_pFXFrame->setPixmap( "/mixerPanel/background_FX.png" );
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXLine[nFX] = new LadspaFXMixerLine( m_pFXFrame );
		m_pLadspaFXLine[nFX]->move( 13, 43 * nFX + 84 );
		connect( m_pLadspaFXLine[nFX], SIGNAL( activeBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaActiveBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( editBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaEditBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( volumeChanged(LadspaFXMixerLine*) ), this, SLOT( ladspaVolumeChanged( LadspaFXMixerLine*) ) );
	}

	if ( g_engine->get_preferences()->isFXTabVisible() ) {
		m_pFXFrame->show();
	}
	else {
		m_pFXFrame->hide();
	}
//~ fX frame


// Master frame
	m_pMasterLine = new MasterMixerLine( NULL );
	m_pMasterLine->move( 0, 0 );
	connect( m_pMasterLine, SIGNAL( volumeChanged(MasterMixerLine*) ), this, SLOT( masterVolumeChanged(MasterMixerLine*) ) );

	m_pShowFXPanelBtn = new ToggleButton(
			m_pMasterLine,
			"/mixerPanel/showFX_on.png",
			"/mixerPanel/showFX_off.png",
			"/mixerPanel/showFX_over.png",
			QSize(42, 13)
	);
	m_pShowFXPanelBtn->move( 67, 242 );
	m_pShowFXPanelBtn->setPressed(false);
	m_pShowFXPanelBtn->setToolTip( trUtf8( "Show FX panel" ) );
	connect( m_pShowFXPanelBtn, SIGNAL(clicked(Button*)), this, SLOT( showFXPanelClicked(Button*)));
	m_pShowFXPanelBtn->setPressed( g_engine->get_preferences()->isFXTabVisible() );

#ifndef LADSPA_SUPPORT
	m_pShowFXPanelBtn->hide();
#endif



	m_pShowPeaksBtn = new ToggleButton(
			m_pMasterLine,
			"/mixerPanel/showPeaks_on.png",
			"/mixerPanel/showPeaks_off.png",
			"/mixerPanel/showPeaks_over.png",
			QSize(42, 13)
	);
	m_pShowPeaksBtn->move( 67, 258 );
	m_pShowPeaksBtn->setPressed( (g_engine->get_preferences())->showInstrumentPeaks() );
	m_pShowPeaksBtn->setToolTip( trUtf8( "Show instrument peaks" ) );
	connect( m_pShowPeaksBtn, SIGNAL(clicked(Button*)), this, SLOT( showPeaksBtnClicked(Button*)));
//~ Master frame




	// LAYOUT!
	QHBoxLayout *pLayout = new QHBoxLayout();
	pLayout->setSpacing( 0 );
	pLayout->setMargin( 0 );

	pLayout->addWidget( m_pFaderScrollArea );
	pLayout->addWidget( m_pFXFrame );
	pLayout->addWidget( m_pMasterLine );
	this->setLayout( pLayout );



	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMixer() ) );
	m_pUpdateTimer->start(50);

	CompositeApp::get_instance()->addEventListener( this );
}



Mixer::~Mixer()
{
	m_pUpdateTimer->stop();
}



MixerLine* Mixer::createMixerLine()
{
	MixerLine *pMixerLine = new MixerLine( 0 );
	pMixerLine->setVolume( 0.2 );
	pMixerLine->setMuteClicked( false );
	pMixerLine->setSoloClicked( false );

	connect( pMixerLine, SIGNAL( noteOnClicked(MixerLine*) ), this, SLOT( noteOnClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( noteOffClicked(MixerLine*) ), this, SLOT( noteOffClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( muteBtnClicked(MixerLine*) ), this, SLOT( muteClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( soloBtnClicked(MixerLine*) ), this, SLOT( soloClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( volumeChanged(MixerLine*) ), this, SLOT( volumeChanged(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameClicked(MixerLine*) ), this, SLOT( nameClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameSelected(MixerLine*) ), this, SLOT( nameSelected(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( panChanged(MixerLine*) ), this, SLOT( panChanged( MixerLine*) ) );
	connect( pMixerLine, SIGNAL( knobChanged(MixerLine*, int) ), this, SLOT( knobChanged( MixerLine*, int) ) );

	return pMixerLine;
}


void Mixer::muteClicked(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	g_engine->setSelectedInstrumentNumber( nLine );
	bool isMuteClicked = ref->isMuteClicked();

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	T<InstrumentList>::shared_ptr instrList = g_engine->get_sampler()->get_instrument_list();

	T<Instrument>::shared_ptr pInstr = instrList->get(nLine);
	pInstr->set_muted( isMuteClicked);
	//(CompositeApp::get_instance())->setSelectedInstrument(nLine);
	g_engine->setSelectedInstrumentNumber(nLine);
}



void Mixer::soloClicked(MixerLine* ref)
{
	Engine *pEngine = g_engine;
	T<Song>::shared_ptr pSong = pEngine->getSong();
	T<InstrumentList>::shared_ptr pInstrList = g_engine->get_sampler()->get_instrument_list();
	int nInstruments = pInstrList->get_size();

	int nLine = findMixerLineByRef(ref);
	pEngine->setSelectedInstrumentNumber( nLine );
	bool isSoloClicked = ref->isSoloClicked();

	if (isSoloClicked) {
		for ( int i = 0; i < nInstruments; ++i ) {
			m_pMixerLine[i]->setSoloClicked( false );
			m_pMixerLine[i]->setMuteClicked( true );
			pInstrList->get( i )->set_muted( true );
		}
		m_pMixerLine[nLine]->setSoloClicked( true );
		m_pMixerLine[nLine]->setMuteClicked( false );
		pInstrList->get( nLine )->set_muted( false );
	}
	else {
		for ( int i = 0; i < nInstruments; ++i ) {
			m_pMixerLine[i]->setMuteClicked( false );
			m_pMixerLine[i]->setSoloClicked( false );
			pInstrList->get( i )->set_muted( false );
		}
	}

	g_engine->setSelectedInstrumentNumber(nLine);
}



/// used in PatternEditorInstrumentList
void Mixer::soloClicked(uint nLine)
{
	MixerLine * L = m_pMixerLine[ nLine ];
	L->setSoloClicked( !L->isSoloClicked() );
	soloClicked( L );
// 	Engine *pEngine = g_engine;
// 	Song *pSong = pEngine->getSong();
// 	InstrumentList *pInstrList = pSong->get_instrument_list();
// 	int nInstruments = pInstrList->get_size();
// 
// 	bool isSoloClicked = m_pMixerLine[ nLine ]->isSoloClicked();
// 
// 	if (!isSoloClicked) {
// 		for ( int i = 0; i < nInstruments; i++ ) {
// 			m_pMixerLine[i]->setSoloClicked( false );
// 			m_pMixerLine[i]->setMuteClicked( true );
// 			pInstrList->get( i )->set_muted( true );
// 		}
// 		m_pMixerLine[nLine]->setSoloClicked( true );
// 		m_pMixerLine[nLine]->setMuteClicked( false );
// 		pInstrList->get( nLine )->set_muted( false );
// 	}
// 	else {
// 		for ( int i = 0; i < nInstruments; i++ ) {
// 			m_pMixerLine[i]->setMuteClicked( false );
// 			m_pMixerLine[i]->setSoloClicked( false );
// 			pInstrList->get( i )->set_muted( false );
// 		}
// 	}

}

bool Mixer::isSoloClicked( uint n )
{
	if ( n >= MAX_INSTRUMENTS || m_pMixerLine[ n ] == NULL ) {
		return false;
	}
	return m_pMixerLine[ n ]->isSoloClicked();
}

void Mixer::noteOnClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	g_engine->setSelectedInstrumentNumber( nLine );

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	T<InstrumentList>::shared_ptr instrList = g_engine->get_sampler()->get_instrument_list();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get(nLine), 1.0, 0.5f, 0.5f, -1, fPitch );
	g_engine->midi_noteOn(note);

	g_engine->setSelectedInstrumentNumber(nLine);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	g_engine->setSelectedInstrumentNumber( nLine );

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	T<InstrumentList>::shared_ptr instrList = g_engine->get_sampler()->get_instrument_list();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get( nLine ), 1.0, 0.5, 0.5, -1, fPitch );
	g_engine->midi_noteOff(note);

	g_engine->setSelectedInstrumentNumber(nLine);
}



uint Mixer::findMixerLineByRef(MixerLine* ref)
{
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pMixerLine[i] == ref) {
			return i;
		}
	}
	return 0;
}



void Mixer::volumeChanged(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	g_engine->setSelectedInstrumentNumber( nLine );

	T<Tritium::Mixer::Channel>::shared_ptr chan = g_engine->get_mixer()->channel(nLine);
	chan->gain( ref->getVolume() );
	g_engine->setSelectedInstrumentNumber(nLine);
}




void Mixer::masterVolumeChanged(MasterMixerLine* ref)
{
	float volume = ref->getVolume();
	T<Tritium::Mixer>::shared_ptr mixer = g_engine->get_mixer();
	mixer->gain(volume);
}



void Mixer::updateMixer()
{
	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Engine *pEngine = g_engine;
	T<Tritium::Mixer>::shared_ptr t_mixer = g_engine->get_mixer();
	T<InstrumentList>::shared_ptr pInstrList = g_engine->get_sampler()->get_instrument_list();

	uint nSelectedInstr = pEngine->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	uint nMuteClicked = 0;
	uint nInstruments = pInstrList->get_size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {

		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pMixerLine[ nInstr ] ) {
				delete m_pMixerLine[ nInstr ];
				m_pMixerLine[ nInstr ] = NULL;

				int newWidth = MIXER_STRIP_WIDTH * nInstruments;
				if ( m_pFaderPanel->width() != newWidth ) {
					m_pFaderPanel->resize( newWidth, height() );
				}
			}
			continue;
		}
		else {
			if ( m_pMixerLine[ nInstr ] == NULL ) {
				// the mixerline doesn't exists..I'll create a new one!
				m_pMixerLine[ nInstr ] = createMixerLine();
				m_pFaderHBox->addWidget( m_pMixerLine[ nInstr ] );

				int newWidth = MIXER_STRIP_WIDTH * nInstruments;
				if ( m_pFaderPanel->width() != newWidth ) {
					m_pFaderPanel->resize( newWidth, height() );
				}
			}
			MixerLine *pLine = m_pMixerLine[ nInstr ];

			T<Instrument>::shared_ptr pInstr = pInstrList->get( nInstr );
			T<Tritium::Mixer::Channel>::shared_ptr chan = g_engine->get_mixer()->channel(nInstr);

			assert( pInstr );

			float fNewPeak_L = pInstr->get_peak_l();
			pInstr->set_peak_l( 0.0f );	// reset instrument peak

			float fNewPeak_R = pInstr->get_peak_r();
			pInstr->set_peak_r( 0.0f );	// reset instrument peak

			float fNewVolume = chan->gain();
			bool bMuted = pInstr->is_muted();

			QString sName = pInstr->get_name();
			float fPan_L = pInstr->get_pan_l();
			float fPan_R = pInstr->get_pan_r();


			// fader
			float fOldPeak_L = pLine->getPeak_L();
			float fOldPeak_R = pLine->getPeak_R();

			if (!bShowPeaks) {
				fNewPeak_L = 0.0f;
				fNewPeak_R = 0.0f;
			}

			if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
				pLine->setPeak_L( fNewPeak_L );
			}
			else {
				pLine->setPeak_L( fOldPeak_L / fallOff );
			}
			if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
				pLine->setPeak_R( fNewPeak_R );
			}
			else {
				pLine->setPeak_R( fOldPeak_R / fallOff );
			}

			// fader position
			pLine->setVolume( fNewVolume );

			// mute
			if ( bMuted ) {
				nMuteClicked++;
			}
			pLine->setMuteClicked( bMuted );

			// instr name
			pLine->setName( sName );

			// pan
			float fPanValue = 0.0;
			if (fPan_R == 1.0) {
				fPanValue = 1.0 - (fPan_L / 2.0);
			}
			else {
				fPanValue = fPan_R / 2.0;
			}

			pLine->setPan( fPanValue );

			// activity
			if ( pLine->getActivity() > 0 ) {
				pLine->setActivity( m_pMixerLine[ nInstr ]->getActivity() - 30 );
				pLine->setPlayClicked( true );
			}
			else {
				pLine->setPlayClicked( false );
			}

			for (uint nFX = 0; nFX < MAX_FX; nFX++) {
			    pLine->setFXLevel( nFX, chan->send_gain(nFX) );
			}

			pLine->setSelected( nInstr == nSelectedInstr );

			pLine->updateMixerLine();
		}
	}

	if (nMuteClicked == nInstruments - 1 ) {
		// find the not muted button
		for (uint i = 0; i < nInstruments; i++) {
			T<Instrument>::shared_ptr instr = pInstrList->get(i);
			if (instr->is_muted() == false) {
				m_pMixerLine[i]->setSoloClicked(true);
				break;
			}
		}
	}
	else {
		for (uint i = 0; i < nInstruments; i++) {
			m_pMixerLine[i]->setSoloClicked(false);
		}
	}


	// update MasterPeak
	float oldPeak_L = m_pMasterLine->getPeak_L();
	float newPeak_L = pEngine->getMasterPeak_L();
	pEngine->setMasterPeak_L(0.0);
	float oldPeak_R = m_pMasterLine->getPeak_R();
	float newPeak_R = pEngine->getMasterPeak_R();
	pEngine->setMasterPeak_R(0.0);

	if (!bShowPeaks) {
		newPeak_L = 0.0;
		newPeak_R = 0.0;
	}

	if (newPeak_L >= oldPeak_L) {
		m_pMasterLine->setPeak_L( newPeak_L );
	}
	else {
		m_pMasterLine->setPeak_L( oldPeak_L / fallOff );
	}
	if (newPeak_R >= oldPeak_R) {
		m_pMasterLine->setPeak_R(newPeak_R);
	}
	else {
		m_pMasterLine->setPeak_R( oldPeak_R / fallOff );
	}




	// set master fader position
	float newVolume = t_mixer->gain();
	float oldVolume = m_pMasterLine->getVolume();
	if (oldVolume != newVolume) {
		m_pMasterLine->setVolume(newVolume);
	}
	m_pMasterLine->updateMixerLine();


#ifdef LADSPA_SUPPORT
	// LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX( nFX );
		if ( pFX ) {
			m_pLadspaFXLine[nFX]->setName( pFX->getPluginName() );
			float fNewPeak_L = 0.0;
			float fNewPeak_R = 0.0;

			float fOldPeak_L = 0.0;
			float fOldPeak_R = 0.0;
			m_pLadspaFXLine[nFX]->getPeaks( &fOldPeak_L, &fOldPeak_R );

			if (fNewPeak_L < fOldPeak_L)	fNewPeak_L = fOldPeak_L / fallOff;
			if (fNewPeak_R < fOldPeak_R)	fNewPeak_R = fOldPeak_R / fallOff;
			m_pLadspaFXLine[nFX]->setPeaks( fNewPeak_L, fNewPeak_R );
			m_pLadspaFXLine[nFX]->setFxActive( pFX->isEnabled() );
			m_pLadspaFXLine[nFX]->setVolume( pFX->getVolume() );
		}
		else {
			m_pLadspaFXLine[nFX]->setName( "No plugin" );
			m_pLadspaFXLine[nFX]->setFxActive( false );
			m_pLadspaFXLine[nFX]->setVolume( 0.0 );
		}
	}
	// ~LADSPA
#endif
}



/// show event
void Mixer::showEvent ( QShowEvent * /*ev*/ )
{
	updateMixer();
}



/// hide event
void Mixer::hideEvent ( QHideEvent * /*ev*/ )
{
}



void Mixer::nameClicked(MixerLine* /*ref*/)
{
	InstrumentEditorPanel::get_instance()->show();
}



void Mixer::nameSelected(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	g_engine->setSelectedInstrumentNumber( nLine );

	g_engine->setSelectedInstrumentNumber(nLine);
}



void Mixer::panChanged(MixerLine* ref) {
	float panValue = ref->getPan();

	float pan_L;
	float pan_R;

	if (panValue >= 0.5) {
		pan_L = (1.0 - panValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = panValue * 2;
	}

	int nLine = findMixerLineByRef(ref);
	g_engine->setSelectedInstrumentNumber( nLine );

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	T<InstrumentList>::shared_ptr instrList = g_engine->get_sampler()->get_instrument_list();

	T<Instrument>::shared_ptr instr = instrList->get(nLine);
	instr->set_pan_l( pan_L );
	instr->set_pan_r( pan_R );


	g_engine->setSelectedInstrumentNumber(nLine);
}



void Mixer::knobChanged(MixerLine* ref, int nKnob) {
	int nLine = findMixerLineByRef(ref);
	g_engine->setSelectedInstrumentNumber( nLine );
	T<Tritium::Mixer::Channel>::shared_ptr chan = g_engine->get_mixer()->channel(nLine);
	chan->send_gain(nKnob, ref->getFXLevel(nKnob));

	QString sInfo = trUtf8( "Set FX %1 level ").arg( nKnob + 1 );
	( CompositeApp::get_instance() )->setStatusBarMessage( sInfo+ QString( "[%1]" ).arg( ref->getFXLevel(nKnob), 0, 'f', 2 ), 2000 );

	g_engine->setSelectedInstrumentNumber(nLine);
}



void Mixer::noteOnEvent( int nInstrument )
{
	if ( m_pMixerLine[ nInstrument ] ) {
		m_pMixerLine[ nInstrument ]->setActivity( 100 );
	}
}



void Mixer::resizeEvent ( QResizeEvent * /*ev*/ )
{
/*
	uint nMaster_X = width() - MASTERMIXER_STRIP_WIDTH;
	int nFXFrameWidth = 213;

	if ( m_pFXFrame->isVisible() ) {
		int w = width() - MASTERMIXER_STRIP_WIDTH - nFXFrameWidth;
		m_pFaderScrollArea->resize( w, m_nMixerHeight );
	}
	else {
		int w = width() - MASTERMIXER_STRIP_WIDTH;
		m_pFaderScrollArea->resize( w, m_nMixerHeight );
	}

	m_pFXFrame->move( nMaster_X - nFXFrameWidth, 0 );

	m_pMasterLine->move( nMaster_X, 0);
	m_pShowPeaksBtn->move( nMaster_X + 67, 242 );
	m_pShowFXPanelBtn->move( nMaster_X + 67, 258 );

	resize(width(), m_nMixerHeight);	// qt bug workaround
*/
}



void Mixer::showFXPanelClicked(Button* ref)
{
	if ( ref->isPressed() ) {
		m_pFXFrame->show();
		g_engine->get_preferences()->setFXTabVisible( true );
	}
	else {
		m_pFXFrame->hide();
		g_engine->get_preferences()->setFXTabVisible( false );
	}

	resizeEvent( NULL ); 	// force an update
}



void Mixer::showPeaksBtnClicked(Button* ref)
{
	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();

	if ( ref->isPressed() ) {
		pPref->setInstrumentPeaks( true );
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = On"), 2000 );
	}
	else {
		pPref->setInstrumentPeaks( false );
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = Off"), 2000 );
	}
}



void Mixer::ladspaActiveBtnClicked( LadspaFXMixerLine* ref )
{
#ifdef LADSPA_SUPPORT
	bool bActive = ref->isFxActive();

	//Engine *engine = g_engine;
	//Song *song = engine->getSong();

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setEnabled( bActive );
			}
			break;
		}
	}
#endif
}



void Mixer::ladspaEditBtnClicked( LadspaFXMixerLine *ref )
{
#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			CompositeApp::get_instance()->getLadspaFXProperties(nFX)->hide();
			CompositeApp::get_instance()->getLadspaFXProperties(nFX)->show();
		}
	}
	g_engine->getSong()->set_modified(true);
#endif
}



void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef LADSPA_SUPPORT
	T<Song>::shared_ptr pSong = (g_engine )->getSong();
	pSong->set_modified(true);

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			T<LadspaFX>::shared_ptr pFX = g_engine->get_effects()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setVolume( ref->getVolume() );
				QString sInfo = trUtf8( "Set LADSPA FX ( %1 ) volume").arg( QString(pFX->getPluginName() ) );
				CompositeApp::get_instance()->setStatusBarMessage( sInfo+ QString( " [%1]" ).arg( ref->getVolume(), 0, 'f', 2 ), 2000 );
			}
		}
	}
#endif
}




void Mixer::getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R )
{
	if ( nMixerLine < MAX_INSTRUMENTS ) {
		fPeak_L = m_pMixerLine[ nMixerLine ]->getPeak_L();
		fPeak_R = m_pMixerLine[ nMixerLine ]->getPeak_R();
	}
	else {
		fPeak_L = 0;
		fPeak_R = 0;
	}
}
