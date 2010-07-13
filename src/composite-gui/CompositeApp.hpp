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

#ifndef COMPOSITE_COMPOSITEAPP_HPP
#define COMPOSITE_COMPOSITEAPP_HPP

#include "config.h"

#include <Tritium/globals.hpp>
#include <Tritium/memory.hpp>

#include "EventListener.hpp"

#include <iostream>
#include <vector>
#include <QtGui>

namespace Tritium
{
	class Song;
}

class SongEditorPanel;
class MainForm;
class PlayerControl;
class PatternEditorPanel;
class InstrumentEditorPanel;
class SongEditor;
class Mixer;
class AudioEngineInfoForm;
class SimpleHTMLBrowser;
class LadspaFXProperties;
class LadspaFXInfo;
class LadspaFXGroup;
class InstrumentRack;
class PlaylistDialog;
class AppPlaylistListener;
//class AudioFileBrowser;

class CompositeApp : public QObject
{
	Q_OBJECT
	public:
		CompositeApp(
			MainForm* pMainForm,
			Tritium::T<Tritium::Song>::shared_ptr pFirstSong
			);

		/// Returns the instance of CompositeApp class
		static CompositeApp* get_instance();

		virtual ~CompositeApp();

		void setSong( Tritium::T<Tritium::Song>::shared_ptr pSong );

		void showPreferencesDialog();
		void showMixer(bool bShow);
		void showAudioEngineInfoForm();
		void showPlaylistDialog();
//		void showAudioFileBrowser();

		::Mixer* getMixer() {	return m_pMixer;	}
		MainForm* getMainForm() {	return m_pMainForm;	}
		SongEditorPanel* getSongEditorPanel() {	return m_pSongEditorPanel;	}
		AudioEngineInfoForm* getAudioEngineInfoForm() {	return m_pAudioEngineInfoForm;	}
		PlaylistDialog* getPlayListDialog() {	return m_pPlaylistDialog;	}
//		AudioFileBrowser* getAudioFileBrowser() {  return m_pAudioFileBrowser;	}
		SimpleHTMLBrowser* getHelpBrowser() {	return m_pHelpBrowser;	}
		PatternEditorPanel* getPatternEditorPanel() {	return m_pPatternEditorPanel;	}
		PlayerControl* getPlayerControl() {	return m_pPlayerControl;	}
		InstrumentRack* getInstrumentRack(){	return m_pInstrumentRack;	}


		void setStatusBarMessage( const QString& msg, int msec = 0 );
		void setScrollStatusBarMessage( const QString& msg, int msec = 0, bool test = true );
                void setWindowTitle( const QString& title);

#ifdef LADSPA_SUPPORT
		LadspaFXProperties* getLadspaFXProperties(uint nFX) {	return m_pLadspaFXProperties[nFX];	}
#endif
		void addEventListener( EventListener* pListener );
		void removeEventListener( EventListener* pListener );
		void closeFXProperties();

		void onDrumkitLoad( QString name );

	public slots:
		void onEventQueueTimer();

	private:
		static CompositeApp *m_pInstance;	///< CompositeApp instance

#ifdef LADSPA_SUPPORT
		LadspaFXProperties *m_pLadspaFXProperties[MAX_FX];
#endif

		MainForm *m_pMainForm;
		::Mixer *m_pMixer;
		PatternEditorPanel* m_pPatternEditorPanel;
		AudioEngineInfoForm *m_pAudioEngineInfoForm;
		SongEditorPanel *m_pSongEditorPanel;
		SimpleHTMLBrowser *m_pHelpBrowser;
		SimpleHTMLBrowser *m_pFirstTimeInfo;
		InstrumentRack* m_pInstrumentRack;
		PlayerControl *m_pPlayerControl;
		PlaylistDialog *m_pPlaylistDialog;
		AppPlaylistListener *m_pAppPlaylistListener;
//		AudioFileBrowser *m_pAudioFileBrowser;

		QTimer *m_pEventQueueTimer;
		std::vector<EventListener*> m_eventListeners;

		// implement EngineListener interface
		void engineError(uint nErrorCode);

		//void setupTopLevelInterface();
		void setupSinglePanedInterface();
		void showInfoSplash();
};


#endif // COMPOSITE_COMPOSITEAPP_HPP
