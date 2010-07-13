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

#include "FLACFile.hpp"
#include <Tritium/Sample.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

#include <vector>
#include <fstream>

#ifdef FLAC_SUPPORT

//#include "FLAC/file_decoder.h"
#include <FLAC++/all.h>

namespace Tritium
{


#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT < 8
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif




/// Reads a FLAC file...not optimized yet
class FLACFile_real : public FLAC::Decoder::File
{
public:
	FLACFile_real();
	~FLACFile_real();

	void load( const QString& filename );
	T<Sample>::shared_ptr getSample();

protected:
	virtual ::FLAC__StreamDecoderWriteStatus write_callback( const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[] );
	virtual void metadata_callback( const ::FLAC__StreamMetadata *metadata );
	virtual void error_callback( ::FLAC__StreamDecoderErrorStatus status );

private:
	std::vector<float> m_audioVect_L;
	std::vector<float> m_audioVect_R;
	QString m_sFilename;
};



FLACFile_real::FLACFile_real()
{
//	infoLog( "INIT" );
}



FLACFile_real::~FLACFile_real()
{
//	infoLog( "DESTROY" );
}



::FLAC__StreamDecoderWriteStatus FLACFile_real::write_callback( const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[] )
{
	int nChannelCount = get_channels();
	int nBits = get_bits_per_sample();

	if ( ( nChannelCount != 1 ) && ( nChannelCount != 2 ) ) {
		ERRORLOG( QString( "wrong number of channels. nChannelCount=%1" ).arg( nChannelCount ) );
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	unsigned nFrames = frame->header.blocksize;

	if ( nBits == 16 ) {
		if ( nChannelCount == 1 ) {	// mono
			const FLAC__int32* data = buffer[0];

			for ( unsigned i = 0; i < nFrames; i++ ) {
				m_audioVect_L.push_back( data[i] / 32768.0 );
				m_audioVect_R.push_back( data[i] / 32768.0 );
			}
		} else {	// stereo
			const FLAC__int32* data_L = buffer[0];
			const FLAC__int32* data_R = buffer[1];

			for ( unsigned i = 0; i < nFrames; i++ ) {
				m_audioVect_L.push_back( ( float )data_L[i] / 32768.0 );
				m_audioVect_R.push_back( ( float )data_R[i] / 32768.0 );
			}
		}
	} else if ( nBits == 24 ) {
		if ( nChannelCount == 1 ) {	// mono
			const FLAC__int32* data = buffer[0];

			for ( unsigned i = 0; i < nFrames; i++ ) {
				m_audioVect_L.push_back( ( float )data[i] / 8388608.0 );
				m_audioVect_R.push_back( ( float )data[i] / 8388608.0 );
			}
		} else {	// stereo
			const FLAC__int32* data_L = buffer[0];
			const FLAC__int32* data_R = buffer[1];

			for ( unsigned i = 0; i < nFrames; i++ ) {
				m_audioVect_L.push_back( ( float )data_L[i] / 8388608.0 );
				m_audioVect_R.push_back( ( float )data_R[i] / 8388608.0 );
			}
		}
	} else {
		ERRORLOG( QString( "[write_callback] FLAC format error. nBits=%1" ).arg( nBits ) );
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



void FLACFile_real::metadata_callback( const ::FLAC__StreamMetadata * /*metadata*/ )
{
}



void FLACFile_real::error_callback( ::FLAC__StreamDecoderErrorStatus /*status*/ )
{
	ERRORLOG( "[error_callback]" );
}



void FLACFile_real::load( const QString& sFilename )
{
	m_sFilename = sFilename;

	// file exists?
	QFile check( sFilename );
	if ( !check.exists() ) {
		ERRORLOG( QString( "file %1 not found" ).arg( sFilename ) );
		return;
	} else {
		/// \todo: devo chiudere il file?
	}

	set_metadata_ignore_all();

#ifdef LEGACY_FLAC
	set_filename( sFilename.toLocal8Bit() );

	State s=init();
	if ( s != FLAC__FILE_DECODER_OK ) {
#else
	FLAC__StreamDecoderInitStatus s = init( sFilename.toLocal8Bit() );
	if ( s!=FLAC__STREAM_DECODER_INIT_STATUS_OK ) {
#endif
		ERRORLOG( "Error in init()" );
	}

#ifdef LEGACY_FLAC
	if ( process_until_end_of_file() == false ) {
		ERRORLOG( "Error in process_until_end_of_file(). Filename: " + m_sFilename );
	}
#else
	if ( process_until_end_of_stream() == false ) {
		ERRORLOG( "[load] Error in process_until_end_of_stream()" );
	}
#endif
}



T<Sample>::shared_ptr FLACFile_real::getSample() {
	//infoLog( "[getSample]" );
	T<Sample>::shared_ptr pSample;

	if ( m_audioVect_L.size() == 0 ) {
		// there were errors loading the file
		return pSample;
	}


	int nFrames = m_audioVect_L.size();
	float *data_L = new float[nFrames];
	float *data_R = new float[nFrames];

	memcpy( data_L, &m_audioVect_L[ 0 ], nFrames * sizeof( float ) );
	memcpy( data_R, &m_audioVect_R[ 0 ], nFrames * sizeof( float ) );
	pSample.reset( new Sample( nFrames, m_sFilename, get_sample_rate(), data_L, data_R ) );

	return pSample;
}

// :::::::::::::::::::::::::::::




FLACFile::FLACFile()
{
	//infoLog( "INIT" );
}


FLACFile::~FLACFile()
{
	//infoLog( "DESTROY" );
}



T<Sample>::shared_ptr FLACFile::load( const QString& sFilename ) {
	//infoLog( "[load] " + sFilename );

	FLACFile_real *pFile = new FLACFile_real();
	pFile->load( sFilename );
	T<Sample>::shared_ptr pSample = pFile->getSample();
	delete pFile;

	return pSample;
}

};

#endif // FLAC_SUPPORT

