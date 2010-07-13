/*
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * This file is part of Tritium
 *
 * Tritium is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Tritium is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QDir>

#include <Tritium/SoundLibrary.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentLayer.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Sample.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/LocalFileMng.hpp>
#include <Tritium/H2Exception.hpp>
#include <Tritium/EngineInterface.hpp>
#include <Tritium/ADSR.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

#include <cstdlib>

#ifdef LIBARCHIVE_SUPPORT
#include <archive.h>		// used for drumkit install
#include <archive_entry.h>	// used for drumkit install
#else //use libtar
	//disabled libtar on windows
	#ifndef WIN32
		#include <zlib.h>       // used for drumkit install
		#include <libtar.h>     // used for drumkit install
	#endif
#endif

#include <fcntl.h>
#include <errno.h>

namespace Tritium
{

SoundLibrary::SoundLibrary()
{
}



SoundLibrary::~SoundLibrary()
{
}



// ::::::::::



Drumkit::Drumkit()
{
}



Drumkit::~Drumkit()
{
}




T<Drumkit>::shared_ptr Drumkit::load( EngineInterface* eng, const QString& sFilename )
{
	LocalFileMng mng(eng);
	T<Drumkit>::shared_ptr rv( mng.loadDrumkit( sFilename ) );
	return rv;
}



std::vector<QString> Drumkit::getUserDrumkitList(EngineInterface* eng)
{
	LocalFileMng mng(eng);
	return mng.getUserDrumkitList();
}



std::vector<QString> Drumkit::getSystemDrumkitList(EngineInterface* eng)
{
	LocalFileMng mng(eng);
	return mng.getSystemDrumkitList();
}



void Drumkit::dump()
{
	DEBUGLOG( "Drumkit dump" );
	DEBUGLOG( "\t|- Name = " + m_sName );
	DEBUGLOG( "\t|- Author = " + m_sAuthor );
	DEBUGLOG( "\t|- Info = " + m_sInfo );

	DEBUGLOG( "\t|- Instrument list" );
	for ( unsigned nInstrument = 0; nInstrument < m_pInstrumentList->get_size(); ++nInstrument ) {
		T<Instrument>::shared_ptr pInstr = m_pInstrumentList->get( nInstrument );
		DEBUGLOG( QString("\t\t|- (%1 of %2) Name = %3")
			 .arg( nInstrument )
			 .arg( m_pInstrumentList->get_size() )
			 .arg( pInstr->get_name() )
			);
		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
			InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
			if ( pLayer ) {
				T<Sample>::shared_ptr pSample = pLayer->get_sample();
				if ( pSample ) {
					DEBUGLOG( "\t\t   |- " + pSample->get_filename() );
				} else {
					DEBUGLOG( "\t\t   |- NULL sample" );
				}
			} else {
				DEBUGLOG( "\t\t   |- NULL Layer" );
			}

		}
//		cout << "\t\t" << i << " - " << instr->getSample()->getFilename() << endl;
	}
}

#ifdef LIBARCHIVE_SUPPORT
void Drumkit::install( EngineInterface* engine, const QString& filename )
{
	DEBUGLOG( "[Drumkit::install] drumkit = " + filename );
	QString dataDir = engine->get_preferences()->getDataDirectory() + "drumkits/";

	int r;
	struct archive *drumkitFile;
	struct archive_entry *entry;
	char newpath[1024];

	drumkitFile = archive_read_new();
	archive_read_support_compression_all(drumkitFile);
	archive_read_support_format_all(drumkitFile);
	if (( r = archive_read_open_file(drumkitFile, filename.toLocal8Bit(), 10240) )) {
		ERRORLOG( QString( "Error: %2, Could not open drumkit: %1" )
			.arg( archive_errno(drumkitFile))
			.arg( archive_error_string(drumkitFile)) );
		archive_read_close(drumkitFile);
		archive_read_finish(drumkitFile);
		return;
	}
	while ( (r = archive_read_next_header(drumkitFile, &entry)) != ARCHIVE_EOF) {
		if (r != ARCHIVE_OK) {
			ERRORLOG( QString( "Error reading drumkit file: %1")
				.arg(archive_error_string(drumkitFile)));
			break;
		}

		// insert data directory prefix
		QString np = dataDir + archive_entry_pathname(entry);
		strcpy( newpath, np.toLocal8Bit() );
		archive_entry_set_pathname(entry, newpath);

		// extract tarball
		r = archive_read_extract(drumkitFile, entry, 0);
		if (r == ARCHIVE_WARN) {
			WARNINGLOG( QString( "warning while extracting %1 (%2)").arg(filename).arg(archive_error_string(drumkitFile)));
		} else if (r != ARCHIVE_OK) {
			ERRORLOG( QString( "error while extracting %1 (%2)").arg(filename).arg(archive_error_string(drumkitFile)));
			break;
		}
	}
	archive_read_close(drumkitFile);
	archive_read_finish(drumkitFile);
}
#else //use libtar

#ifndef WIN32
void Drumkit::install( EngineInterface* engine, const QString& filename )
{
        DEBUGLOG( "[Drumkit::install] drumkit = " + filename );
        QString dataDir = engine->get_preferences()->getDataDirectory() + "drumkits/";

        // GUNZIP !!!
        QString gunzippedName = filename.left( filename.indexOf( "." ) );
        gunzippedName += ".tar";
        FILE *pGunzippedFile = fopen( gunzippedName.toLocal8Bit(), "wb" );
        gzFile gzipFile = gzopen( filename.toLocal8Bit(), "rb" );
	if ( !gzipFile ) {	
		throw H2Exception( "Error opening gzip file" );
	}

        uchar buf[4096];
        while ( gzread( gzipFile, buf, 4096 ) > 0 ) {
                fwrite( buf, sizeof( uchar ), 4096, pGunzippedFile );
        }
        gzclose( gzipFile );
        fclose( pGunzippedFile );


        // UNTAR !!!
        TAR *tarFile;

        char tarfilename[1024];
        strcpy( tarfilename, gunzippedName.toLocal8Bit() );

        if ( tar_open( &tarFile, tarfilename, NULL, O_RDONLY, 0, TAR_VERBOSE | TAR_GNU ) == -1 ) { 
		ERRORLOG( QString( "[Drumkit::install] tar_open(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
		return;
        }

        char destDir[1024];
        strcpy( destDir, dataDir.toLocal8Bit() );
        if ( tar_extract_all( tarFile, destDir ) != 0 ) {
                ERRORLOG( QString( "[Drumkit::install] tar_extract_all(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
        }

        if ( tar_close( tarFile ) != 0 ) {
                ERRORLOG( QString( "[Drumkit::install] tar_close(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
        }
}
#endif

#endif

void Drumkit::save( EngineInterface* engine, const QString& sName, const QString& sAuthor, const QString& sInfo, const QString& sLicense )
{
	DEBUGLOG( "Saving drumkit" );

	T<Drumkit>::shared_ptr pDrumkitInfo( new Drumkit );
	pDrumkitInfo->setName( sName );
	pDrumkitInfo->setAuthor( sAuthor );
	pDrumkitInfo->setInfo( sInfo );
	pDrumkitInfo->setLicense( sLicense );

	T<InstrumentList>::shared_ptr pSongInstrList = engine->get_sampler()->get_instrument_list();
	T<InstrumentList>::shared_ptr pInstrumentList( new InstrumentList() );

	for ( uint nInstrument = 0; nInstrument < pSongInstrList->get_size(); nInstrument++ ) {
		T<Instrument>::shared_ptr pOldInstr = pSongInstrList->get( nInstrument );
		T<Instrument>::shared_ptr pNewInstr( new Instrument( pOldInstr->get_id(), pOldInstr->get_name(), new ADSR( *( pOldInstr->get_adsr() ) ) ) );
		pNewInstr->set_gain( pOldInstr->get_gain() );
		pNewInstr->set_pan_l( pOldInstr->get_pan_l() );
		pNewInstr->set_pan_r( pOldInstr->get_pan_r() );
		pNewInstr->set_muted( pOldInstr->is_muted() );
		pNewInstr->set_random_pitch_factor( pOldInstr->get_random_pitch_factor() );
		pNewInstr->set_mute_group( pOldInstr->get_mute_group() );

		pNewInstr->set_filter_active( pOldInstr->is_filter_active() );
		pNewInstr->set_filter_cutoff( pOldInstr->get_filter_cutoff() );
		pNewInstr->set_filter_resonance( pOldInstr->get_filter_resonance() );


		QString sInstrDrumkit = pOldInstr->get_drumkit_name();

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pOldLayer = pOldInstr->get_layer( nLayer );
			if ( pOldLayer ) {
				T<Sample>::shared_ptr pSample = pOldLayer->get_sample();
				// Following is not a real sample, it contains only the filename information
				T<Sample>::shared_ptr pNewSample( new Sample( 0, pSample->get_filename(), 0 ) );
				InstrumentLayer *pLayer = new InstrumentLayer( pNewSample );
				pLayer->set_gain( pOldLayer->get_gain() );
				pLayer->set_pitch( pOldLayer->get_pitch() );
				pLayer->set_velocity_range( pOldLayer->get_velocity_range() );

				pNewInstr->set_layer( pLayer, nLayer );
			} else {
				pNewInstr->set_layer( NULL, nLayer );
			}
		}
		pInstrumentList->add ( pNewInstr );
	}

	pDrumkitInfo->setInstrumentList( pInstrumentList );

	T<Mixer>::shared_ptr mixer = engine->get_mixer();
	for( uint nChannel = 0 ; mixer->count() ; ++nChannel ) {
	    pDrumkitInfo->channels().push_back( mixer->channel(nChannel) );
	}

	if( pInstrumentList->get_size() != pDrumkitInfo->channels().size() ) {
	    ERRORLOG( QString("When saving drumkit, instrument and channel counts "
			      "did not match (%1 and %2)")
		      .arg(pInstrumentList->get_size())
		      .arg(pDrumkitInfo->channels().size())
		);
	}

	LocalFileMng fileMng(engine);
	int err = fileMng.saveDrumkit( pDrumkitInfo );
	if ( err != 0 ) {
		ERRORLOG( "Error saving the drumkit" );
		throw H2Exception( "Error saving the drumkit" );
	}

	// delete the drumkit info
	pDrumkitInfo.reset();
}




void Drumkit::removeDrumkit( EngineInterface* engine, const QString& sDrumkitName )
{
	DEBUGLOG( "Removing drumkit: " + sDrumkitName );

	QString dataDir = engine->get_preferences()->getDataDirectory() + "drumkits/";
	dataDir += sDrumkitName;
	QString cmd = QString( "rm -rf \"" ) + dataDir + "\"";
	DEBUGLOG( cmd );
	if ( system( cmd.toLocal8Bit() ) != 0 ) {
		ERRORLOG( "Error executing '" + cmd + "'" );
		throw H2Exception( QString( "Error executing '%1'" ).arg( cmd ) );
	}
}

};

