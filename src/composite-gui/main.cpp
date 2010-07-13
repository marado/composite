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

#include <QtGui>
#include <QLibraryInfo>
#include "config.h"
#include "version.h"
#include <getopt.h>

#include "SplashScreen.hpp"
#include "CompositeApp.hpp"
#include "MainForm.hpp"

#include <Tritium/MidiMap.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/globals.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/DataPath.hpp>
#include <Tritium/H2Exception.hpp>
#include <Tritium/Playlist.hpp>
#include <Tritium/Logger.hpp>

#include <iostream>
using namespace std;

void showInfo();
void showUsage();

/**
 * Global pointer to the Tritium Engine
 */
Tritium::Engine* Tritium::g_engine;

#define HAS_ARG 1
static struct option long_opts[] = {
	{"driver", required_argument, NULL, 'd'},
	{"song", required_argument, NULL, 's'},
	{"version", 0, NULL, 'v'},
	{"nosplash", 0, NULL, 'n'},
	{"verbose", optional_argument, NULL, 'V'},
	{"help", 0, NULL, 'h'},
	{0, 0, 0, 0},
};

#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )


//
// Set the palette used in the application
//
void setPalette( QApplication *pQApp )
{
	// create the default palette
	QPalette defaultPalette;

	// A general background color.
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );

	// A general foreground color.
	defaultPalette.setColor( QPalette::Foreground, QColor( 255, 255, 255 ) );

	// Used as the background color for text entry widgets; usually white or another light color.
	defaultPalette.setColor( QPalette::Base, QColor( 88, 94, 112 ) );

	// Used as the alternate background color in views with alternating row colors
	defaultPalette.setColor( QPalette::AlternateBase, QColor( 138, 144, 162 ) );

	// The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.
	defaultPalette.setColor( QPalette::Text, QColor( 255, 255, 255 ) );

	// The general button background color. This background can be different from Background as some styles require a different background color for buttons.
	defaultPalette.setColor( QPalette::Button, QColor( 88, 94, 112 ) );

	// A foreground color used with the Button color.
	defaultPalette.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );


	// Lighter than Button color.
	defaultPalette.setColor( QPalette::Light, QColor( 138, 144, 162 ) );

	// Between Button and Light.
	defaultPalette.setColor( QPalette::Midlight, QColor( 128, 134, 152 ) );

	// Darker than Button.
	defaultPalette.setColor( QPalette::Dark, QColor( 58, 62, 72 ) );

	// Between Button and Dark.
	defaultPalette.setColor( QPalette::Mid, QColor( 81, 86, 99 ) );

	// A very dark color. By default, the shadow color is Qt::black.
	defaultPalette.setColor( QPalette::Shadow, QColor( 255, 255, 255 ) );


	// A color to indicate a selected item or the current item.
	defaultPalette.setColor( QPalette::Highlight, QColor( 116, 124, 149 ) );

	// A text color that contrasts with Highlight.
	defaultPalette.setColor( QPalette::HighlightedText, QColor( 255, 255, 255 ) );

	pQApp->setPalette( defaultPalette );
}




int main(int argc, char *argv[])
{
	try {
		// Options...
		char *cp;
		struct option *op;
		char opts[NELEM(long_opts) * 3 + 1];

		// Build up the short option QString
		cp = opts;
		for (op = long_opts; op < &long_opts[NELEM(long_opts)]; op++) {
			*cp++ = op->val;
			if (op->has_arg)
				*cp++ = ':';
			if (op->has_arg == optional_argument )
				*cp++ = ':';  // gets another one
		}

		QApplication* pQApp = new QApplication(argc, argv);

		// Deal with the options
		QString songFilename;
		bool bNoSplash = false;
		QString sSelectedDriver;
		bool showVersionOpt = false;
		const char* logLevelOpt = "Error";
		bool showHelpOpt = false;

		int c;
		for (;;) {
			c = getopt_long(argc, argv, opts, long_opts, NULL);
			if (c == -1)
				break;

			switch(c) {
				case 'd':
					sSelectedDriver = QString::fromLocal8Bit(optarg);
					break;

				case 's':
					songFilename = QString::fromLocal8Bit(optarg);
					break;

				case 'v':
					showVersionOpt = true;
					break;

				case 'V':
					if( optarg ) {
						logLevelOpt = optarg;
					} else {
						logLevelOpt = "Warning";
					}
					break;
				case 'n':
					bNoSplash = true;
					break;

				case 'h':
				case '?':
					showHelpOpt = true;
					break;
			}
		}

		if( showVersionOpt ) {
			std::cout << get_version() << std::endl;
			exit(0);
		}
		showInfo();
		if( showHelpOpt ) {
			showUsage();
			exit(0);
		}

		// Man your battle stations... this is not a drill.
		Tritium::Logger::create_instance();
		Tritium::T<Tritium::Preferences>::shared_ptr pPref( new Tritium::Preferences() );
		Tritium::Logger::get_instance()->set_logging_level( logLevelOpt );
		// See below for Tritium::Engine.


		DEBUGLOG( QString("Using QT version ") + QString( qVersion() ) );
		DEBUGLOG( "Using data path: " + Tritium::DataPath::get_data_path() );

		if (sSelectedDriver == "auto") {
			pPref->m_sAudioDriver = "Auto";
		}
		else if (sSelectedDriver == "jack") {
			pPref->m_sAudioDriver = "Jack";
		}

		QString family = pPref->getApplicationFontFamily();
		pQApp->setFont( QFont( family, pPref->getApplicationFontPointSize() ) );

		QTranslator qttor( 0 );
		QTranslator tor( 0 );
		QString sTranslationFile = QString("composite.") + QLocale::system().name();
		QString sLocale = QLocale::system().name();
		if ( sLocale != "C") {
			if (qttor.load( QString( "qt_" ) + sLocale,
				QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
				pQApp->installTranslator( &qttor );
                        else
				DEBUGLOG( QString("Warning: No Qt translation for locale %1 found.").arg(QLocale::system().name()));


			QString sTranslationPath = "data/i18n";
			QString total = sTranslationPath + "/" + sTranslationFile + ".qm";

			bool bTransOk = tor.load( total, "." );
			if ( bTransOk ) {
				DEBUGLOG( QString( "Using locale: %1/%2" ).arg( sTranslationPath ).arg( sTranslationFile ) );
			}
			else {
				sTranslationPath = Tritium::DataPath::get_data_path() + "/i18n";
				total = sTranslationPath + "/" + sTranslationFile + ".qm";
				bTransOk = tor.load( total, "." );
				if (bTransOk) {
					DEBUGLOG( "Using locale: " + sTranslationPath + "/" + sTranslationFile );
				}
				else {
					DEBUGLOG( "Warning: no locale found: " + sTranslationPath + "/" + sTranslationFile );
				}
			}
			if (tor.isEmpty()) {
				DEBUGLOG( "Warning: error loading locale: " +  total );
			}
		}
		pQApp->installTranslator( &tor );

		QString sStyle = pPref->getQTStyle();
		if ( !sStyle.isEmpty() ) {
			pQApp->setStyle( sStyle );
		}

		setPalette( pQApp );

		SplashScreen *pSplash = new SplashScreen();

		if (bNoSplash) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}

		// Engine here to honor all preferences.
		Tritium::g_engine = new Tritium::Engine(pPref);
		MainForm *pMainForm = new MainForm( pQApp, songFilename );
		pMainForm->show();
		pSplash->finish( pMainForm );

		pQApp->exec();

		delete pSplash;
		delete pMainForm;
		delete pQApp;
		// delete Tritium::g_engine; // Deleted by ~CompositeApp (via pMainForm)

		DEBUGLOG( "Quitting..." );
		cout << "\nBye..." << endl;
		pPref.reset(); // Preferences require the Logger.
		delete Tritium::Logger::get_instance();

		//	pQApp->dumpObjectTree();

	}
	catch ( const Tritium::H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
	}

	return 0;
}



/**
 * Show some information
 */
void showInfo()
{
	cout << "\nComposite " + get_version() + " [" + __DATE__ + "]  [http://gabe.is-a-geek.org/composite/]" << endl;
	cout << "Copyright 2002-2008 Alessandro Cominu" << endl;
	cout << "Copyright 2009 Gabriel Beddingfield" << endl;
//	DEBUGLOG( "Compiled modules: " + QString(COMPILED_FEATURES) << endl;

	cout << "\nComposite comes with ABSOLUTELY NO WARRANTY" << endl;
	cout << "This is free software, and you are welcome to redistribute it" << endl;
	cout << "under certain conditions. See the file COPYING for details\n" << endl;
}



/**
 * Show the correct usage
 */
void showUsage()
{
	std::cout << "Usage: composite [-v] [-h] -s file" << std::endl;
	std::cout << "   -d, --driver AUDIODRIVER - Use the selected audio driver (jack)" << std::endl;
	std::cout << "   -s, --song FILE - Load a song (*.h2song) at startup" << std::endl;
	std::cout << "   -n, --nosplash - Hide splash screen" << std::endl;
	std::cout << "   -V[Level], --verbose[=Level] - Print a lot of debugging info" << std::endl;
        std::cout << "                 Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH" << std::endl;
	std::cout << "   -v, --version - Show version info" << std::endl;
	std::cout << "   -h, --help - Show this help message" << std::endl;
}
