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

#ifndef COMPOSITE_MAINFORM_HPP
#define COMPOSITE_MAINFORM_HPP

#include <QtNetwork>
#include <QtGui>

#include <map>

#include "EventListener.hpp"
#include "config.h"

class CompositeApp;

///
/// Main window
///
class MainForm : public QMainWindow, public EventListener
{
	Q_OBJECT

	public:
		QApplication* m_pQApp;

		MainForm( QApplication *app, const QString& songFilename );
		~MainForm();

		void updateRecentUsedSongList();

		virtual void errorEvent( int nErrorCode );

	public slots:
		void showPreferencesDialog();
		void showUserManual();

		void action_file_new();
		void action_file_open();
		void action_file_openDemo();
		void action_file_save();
		void action_file_save_as();
		void action_file_openPattern();
		void action_file_export_pattern_as();
		bool action_file_exit();

		void action_file_export();
		void action_file_export_midi();
		void action_file_songProperties();

		void action_help_about();

		void action_instruments_addInstrument();
		void action_instruments_clearAll();
		void action_instruments_saveLibrary();
		void action_instruments_exportLibrary();
		void action_instruments_importLibrary();


		void action_window_showMixer();
		void action_window_showPlaylistDialog();
		void action_window_showSongEditor();
		void action_window_showPatternEditor();
		void action_window_showDrumkitManagerPanel();

		void action_debug_showAudioEngineInfo();

		void closeEvent( QCloseEvent* ev );

		void onPlayStopAccelEvent();
		void onRestartAccelEvent();
		void onBPMPlusAccelEvent();
		void onBPMMinusAccelEvent();
		void onSaveAsAccelEvent();
		void onSaveAccelEvent();
		void onOpenAccelEvent();

		void action_file_open_recent( QAction *pAction );

	private slots:
		void onAutoSaveTimer();
		void onPlaylistDisplayTimer();

	protected:
		// Returns true if handled, false if aborted.
		bool handleUnsavedChanges();

	private:
		CompositeApp* h2app;

		QMenu *m_pRecentFilesMenu;
		QAction *m_pRecentFileAction0;
		QAction *m_pRecentFileAction1;
		QAction *m_pRecentFileAction2;
		QAction *m_pRecentFileAction3;
		QAction *m_pRecentFileAction4;

		QHttp m_http;

		QTimer m_autosaveTimer;

		/** Create the menubar */
		void createMenuBar();

		void closeAll();
		void openSongFile( const QString& sFilename );

		bool eventFilter( QObject *o, QEvent *e );

		std::map<int,int>  keycodeInstrumentMap;
		void initKeyInstMap();

		QString getAutoSaveFilename();

};

#endif // COMPOSITE_MAINFORM_HPP
