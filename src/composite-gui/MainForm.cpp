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

#include <Tritium/Engine.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/Playlist.hpp>
#include <Tritium/ADSR.hpp>
#include <Tritium/smf/SMF.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/LocalFileMng.hpp>
#include <Tritium/Pattern.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/InstrumentLayer.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/memory.hpp>

using namespace Tritium;

#include "AboutDialog.hpp"
#include "AudioEngineInfoForm.hpp"
#include "ExportSongDialog.hpp"
#include "CompositeApp.hpp"
#include "InstrumentRack.hpp"
#include "Skin.hpp"
#include "MainForm.hpp"
#include "PlayerControl.hpp"
#include "HelpBrowser.hpp"
#include "LadspaFXProperties.hpp"
#include "SongPropertiesDialog.hpp"

#include "Mixer/Mixer.hpp"
#include "InstrumentEditor/InstrumentEditorPanel.hpp"
#include "PatternEditor/PatternEditorPanel.hpp"
#include "SongEditor/SongEditor.hpp"
#include "SongEditor/SongEditorPanel.hpp"
#include "SoundLibrary/SoundLibraryPanel.hpp"
#include "SoundLibrary/SoundLibraryImportDialog.hpp"
#include "SoundLibrary/SoundLibrarySaveDialog.hpp"
#include "SoundLibrary/SoundLibraryExportDialog.hpp"

#include <QtGui>

#ifndef WIN32
	#include <sys/time.h>
#endif

#include <memory>
#include <cassert>

using namespace std;
using namespace Tritium;

MainForm::MainForm( QApplication *app, const QString& songFilename )
 : QMainWindow( 0, 0 )
{
	setMinimumSize( QSize( 1000, 600 ) );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	m_pQApp = app;

	m_pQApp->processEvents();

	// Load default song
	T<Song>::shared_ptr song;
	if ( !songFilename.isEmpty() ) {
		song = Song::load( g_engine, songFilename );
		if (song == NULL) {
			//QMessageBox::warning( this, "Composite", trUtf8("Error loading song.") );
			song = Song::get_empty_song(g_engine);
			song->set_filename( "" );
		}
	}
	else {
		T<Preferences>::shared_ptr pref = g_engine->get_preferences();
		bool restoreLastSong = pref->isRestoreLastSongEnabled();
		QString filename = pref->getLastSongFilename();
		if ( restoreLastSong && ( !filename.isEmpty() )) {
			song = Song::load( g_engine, filename );
			if (song == NULL) {
				//QMessageBox::warning( this, "Composite", trUtf8("Error restoring last song.") );
				song = Song::get_empty_song(g_engine);
				song->set_filename( "" );
			}
		}
		else {
			song = Song::get_empty_song(g_engine);
			song->set_filename( "" );
		}
	}

	h2app = new CompositeApp( this, song );
	h2app->addEventListener( this );

	createMenuBar();

	h2app->setStatusBarMessage( trUtf8("Composite Ready."), 10000 );

	initKeyInstMap();

	// we need to do all this to support the keyboard playing
	// for all the window modes
	h2app->getMixer()->installEventFilter (this);
	h2app->getPatternEditorPanel()->installEventFilter (this);
	h2app->getSongEditorPanel()->installEventFilter (this);
	h2app->getPlayerControl()->installEventFilter(this);
	InstrumentEditorPanel::get_instance()->installEventFilter(this);
	h2app->getAudioEngineInfoForm()->installEventFilter(this);

	installEventFilter( this );

	connect( &m_autosaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimer()));
	m_autosaveTimer.start( 60 * 1000 );

//playlist display timer
	QTimer *playlistDisplayTimer = new QTimer(this);
	connect( playlistDisplayTimer, SIGNAL( timeout() ), this, SLOT( onPlaylistDisplayTimer() ) );
	playlistDisplayTimer->start(30000);	// update player control at 
// ~ playlist display timer
	
//beatcouter
	g_engine->setBcOffsetAdjust();

}



MainForm::~MainForm()
{
	// remove the autosave file
	QFile file( getAutoSaveFilename() );
        file.remove();

	g_engine->get_transport()->stop();

	// remove the autosave file
	m_autosaveTimer.stop();
	QFile autosaveFile( "tritium_autosave.h2song" );
	autosaveFile.remove();

	hide();

	if (h2app != NULL) {
		delete h2app;
		h2app = NULL;
	}

}



///
/// Create the menubar
///
void MainForm::createMenuBar()
{
	// menubar
	QMenuBar *m_pMenubar = new QMenuBar( this );
	setMenuBar( m_pMenubar );

	// FILE menu
	QMenu *m_pFileMenu = m_pMenubar->addMenu( trUtf8( "&Project" ) );

	m_pFileMenu->addAction( trUtf8( "&New" ), this, SLOT( action_file_new() ), QKeySequence( "Ctrl+N" ) );
	m_pFileMenu->addAction( trUtf8( "Show &info" ), this, SLOT( action_file_songProperties() ), QKeySequence( "" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "&Open" ), this, SLOT( action_file_open() ), QKeySequence( "Ctrl+O" ) );
	m_pFileMenu->addAction( trUtf8( "Open &Demo" ), this, SLOT( action_file_openDemo() ), QKeySequence( "Ctrl+D" ) );

	m_pRecentFilesMenu = m_pFileMenu->addMenu( trUtf8( "Open &recent" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "&Save" ), this, SLOT( action_file_save() ), QKeySequence( "Ctrl+S" ) );
	m_pFileMenu->addAction( trUtf8( "Save &as..." ), this, SLOT( action_file_save_as() ), QKeySequence( "Ctrl+Shift+S" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction ( trUtf8 ( "Open &Pattern" ), this, SLOT ( action_file_openPattern() ), QKeySequence ( "" ) );
	m_pFileMenu->addAction( trUtf8( "Expor&t pattern as..." ), this, SLOT( action_file_export_pattern_as() ), QKeySequence( "Ctrl+P" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "Export &MIDI file" ), this, SLOT( action_file_export_midi() ), QKeySequence( "Ctrl+M" ) );
	m_pFileMenu->addAction( trUtf8( "&Export song" ), this, SLOT( action_file_export() ), QKeySequence( "Ctrl+E" ) );


#ifndef Q_OS_MACX
	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8("&Quit"), this, SLOT( action_file_exit() ), QKeySequence( "Ctrl+Q" ) );
#endif

	updateRecentUsedSongList();
	connect( m_pRecentFilesMenu, SIGNAL( triggered(QAction*) ), this, SLOT( action_file_open_recent(QAction*) ) );
	//~ FILE menu


	// INSTRUMENTS MENU
	QMenu *m_pInstrumentsMenu = m_pMenubar->addMenu( trUtf8( "I&nstruments" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "&Add instrument" ), this, SLOT( action_instruments_addInstrument() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "&Clear all" ), this, SLOT( action_instruments_clearAll() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "&Save library" ), this, SLOT( action_instruments_saveLibrary() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "&Export library" ), this, SLOT( action_instruments_exportLibrary() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "&Import library" ), this, SLOT( action_instruments_importLibrary() ), QKeySequence( "" ) );




	// Tools menu
	QMenu *m_pToolsMenu = m_pMenubar->addMenu( trUtf8( "&Tools" ));

//	if ( g_engine->get_preferences()->getInterfaceMode() == Preferences::SINGLE_PANED ) {
//		m_pWindowMenu->addAction( trUtf8("Show song editor"), this, SLOT( action_window_showSongEditor() ), QKeySequence( "" ) );
//	}
	m_pToolsMenu->addAction( trUtf8("Playlist &editor"), this, SLOT( action_window_showPlaylistDialog() ), QKeySequence( "" ) );

	m_pToolsMenu->addAction( trUtf8("&Mixer"), this, SLOT( action_window_showMixer() ), QKeySequence( "Alt+M" ) );

	m_pToolsMenu->addAction( trUtf8("&Instrument Rack"), this, SLOT( action_window_showDrumkitManagerPanel() ), QKeySequence( "Alt+I" ) );
	m_pToolsMenu->addAction( trUtf8("&Preferences"), this, SLOT( showPreferencesDialog() ), QKeySequence( "Alt+P" ) );

	//~ Tools menu

        Logger *l = Logger::get_instance();
        if ( l->get_log_level() == 15 ) {
		// DEBUG menu
		QMenu *m_pDebugMenu = m_pMenubar->addMenu( trUtf8("De&bug") );
		m_pDebugMenu->addAction( trUtf8( "Show &audio engine info" ), this, SLOT( action_debug_showAudioEngineInfo() ) );
		m_pDebugMenu->addAction( trUtf8( "Print Objects" ), this, SLOT( action_debug_printObjects() ) );
		//~ DEBUG menu
	}

	// INFO menu
	QMenu *m_pInfoMenu = m_pMenubar->addMenu( trUtf8( "&Info" ) );
	m_pInfoMenu->addAction( trUtf8("&User manual"), this, SLOT( showUserManual() ), QKeySequence( "Ctrl+?" ) );
	m_pInfoMenu->addSeparator();
	m_pInfoMenu->addAction( trUtf8("&About"), this, SLOT( action_help_about() ), QKeySequence( trUtf8("", "Info|About") ) );
	//~ INFO menu
}



bool MainForm::action_file_exit()
{
	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return false;
	}
	closeAll();
	return true;
}



void MainForm::action_file_new()
{
	g_engine->get_transport()->stop();

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return;
	}

	T<Song>::shared_ptr song = Song::get_empty_song(g_engine);
	song->set_filename( "" );
	h2app->setSong(song);
 	g_engine->setSelectedPatternNumber( 0 );
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();
}



void MainForm::action_file_save_as()
{
	g_engine->get_transport()->stop();

        std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode( QFileDialog::AnyFile );
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
        fd->setAcceptMode( QFileDialog::AcceptSave );
        fd->setWindowTitle( trUtf8( "Save song" ) );

	T<Song>::shared_ptr song = g_engine->getSong();
	QString defaultFilename;
	QString lastFilename = song->get_filename();

	if ( lastFilename.isEmpty() ) {
		defaultFilename = g_engine->getSong()->get_name();
		defaultFilename += ".h2song";
	}
	else {
		defaultFilename = lastFilename;
	}

	fd->selectFile( defaultFilename );

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if ( !filename.isEmpty() ) {
		QString sNewFilename = filename;
		if ( sNewFilename.endsWith(".h2song") == false ) {
			filename += ".h2song";
		}

		song->set_filename(filename);
		action_file_save();
	}
	h2app->setScrollStatusBarMessage( trUtf8("Song saved as.") + QString(" Into: ") + defaultFilename, 2000 );
        h2app->setWindowTitle( filename );
}



void MainForm::action_file_save()
{
//	if ( ((g_engine)->getState() == STATE_PLAYING) ) {
//		(g_engine)->stop();
//	}

	T<Song>::shared_ptr song = g_engine->getSong();
	QString filename = song->get_filename();

	if ( filename.isEmpty() ) {
		// just in case!
		return action_file_save_as();
	}

	LocalFileMng mng(g_engine);
	bool saved = false;
	saved = song->save( g_engine, filename );
	

	if(! saved) {
		QMessageBox::warning( this, "Composite", trUtf8("Could not save song.") );
	} else {
		g_engine->get_preferences()->setLastSongFilename( song->get_filename() );

		// add the new loaded song in the "last used song" vector
		T<Preferences>::shared_ptr pPref = g_engine->get_preferences();
		vector<QString> recentFiles = pPref->getRecentFiles();
		recentFiles.insert( recentFiles.begin(), filename );
		pPref->setRecentFiles( recentFiles );

		updateRecentUsedSongList();

		h2app->setScrollStatusBarMessage( trUtf8("Song saved.") + QString(" Into: ") + filename, 2000 );
	}
}




void MainForm::action_help_about() {
	//QWidget *parent = this;
//	if (workspace) {
//		parent = workspace;
//	}

	// show modal dialog
	AboutDialog *dialog = new AboutDialog( NULL );
	dialog->exec();
}




void MainForm::showUserManual()
{
	h2app->getHelpBrowser()->hide();
	h2app->getHelpBrowser()->show();
}


void MainForm::action_file_export_pattern_as()
{
	g_engine->get_transport()->stop();

	Engine *engine = g_engine;
	int selectedpattern = engine->getSelectedPatternNumber();
	T<Song>::shared_ptr song = engine->getSong();
	T<Pattern>::shared_ptr pat = song->get_pattern_list()->get ( selectedpattern );

	T<Instrument>::shared_ptr instr = g_engine->get_sampler()->get_instrument_list()->get ( 0 );
	assert ( instr );

	QDir dir  = g_engine->get_preferences()->__lastspatternDirectory;


	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::AnyFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Pattern (*.h2pattern)" ) );
	fd->setAcceptMode ( QFileDialog::AcceptSave );
        fd->setWindowTitle ( trUtf8 ( "Save Pattern as ..." ) );
	fd->setDirectory ( dir );



	QString defaultPatternname = QString ( pat->get_name() );

	fd->selectFile ( defaultPatternname );

	LocalFileMng fileMng(g_engine);
	QString filename;
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
		QString tmpfilename = filename;
		QString toremove = tmpfilename.section( '/', -1 ); 
		QString newdatapath =  tmpfilename.replace( toremove, "" );
		g_engine->get_preferences()->__lastspatternDirectory = newdatapath;
	}

	if ( !filename.isEmpty() )
	{
		QString sNewFilename = filename;
		if(sNewFilename.endsWith( ".h2pattern" ) ){
			sNewFilename += "";
		}
		else{
			sNewFilename += ".h2pattern";
		}
		QString patternname = sNewFilename;
		QString realpatternname = filename;
		QString realname = realpatternname.mid( realpatternname.lastIndexOf( "/" ) + 1 );
		if ( realname.endsWith( ".h2pattern" ) )
			realname.replace( ".h2pattern", "" );
		pat->set_name(realname);
		CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
		int err = fileMng.savePattern ( song , selectedpattern, patternname, realname, 2 );
		if ( err != 0 )
		{
			QMessageBox::warning( this, "Composite", trUtf8("Could not export pattern.") );
			ERRORLOG ( "Error saving the pattern" );
		}
	}
	h2app->setStatusBarMessage ( trUtf8 ( "Pattern saved." ), 10000 );
	
	//update SoundlibraryPanel
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}



void MainForm::action_file_open()
{
	g_engine->get_transport()->stop();

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return;
	}

	static QString lastUsedDir = g_engine->get_preferences()->getDataDirectory() + "/songs";
	
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
	fd->setDirectory( lastUsedDir );

        fd->setWindowTitle( trUtf8( "Open song" ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
		lastUsedDir = fd->directory().absolutePath();
	}


	if ( !filename.isEmpty() ) {
		openSongFile( filename );
	}

	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();
}


void MainForm::action_file_openPattern()
{

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();

	T<Instrument>::shared_ptr instr = g_engine->get_sampler()->get_instrument_list()->get ( 0 );
	assert ( instr );

	QDir dirPattern( g_engine->get_preferences()->getDataDirectory() + "/patterns" );
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::ExistingFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Pattern (*.h2pattern)" ) );
	fd->setDirectory ( dirPattern );

	fd->setWindowTitle ( trUtf8 ( "Open Pattern" ) );


	QString filename;
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
	}
	QString patternname = filename;


	LocalFileMng mng(g_engine);
	LocalFileMng fileMng(g_engine);
	T<Pattern>::shared_ptr err = fileMng.loadPattern ( patternname );
	if ( err == 0 )
	{
		ERRORLOG( "Error loading the pattern" );
		ERRORLOG( patternname );
	}
	else
	{
		T<Pattern>::shared_ptr pNewPattern = err;
		pPatternList->add ( pNewPattern );
		song->set_modified( true );
	}

	CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
}

/// \todo parametrizzare il metodo action_file_open ed eliminare il seguente...
void MainForm::action_file_openDemo()
{
	g_engine->get_transport()->stop();

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return;
	}

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );

	fd->setWindowTitle( trUtf8( "Open song" ) );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/
	fd->setDirectory( QString( g_engine->get_preferences()->getDemoPath() ) );


	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}


	if ( !filename.isEmpty() ) {
		openSongFile( filename );
		g_engine->getSong()->set_filename( "" );
	}
}



void MainForm::showPreferencesDialog()
{
	g_engine->get_transport()->stop();
	h2app->showPreferencesDialog();
}



void MainForm::action_window_showPlaylistDialog()
{
	h2app->showPlaylistDialog();	
}



void MainForm::action_window_showMixer()
{
	bool isVisible = CompositeApp::get_instance()->getMixer()->isVisible();
	h2app->showMixer( !isVisible );
}



void MainForm::action_debug_showAudioEngineInfo()
{
	h2app->showAudioEngineInfoForm();
}



///
/// Shows the song editor
///
void MainForm::action_window_showSongEditor()
{
	bool isVisible = h2app->getSongEditorPanel()->isVisible();
	h2app->getSongEditorPanel()->setHidden( isVisible );
}



void MainForm::action_instruments_addInstrument()
{
	g_engine->lock( RIGHT_HERE );
	T<InstrumentList>::shared_ptr pList = g_engine->get_sampler()->get_instrument_list();

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pList->get_size(); ++i ) {
		T<Instrument>::shared_ptr pInstr = pList->get( i );
		if ( pInstr->get_id().toInt() > nID ) {
			nID = pInstr->get_id().toInt();
		}
	}
	++nID;

	T<Instrument>::shared_ptr pNewInstr( new Instrument(QString( nID ), "New instrument", new ADSR()) );
	pList->add( pNewInstr );
	
	#ifdef JACK_SUPPORT
	g_engine->renameJackPorts();
	#endif
	
	g_engine->unlock();

	g_engine->setSelectedInstrumentNumber( pList->get_size() - 1 );

	// Force an update
	//g_engine->get_event_queue()->pushEvent( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



void MainForm::action_instruments_clearAll()
{
	switch(
	       QMessageBox::information( this,
					 "Composite",
					 trUtf8("Clear all instruments?"),
					 trUtf8("Ok"),
					 trUtf8("Cancel"),
					 0,      // Enter == button 0
					 1 )) { // Escape == button 2
	case 0:
	  // ok btn pressed
	  break;
	case 1:
	  // cancel btn pressed
	  return;
	}

	// Remove all layers
	g_engine->lock( RIGHT_HERE );
	T<Song>::shared_ptr pSong = g_engine->getSong();
	T<InstrumentList>::shared_ptr pList = g_engine->get_sampler()->get_instrument_list();
	for (uint i = 0; i < pList->get_size(); i++) {
		T<Instrument>::shared_ptr pInstr = pList->get( i );
		pInstr->set_name( (QString( trUtf8( "Instrument %1" ) ).arg( i + 1 )) );
		// remove all layers
		for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer* pLayer = pInstr->get_layer( nLayer );
			delete pLayer;
			pInstr->set_layer( NULL, nLayer );
		}
	}
	g_engine->unlock();
	g_engine->get_event_queue()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



void MainForm::action_instruments_exportLibrary()
{
        SoundLibraryExportDialog exportDialog( this );
 	exportDialog.exec();
}




void MainForm::action_instruments_importLibrary()
{
	SoundLibraryImportDialog dialog( this );
	dialog.exec();
}



void MainForm::action_instruments_saveLibrary()
{
	SoundLibrarySaveDialog dialog( this );
	dialog.exec();
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}







///
/// Window close event
///
void MainForm::closeEvent( QCloseEvent* ev )
{
	if ( action_file_exit() == false ) {
		// don't close!!!
		ev->ignore();
		return;
	}

	ev->accept();
}



void MainForm::action_file_export()
{
	g_engine->get_transport()->stop();

	ExportSongDialog *dialog = new ExportSongDialog(this);
	dialog->exec();
	delete dialog;
}



void MainForm::action_window_showDrumkitManagerPanel()
{
	InstrumentRack *pPanel = CompositeApp::get_instance()->getInstrumentRack();
	pPanel->setHidden( pPanel->isVisible() );
}




void MainForm::closeAll() {
	// save window properties in the preferences files
	T<Preferences>::shared_ptr pref = g_engine->get_preferences();

	// mainform
	WindowProperties mainFormProp;
	mainFormProp.x = x();
	mainFormProp.y = y();
	mainFormProp.height = height();
	mainFormProp.width = width();
	pref->setMainFormProperties( mainFormProp );

	// Save mixer properties
	WindowProperties mixerProp;
	mixerProp.x = h2app->getMixer()->x();
	mixerProp.y = h2app->getMixer()->y();
	mixerProp.width = h2app->getMixer()->width();
	mixerProp.height = h2app->getMixer()->height();
	mixerProp.visible = h2app->getMixer()->isVisible();
	pref->setMixerProperties( mixerProp );

	// save pattern editor properties
	WindowProperties patternEditorProp;
	patternEditorProp.x = h2app->getPatternEditorPanel()->x();
	patternEditorProp.y = h2app->getPatternEditorPanel()->y();
	patternEditorProp.width = h2app->getPatternEditorPanel()->width();
	patternEditorProp.height = h2app->getPatternEditorPanel()->height();
	patternEditorProp.visible = h2app->getPatternEditorPanel()->isVisible();
	pref->setPatternEditorProperties( patternEditorProp );

	// save song editor properties
	WindowProperties songEditorProp;
	songEditorProp.x = h2app->getSongEditorPanel()->x();
	songEditorProp.y = h2app->getSongEditorPanel()->y();
	songEditorProp.width = h2app->getSongEditorPanel()->width();
	songEditorProp.height = h2app->getSongEditorPanel()->height();

	QSize size = h2app->getSongEditorPanel()->frameSize();
	songEditorProp.visible = h2app->getSongEditorPanel()->isVisible();
	pref->setSongEditorProperties( songEditorProp );


	// save audio engine info properties
	WindowProperties audioEngineInfoProp;
	audioEngineInfoProp.x = h2app->getAudioEngineInfoForm()->x();
	audioEngineInfoProp.y = h2app->getAudioEngineInfoForm()->y();
	audioEngineInfoProp.visible = h2app->getAudioEngineInfoForm()->isVisible();
	pref->setAudioEngineInfoProperties( audioEngineInfoProp );


#ifdef LADSPA_SUPPORT
	// save LADSPA FX window properties
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		WindowProperties prop;
		prop.x = h2app->getLadspaFXProperties(nFX)->x();
		prop.y = h2app->getLadspaFXProperties(nFX)->y();
		prop.visible= h2app->getLadspaFXProperties(nFX)->isVisible();
		pref->setLadspaProperties(nFX, prop);
	}
#endif

	m_pQApp->quit();
}



// keybindings..

void MainForm::onPlayStopAccelEvent()
{
	T<Transport>::shared_ptr xport = g_engine->get_transport();
	TransportPosition::State state = xport->get_state();
	switch (state) {
	case TransportPosition::STOPPED:
		xport->start();
		break;

	case TransportPosition::ROLLING:
		xport->stop();
		break;

	default:
		ERRORLOG( "[MainForm::onPlayStopAccelEvent()] Unhandled case." );
	}
}



void MainForm::onRestartAccelEvent()
{
	Engine* pEngine = g_engine;
	pEngine->setPatternPos( 0 );
}



void MainForm::onBPMPlusAccelEvent()
{
	Engine* pEngine = g_engine;
	g_engine->lock( RIGHT_HERE );

	T<Song>::shared_ptr pSong = pEngine->getSong();
	if (pSong->get_bpm()  < 300) {
		pEngine->setBPM( pSong->get_bpm() + 0.1 );
	}
	g_engine->unlock();
}



void MainForm::onBPMMinusAccelEvent()
{
	Engine* pEngine = g_engine;
	g_engine->lock( RIGHT_HERE );

	T<Song>::shared_ptr pSong = pEngine->getSong();
	if (pSong->get_bpm() > 40 ) {
		pEngine->setBPM( pSong->get_bpm() - 0.1 );
	}
	g_engine->unlock();
}



void MainForm::onSaveAsAccelEvent()
{
	action_file_save_as();
}



void MainForm::onSaveAccelEvent()
{
	action_file_save();
}



void MainForm::onOpenAccelEvent()
{
	action_file_open();
}



void MainForm::updateRecentUsedSongList()
{
	m_pRecentFilesMenu->clear();

	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();
	vector<QString> recentUsedSongs = pPref->getRecentFiles();

	QString sFilename;

	for ( uint i = 0; i < recentUsedSongs.size(); ++i ) {
		sFilename = recentUsedSongs[ i ];

		if ( !sFilename.isEmpty() ) {
			QAction *pAction = new QAction( this  );
			pAction->setText( sFilename );
			m_pRecentFilesMenu->addAction( pAction );
		}
	}
}



void MainForm::action_file_open_recent(QAction *pAction)
{
//	DEBUGLOG( pAction->text() );
	openSongFile( pAction->text() );
}



void MainForm::openSongFile( const QString& sFilename )
{
 	Engine *engine = g_engine;
	engine->get_transport()->stop();

	h2app->closeFXProperties();
	LocalFileMng mng(g_engine);
	T<Song>::shared_ptr pSong = Song::load( g_engine, sFilename );
	if ( pSong == NULL ) {
		QMessageBox::information( this, "Composite", trUtf8("Error loading song.") );
		return;
	}

	// add the new loaded song in the "last used song" vector
	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();
	vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), sFilename );
	pPref->setRecentFiles( recentFiles );

	h2app->setSong( pSong );

	updateRecentUsedSongList();
	engine->setSelectedPatternNumber( 0 );
}



void MainForm::initKeyInstMap()
{

	QString loc = QLocale::system().name();
	int instr = 0;

///POSIX Locale
//locale for keyboardlayout QWERTZ
// de_DE, de_AT, de_LU, de_CH, de

//locale for keyboardlayout AZERTY
// fr_BE, fr_CA, fr_FR, fr_LU, fr_CH

//locale for keyboardlayout QWERTY
// en_GB, en_US, en_ZA, usw.

	if ( loc.contains( "de" ) || loc.contains( "DE" )){ ///QWERTZ
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_M] = instr++;
	
		keycodeInstrumentMap[Qt::Key_Q] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}
	else if ( loc.contains( "fr" ) || loc.contains( "FR" )){ ///AZERTY
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_Question] = instr++;
	
		keycodeInstrumentMap[Qt::Key_A] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}else
	{ /// default QWERTY
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_M] = instr++;
	
		keycodeInstrumentMap[Qt::Key_Q] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}

	/*
	// QWERTY etc.... rows of the keyboard
	keycodeInstrumentMap[Qt::Key_Q] = instr++;
	keycodeInstrumentMap[Qt::Key_W] = instr++;
	keycodeInstrumentMap[Qt::Key_E] = instr++;
	keycodeInstrumentMap[Qt::Key_R] = instr++;
	keycodeInstrumentMap[Qt::Key_T] = instr++;
	keycodeInstrumentMap[Qt::Key_Y] = instr++;
	keycodeInstrumentMap[Qt::Key_U] = instr++;
	keycodeInstrumentMap[Qt::Key_I] = instr++;
	keycodeInstrumentMap[Qt::Key_O] = instr++;
	keycodeInstrumentMap[Qt::Key_P] = instr++;
	keycodeInstrumentMap[Qt::Key_BracketLeft] = instr++;
	keycodeInstrumentMap[Qt::Key_BracketRight] = instr++;
	keycodeInstrumentMap[Qt::Key_A] = instr++;
	keycodeInstrumentMap[Qt::Key_S] = instr++;
	keycodeInstrumentMap[Qt::Key_D] = instr++;
	keycodeInstrumentMap[Qt::Key_F] = instr++;
	keycodeInstrumentMap[Qt::Key_G] = instr++;
	keycodeInstrumentMap[Qt::Key_H] = instr++;
	keycodeInstrumentMap[Qt::Key_J] = instr++;
	keycodeInstrumentMap[Qt::Key_K] = instr++;
	keycodeInstrumentMap[Qt::Key_L] = instr++;
	keycodeInstrumentMap[Qt::Key_Semicolon] = instr++;
	keycodeInstrumentMap[Qt::Key_Apostrophe] = instr++;
	keycodeInstrumentMap[Qt::Key_Z] = instr++;
	keycodeInstrumentMap[Qt::Key_X] = instr++;
	keycodeInstrumentMap[Qt::Key_C] = instr++;
	keycodeInstrumentMap[Qt::Key_V] = instr++;
	keycodeInstrumentMap[Qt::Key_B] = instr++;
	keycodeInstrumentMap[Qt::Key_N] = instr++;
	keycodeInstrumentMap[Qt::Key_M] = instr++;
	keycodeInstrumentMap[Qt::Key_Comma] = instr++;
	keycodeInstrumentMap[Qt::Key_Period] = instr++;
*/
}



bool MainForm::eventFilter( QObject * /*o*/, QEvent *e )
{
	if ( e->type() == QEvent::KeyPress) {
		// special processing for key press
		QKeyEvent *k = (QKeyEvent *)e;
		Engine *engine = g_engine;

		// qDebug( "Got key press for instrument '%c'", k->ascii() );
		int songnumber = 0;

		switch (k->key()) {
			case Qt::Key_Space:
				onPlayStopAccelEvent();
				return TRUE; // eat event


			case Qt::Key_Comma:
				engine->handleBeatCounter();
				return TRUE; // eat even
				break;

			case Qt::Key_Backspace:
				onRestartAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Plus:
				onBPMPlusAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Minus:
				onBPMMinusAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Backslash:
				engine->onTapTempoAccelEvent();
				return TRUE; // eat event
				break;

			case  Qt::Key_S | Qt::CTRL:
				onSaveAccelEvent();
				return TRUE;
				break;
			
			case  Qt::Key_F5 :
				if( engine->get_internal_playlist().size() == 0)
					break;
				engine->get_playlist().setPrevSongPlaylist();
				songnumber = engine->get_playlist().getActiveSongNumber();
				CompositeApp::get_instance()->setScrollStatusBarMessage( trUtf8( "Playlist: Set song No. %1" ).arg( songnumber +1 ), 5000 );
				return TRUE;
				break;

			case  Qt::Key_F6 :
				if( engine->get_internal_playlist().size() == 0)
					break;
				engine->get_playlist().setNextSongPlaylist();
				songnumber = engine->get_playlist().getActiveSongNumber();
				CompositeApp::get_instance()->setScrollStatusBarMessage( trUtf8( "Playlist: Set song No. %1" ).arg( songnumber +1 ), 5000 );
				return TRUE;
				break;

			case  Qt::Key_F12 : //panic button stop all playing notes
				engine->__panic();
//				QMessageBox::information( this, "Composite", trUtf8( "Panic" ) );
				return TRUE;
				break;

			case  Qt::Key_F9 : // Qt::Key_Left do not work. Some ideas ?
				engine->setPatternPos( engine->getPatternPos() - 1 );
				return TRUE;
				break;

			case  Qt::Key_F10 : // Qt::Key_Right do not work. Some ideas ?
				engine->setPatternPos( engine->getPatternPos() + 1 );
				return TRUE;
				break;
			
			case Qt::Key_L :
				engine->togglePlaysSelected();
				QString msg = g_engine->get_preferences()->patternModePlaysSelected() ? "Single pattern mode" : "Stacked pattern mode";
				CompositeApp::get_instance()->setStatusBarMessage( msg, 5000 );
				CompositeApp::get_instance()->getSongEditorPanel()->setModeActionBtn( g_engine->get_preferences()->patternModePlaysSelected() );
				CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
				
				return TRUE;
			
		// 	QAccel *a = new QAccel( this );
// 	a->connectItem( a->insertItem(Key_S + CTRL), this, SLOT( onSaveAccelEvent() ) );
// 	a->connectItem( a->insertItem(Key_O + CTRL), this, SLOT( onOpenAccelEvent() ) );

		}

		// virtual keyboard handling
		map<int,int>::iterator found = keycodeInstrumentMap.find ( k->key() );
		if (found != keycodeInstrumentMap.end()) {
//			DEBUGLOG( "[eventFilter] virtual keyboard event" );
			// insert note at the current column in time
			// if event recording enabled
			int row = (*found).second;
			Engine* engine = g_engine;

			float velocity = 0.8;
			float pan_L = 1.0;
			float pan_R = 1.0;

			engine->addRealtimeNote (row, velocity, pan_L, pan_R);

			return TRUE; // eat event
		}
		else {
			return FALSE; // let it go
		}
        }
	else {
		return FALSE; // standard event processing
        }
}

void MainForm::action_file_export_midi()
{
	g_engine->get_transport()->stop();

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Midi file (*.mid)") );
	fd->setDirectory( QDir::homePath() );
	fd->setWindowTitle( trUtf8( "Export MIDI file" ) );
	fd->setAcceptMode( QFileDialog::AcceptSave );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QString sFilename;
	if ( fd->exec() == QDialog::Accepted ) {
		sFilename = fd->selectedFiles().first();
	}

	if ( !sFilename.isEmpty() ) {
		if ( sFilename.endsWith(".mid") == false ) {
			sFilename += ".mid";
		}

		T<Song>::shared_ptr pSong = g_engine->getSong();

		// create the Standard Midi File object
		SMFWriter *pSmfWriter = new SMFWriter();
		T<InstrumentList>::shared_ptr instrument_list =
		    g_engine->get_sampler()->get_instrument_list();
		pSmfWriter->save( sFilename, pSong, instrument_list );

		delete pSmfWriter;
	}
}



void MainForm::errorEvent( int nErrorCode )
{
	//ERRORLOG( "[errorEvent]" );

	QString msg;
	switch (nErrorCode) {
		case Engine::UNKNOWN_DRIVER:
			msg = trUtf8( "Unknown audio driver" );
			break;

		case Engine::ERROR_STARTING_DRIVER:
			msg = trUtf8( "Error starting audio driver" );
			break;

		case Engine::JACK_SERVER_SHUTDOWN:
			msg = trUtf8( "Jack driver: server shutdown" );
			break;

		case Engine::JACK_CANNOT_ACTIVATE_CLIENT:
			msg = trUtf8( "Jack driver: cannot activate client" );
			break;

		case Engine::JACK_CANNOT_CONNECT_OUTPUT_PORT:
			msg = trUtf8( "Jack driver: cannot connect output port" );
			break;

		case Engine::JACK_ERROR_IN_PORT_REGISTER:
			msg = trUtf8( "Jack driver: error in port register" );
			break;

		default:
			msg = QString( trUtf8( "Unknown error %1" ) ).arg( nErrorCode );
	}
	QMessageBox::information( this, "Composite", msg );
}


void MainForm::action_file_songProperties()
{
	SongPropertiesDialog *pDialog = new SongPropertiesDialog( this );
	if ( pDialog->exec() == QDialog::Accepted ) {
		g_engine->getSong()->set_modified( true );
	}
	delete pDialog;
}


void MainForm::action_window_showPatternEditor()
{
	bool isVisible = CompositeApp::get_instance()->getPatternEditorPanel()->isVisible();
	CompositeApp::get_instance()->getPatternEditorPanel()->setHidden( isVisible );
}

QString MainForm::getAutoSaveFilename()
{
	T<Song>::shared_ptr pSong = g_engine->getSong();
	assert( pSong );
	QString sOldFilename = pSong->get_filename();
	QString newName = "autosave.h2song";

	if ( !sOldFilename.isEmpty() ) {
		newName = sOldFilename.left( sOldFilename.length() - 7 ) + ".autosave.h2song";
	}

	return newName;
}



void MainForm::onAutoSaveTimer()
{
	//DEBUGLOG( "[onAutoSaveTimer]" );
	T<Song>::shared_ptr pSong = g_engine->getSong();
	assert( pSong );
	QString sOldFilename = pSong->get_filename();

	pSong->save( g_engine, getAutoSaveFilename() );

	pSong->set_filename(sOldFilename);

/*
	Song *pSong = h2app->getSong();
	if (pSong->getFilename() == "") {
		pSong->save( "autosave.h2song" );
		return;
	}

	action_file_save();
*/
}


void MainForm::onPlaylistDisplayTimer()
{
	if( g_engine->get_internal_playlist().size() == 0)
		return;
	int songnumber = g_engine->get_playlist().getActiveSongNumber();
	QString songname;
	if ( songnumber == -1 )
			return;

	if ( g_engine->getSong()->get_name() == "Untitled Song" ){
		songname = g_engine->getSong()->get_filename(); 
	}else
	{
		songname = g_engine->getSong()->get_name();
	}
	QString message = (trUtf8("Playlist: Song No. %1").arg( songnumber + 1)) + QString("  ---  Songname: ") + songname + QString("  ---  Author: ") + g_engine->getSong()->get_author();
	CompositeApp::get_instance()->setScrollStatusBarMessage( message, 2000 );
}

// Returns true if unsaved changes are successfully handled (saved, discarded, etc.)
// Returns false if not (i.e. Cancel)
bool MainForm::handleUnsavedChanges()
{
	bool done = false;
	bool rv = true;
	while ( !done && g_engine->getSong()->get_modified() ) {
		switch(
				QMessageBox::information( this, "Composite",
						trUtf8("\nThe document contains unsaved changes.\n"
						"Do you want to save the changes?\n"),
						trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
						0,      // Enter == button 0
						2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				// If the save fails, the __is_modified flag will still be true
			    if ( ! g_engine->getSong()->get_filename().isEmpty() ) {
					action_file_save();
				} else {
					// never been saved
					action_file_save_as();
				}
				// save
				break;
			case 1: // Discard clicked or Alt+D pressed
				// don't save but exit
				done = true;
				break;
			case 2: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				done = true;
				rv = false;
				break;
		}
	}
	return rv;
}
