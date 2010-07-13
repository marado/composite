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

#include "version.h"

#include "CompositeApp.hpp"
#include "Skin.hpp"
#include "PreferencesDialog.hpp"
#include "MainForm.hpp"
#include "PlayerControl.hpp"
#include "AudioEngineInfoForm.hpp"
#include "HelpBrowser.hpp"
#include "LadspaFXProperties.hpp"
#include "InstrumentRack.hpp"

#include "PatternEditor/PatternEditorPanel.hpp"
#include "InstrumentEditor/InstrumentEditorPanel.hpp"
#include "SongEditor/SongEditor.hpp"
#include "SongEditor/SongEditorPanel.hpp"
#include "PlaylistEditor/PlaylistDialog.hpp"
//#include "AudioFileBrowser/AudioFileBrowser.hpp"
#include "InstrumentRack.hpp"
#include "SoundLibrary/SoundLibraryPanel.hpp"

#include "Mixer/Mixer.hpp"
#include "Mixer/MixerLine.hpp"

#include <Tritium/Engine.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/fx/LadspaFX.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/Playlist.hpp>
#include <Tritium/Logger.hpp>

#include <QtGui>
#include <cassert>

using namespace Tritium;

CompositeApp* CompositeApp::m_pInstance = NULL;

class AppPlaylistListener : public Tritium::PlaylistListener
{
public:
    CompositeApp* q;
    Tritium::Playlist *d;

    AppPlaylistListener() : q(0), d(0) {}
    ~AppPlaylistListener() {
	if(d) d->unsubscribe();
    }

    void selection_changed() {
	if(q) {
	    q->getInstrumentRack()
		->getSoundLibraryPanel()
		->update_background_color();
	}
    }

    void set_song(Tritium::T<Tritium::Song>::shared_ptr pSong) {
	if(q) {
	    q->setSong(pSong);
	}
    }
};

CompositeApp::CompositeApp( MainForm *pMainForm, T<Song>::shared_ptr pFirstSong )
 : m_pMainForm( pMainForm )
 , m_pMixer( NULL )
 , m_pPatternEditorPanel( NULL )
 , m_pAudioEngineInfoForm( NULL )
 , m_pSongEditorPanel( NULL )
 , m_pHelpBrowser( NULL )
 , m_pFirstTimeInfo( NULL )
 , m_pPlayerControl( NULL )
 , m_pPlaylistDialog( NULL )

{
	m_pInstance = this;

	m_pEventQueueTimer = new QTimer(this);
	connect( m_pEventQueueTimer, SIGNAL( timeout() ), this, SLOT( onEventQueueTimer() ) );
	m_pEventQueueTimer->start(50);	// update at 20 fps


	// Audio Engine must already be created
	assert(g_engine);
	g_engine->setSong( pFirstSong );
	g_engine->get_preferences()->setLastSongFilename( pFirstSong->get_filename() );

	// set initial title
	QString qsSongName( pFirstSong->get_name() );
	if( qsSongName == "Untitled Song" && !pFirstSong->get_filename().isEmpty() ){
		qsSongName = pFirstSong->get_filename();
		qsSongName = qsSongName.section( '/', -1 );
	}

        setWindowTitle( qsSongName  );

	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();

	setupSinglePanedInterface();

	// restore audio engine form properties
	m_pAudioEngineInfoForm = new AudioEngineInfoForm( 0 );
	WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
	m_pAudioEngineInfoForm->move( audioEngineInfoProp.x, audioEngineInfoProp.y );
	if ( audioEngineInfoProp.visible ) {
		m_pAudioEngineInfoForm->show();
	}
	else {
		m_pAudioEngineInfoForm->hide();
	}
	
	m_pAppPlaylistListener = new AppPlaylistListener;
	m_pAppPlaylistListener->q = this;
	m_pAppPlaylistListener->d = &g_engine->get_playlist();
	m_pAppPlaylistListener->d->subscribe(m_pAppPlaylistListener);
	// Unsubscription done by the destructor.
	m_pPlaylistDialog = new PlaylistDialog( 0 );
	
	showInfoSplash();	// First time information
}



CompositeApp::~CompositeApp()
{
	DEBUGLOG( "[~CompositeApp]" );
	m_pEventQueueTimer->stop();

	delete m_pHelpBrowser;
	delete m_pAudioEngineInfoForm;
	delete m_pMixer;
	delete m_pPlaylistDialog;
	delete m_pAppPlaylistListener;

	Engine *engine = g_engine;
	if (engine) {
		T<Song>::shared_ptr song = engine->getSong();
		// Engine calls removeSong on from its destructor, so here we just delete the objects:
		song.reset();
		delete engine;
	}

	#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		delete m_pLadspaFXProperties[nFX];
	}
	#endif
}



/// Return an CompositeApp m_pInstance
CompositeApp* CompositeApp::get_instance() {
	if (m_pInstance == NULL) {
		std::cerr << "Error! CompositeApp::get_instance (m_pInstance = NULL)" << std::endl;
	}
	return m_pInstance;
}




void CompositeApp::setupSinglePanedInterface()
{
	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();

	// MAINFORM
	WindowProperties mainFormProp = pPref->getMainFormProperties();
	m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
	m_pMainForm->move( mainFormProp.x, mainFormProp.y );

	QSplitter *pSplitter = new QSplitter( NULL );
	pSplitter->setOrientation( Qt::Vertical );
	pSplitter->setOpaqueResize( true );

	// SONG EDITOR
	m_pSongEditorPanel = new SongEditorPanel( pSplitter );
	WindowProperties songEditorProp = pPref->getSongEditorProperties();
	m_pSongEditorPanel->resize( songEditorProp.width, songEditorProp.height );

	// this HBox will contain the InstrumentRack and the Pattern editor
	QWidget *pSouthPanel = new QWidget( pSplitter );
	QHBoxLayout *pEditorHBox = new QHBoxLayout();
	pEditorHBox->setSpacing( 5 );
	pEditorHBox->setMargin( 0 );
	pSouthPanel->setLayout( pEditorHBox );

	// INSTRUMENT RACK
	m_pInstrumentRack = new InstrumentRack( NULL );

	// PATTERN EDITOR
	m_pPatternEditorPanel = new PatternEditorPanel( NULL );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );

	pEditorHBox->addWidget( m_pPatternEditorPanel );
	pEditorHBox->addWidget( m_pInstrumentRack );

	// PLayer control
	m_pPlayerControl = new PlayerControl( NULL );


	QWidget *mainArea = new QWidget( m_pMainForm );	// this is the main widget
	m_pMainForm->setCentralWidget( mainArea );

	// LAYOUT!!
	QVBoxLayout *pMainVBox = new QVBoxLayout();
	pMainVBox->setSpacing( 5 );
	pMainVBox->setMargin( 0 );
	pMainVBox->addWidget( m_pPlayerControl );
	pMainVBox->addWidget( pSplitter );

	mainArea->setLayout( pMainVBox );




	// MIXER
	m_pMixer = new ::Mixer(0);
	WindowProperties mixerProp = pPref->getMixerProperties();
	m_pMixer->resize( mixerProp.width, mixerProp.height );
	m_pMixer->move( mixerProp.x, mixerProp.y );
	m_pMixer->updateMixer();
	if ( mixerProp.visible ) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}


	// HELP BROWSER
	QString sDocPath = QString( DataPath::get_data_path() ) + "/doc";
	QString sDocURI = sDocPath + "/manual.html";
	m_pHelpBrowser = new SimpleHTMLBrowser( NULL, sDocPath, sDocURI, SimpleHTMLBrowser::MANUAL );

#ifdef LADSPA_SUPPORT
	// LADSPA FX
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( NULL, nFX );
		m_pLadspaFXProperties[nFX]->hide();
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
		if ( prop.visible ) {
			m_pLadspaFXProperties[nFX]->show();
		}
		else {
			m_pLadspaFXProperties[nFX]->hide();
		}
	}
#endif

//	m_pMainForm->showMaximized();
}


void CompositeApp::closeFXProperties()
{
#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX]->close();
	}
#endif
}

void CompositeApp::setSong(T<Song>::shared_ptr song)
{
	T<Song>::shared_ptr oldSong = (g_engine)->getSong();
	if (oldSong) {
		(g_engine)->removeSong();
		oldSong.reset();
	}

	g_engine->setSong( song );
	g_engine->get_preferences()->setLastSongFilename( song->get_filename() );

	m_pSongEditorPanel->updateAll();
	m_pPatternEditorPanel->updateSLnameLabel();

	QString songName( song->get_name() );
	if( songName == "Untitled Song" && !song->get_filename().isEmpty() ){
		songName = song->get_filename();
		songName = songName.section( '/', -1 );
	}
        setWindowTitle( songName  );

	m_pMainForm->updateRecentUsedSongList();
}



void CompositeApp::showMixer(bool show)
{
	m_pMixer->setVisible( show );
}



void CompositeApp::showPreferencesDialog()
{
	PreferencesDialog preferencesDialog(m_pMainForm);
	preferencesDialog.exec();
}




void CompositeApp::setStatusBarMessage( const QString& msg, int msec )
{
	getPlayerControl()->showMessage( msg, msec );
}

void CompositeApp::setWindowTitle( const QString& title){
    m_pMainForm->setWindowTitle( ( "Composite " + QString( get_version().c_str()) + QString( " - " ) + title ) );
}

void CompositeApp::setScrollStatusBarMessage( const QString& msg, int msec, bool test )
{
	getPlayerControl()->showScrollMessage( msg, msec , test);
}



void CompositeApp::showAudioEngineInfoForm()
{
	m_pAudioEngineInfoForm->hide();
	m_pAudioEngineInfoForm->show();
}

void CompositeApp::showPlaylistDialog()
{
	m_pPlaylistDialog->hide();
	m_pPlaylistDialog->show();
}


void CompositeApp::showInfoSplash()
{
	QString sDocPath( DataPath::get_data_path().append( "/doc/infoSplash" ) );

	QDir dir(sDocPath);
	if ( !dir.exists() ) {
		ERRORLOG( QString("[showInfoSplash] Directory ").append( sDocPath ).append( " not found." ) );
		return;
	}

	QString sFilename;
	int nNewsID = 0;
	QFileInfoList list = dir.entryInfoList();

	for ( int i =0; i < list.size(); ++i ) {
		QString sFile = list.at( i ).fileName();

		if ( sFile == "." || sFile == ".." ) {
			continue;
		}

		int nPos = sFile.lastIndexOf( "-" );
		QString sNewsID = sFile.mid( nPos + 1, sFile.length() - nPos - 1 );
		int nID = sNewsID.toInt();
		if ( nID > nNewsID ) {
			sFilename = sFile;
		}
//		DEBUGLOG( "news: " + sFilename + " id: " + sNewsID );
	}
	DEBUGLOG( "[showInfoSplash] Selected news: " + sFilename );

	QString sLastRead = g_engine->get_preferences()->getLastNews();
	if ( sLastRead != sFilename && !sFilename.isEmpty() ) {
		QString sDocURI = sDocPath;
		sDocURI.append( "/" ).append( sFilename );
		SimpleHTMLBrowser *m_pFirstTimeInfo = new SimpleHTMLBrowser( m_pMainForm, sDocPath, sDocURI, SimpleHTMLBrowser::WELCOME );
		if ( m_pFirstTimeInfo->exec() == QDialog::Accepted ) {
			g_engine->get_preferences()->setLastNews( sFilename );
		}
	}
}

void CompositeApp::onDrumkitLoad( QString name ){
	setStatusBarMessage( trUtf8( "Drumkit loaded: [%1]" ).arg( name ), 2000 );
	m_pPatternEditorPanel->updateSLnameLabel( );
}

void CompositeApp::onEventQueueTimer()
{
	// use the timer to do schedule instrument slaughter;
	T<EventQueue>::shared_ptr pQueue = g_engine->get_event_queue();

	Event event;
	while ( ( event = pQueue->pop_event() ).type != EVENT_NONE ) {
		for (int i = 0; i < (int)m_eventListeners.size(); i++ ) {
			EventListener *pListener = m_eventListeners[ i ];

			switch ( event.type ) {
				case EVENT_STATE:
					pListener->stateChangedEvent( event.value );
					break;

				case EVENT_PATTERN_CHANGED:
					pListener->patternChangedEvent();
					break;

				case EVENT_PATTERN_MODIFIED:
					pListener->patternModifiedEvent();
					break;

				case EVENT_SELECTED_PATTERN_CHANGED:
					pListener->selectedPatternChangedEvent();
					break;

				case EVENT_SELECTED_INSTRUMENT_CHANGED:
					pListener->selectedInstrumentChangedEvent();
					break;

				case EVENT_MIDI_ACTIVITY:
					pListener->midiActivityEvent();
					break;

				case EVENT_NOTEON:
					pListener->noteOnEvent( event.value );
					break;

				case EVENT_ERROR:
					pListener->errorEvent( event.value );
					break;

				case EVENT_XRUN:
					pListener->XRunEvent();
					break;

				case EVENT_METRONOME:
					pListener->metronomeEvent( event.value );
					break;

				case EVENT_PROGRESS:
					pListener->progressEvent( event.value );
					break;

				case EVENT_TRANSPORT:
					pListener->transportEvent( (TransportPosition::State)event.value );
					break;

				case EVENT_JACK_TIME_MASTER:
					pListener->jackTimeMasterEvent( event.value );
					break;

				default:
					ERRORLOG( QString("[onEventQueueTimer] Unhandled event: %1").arg( event.type ) );
			}

		}
	}
}


void CompositeApp::addEventListener( EventListener* pListener )
{
	if (pListener) {
		m_eventListeners.push_back( pListener );
	}
}


void CompositeApp::removeEventListener( EventListener* pListener )
{
	for ( uint i = 0; i < m_eventListeners.size(); i++ ) {
		if ( pListener == m_eventListeners[ i ] ) {
			m_eventListeners.erase( m_eventListeners.begin() + i );
		}
	}
}

