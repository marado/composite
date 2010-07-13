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
#include "DiskWriterDriver.hpp"

#include <Tritium/Logger.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/TransportPosition.hpp>

#include <QThread>
#include <cassert>

namespace Tritium
{

class DiskWriterDriverThread : public QThread
{
	bool m_abort;
	DiskWriterDriver* pDriver;
public:
	DiskWriterDriverThread(DiskWriterDriver* d) :
		m_abort(false),
		pDriver(d)
		{}
	void shutdown() { m_abort = true; }
	void run();
};

DiskWriterDriverThread * diskWriterDriverThread;

void DiskWriterDriverThread::run()
{
	DEBUGLOG( "DiskWriterDriver thread start" );
	Engine* engine = pDriver->get_engine();
        T<Transport>::shared_ptr xport = engine->get_transport();
        TransportPosition xpos;

	// always rolling, no user interaction
        xport->locate(0);
        xport->start();

	SF_INFO soundInfo;
	soundInfo.samplerate = pDriver->m_nSampleRate;
//	soundInfo.frames = -1;//getNFrames();		///\todo: da terminare
	soundInfo.channels = 2;
	soundInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	if ( !sf_format_check( &soundInfo ) ) {
		ERRORLOG( "Error in soundInfo" );
	}

	SNDFILE* m_file = sf_open( pDriver->m_sFilename.toLocal8Bit(), SFM_WRITE, &soundInfo );

	float *pData = new float[ pDriver->m_nBufferSize * 2 ];	// always stereo

	float *pData_L = pDriver->m_pOut_L;
	float *pData_R = pDriver->m_pOut_R;

        uint32_t report_interval = pDriver->m_nBufferSize * 64;

	while ( (!m_abort)
		&& (pDriver->m_processCallback( pDriver->m_nBufferSize, pDriver->m_processCallback_arg ) == 0) ) {
		// process...
		for ( unsigned i = 0; i < pDriver->m_nBufferSize; i++ ) {
			if(pData_L[i] > 1){
				pData[i * 2] = 1;
			}
			else if(pData_L[i] < -1){
				pData[i * 2] = -1;
			}else
			{ 
				pData[i * 2] = pData_L[i];
			}

			if(pData_R[i] > 1){
				pData[i * 2 + 1] = 1;
			}
			else if(pData_R[i] < -1){
				pData[i * 2 + 1] = -1;
			}else
			{ 
				pData[i * 2 + 1] = pData_R[i];
			}
		}
		int res = sf_writef_float( m_file, pData, pDriver->m_nBufferSize );
		if ( res != ( int )pDriver->m_nBufferSize ) {
			ERRORLOG( "Error during sf_write_float" );
		}

                // Since we're calling the position AFTER the process cycle, this
                // position actually refers to the *next* process cycle.
                xport->get_position(&xpos);
		if ( (xpos.frame % report_interval) == 0 ) {
			int nPatterns = engine->getSong()->get_pattern_group_vector()->size();
			int nCurrentPatternPos = engine->getPatternPos();
			assert( nCurrentPatternPos != -1 );

			float fPercent = ( float ) nCurrentPatternPos / ( float )nPatterns * 100.0;
			engine->get_event_queue()->push_event( EVENT_PROGRESS, ( int )fPercent );
			DEBUGLOG( QString( "DiskWriterDriver: %1%, transport frames:%2" ).arg( fPercent ).arg( xpos.frame ) );
		}
	}
	engine->get_event_queue()->push_event( EVENT_PROGRESS, 100 );

	delete[] pData;
	pData = NULL;

	sf_close( m_file );

	DEBUGLOG( "DiskWriterDriver thread end" );

}




DiskWriterDriver::DiskWriterDriver(
	Engine* parent,
	audioProcessCallback processCallback,
	void* arg,
	unsigned nSamplerate,
	const QString& sFilename )
		: AudioOutput(parent)
		, m_nSampleRate( nSamplerate )
		, m_sFilename( sFilename )
		, m_processCallback( processCallback )
		, m_processCallback_arg( arg )
{
	DEBUGLOG( "INIT" );
	assert(parent);
}



DiskWriterDriver::~DiskWriterDriver()
{
	DEBUGLOG( "DESTROY" );
}



int DiskWriterDriver::init( unsigned nBufferSize )
{
	DEBUGLOG( QString( "Init, %1 samples" ).arg( nBufferSize ) );

	m_nBufferSize = nBufferSize;
	m_pOut_L = new float[nBufferSize];
	m_pOut_R = new float[nBufferSize];

	return 0;
}



///
/// Connect
/// return 0: Ok
///
int DiskWriterDriver::connect()
{
	DEBUGLOG( "[connect]" );

	diskWriterDriverThread = new DiskWriterDriverThread(this);
	diskWriterDriverThread->start();

	return 0;
}



/// disconnect
void DiskWriterDriver::disconnect()
{
	DEBUGLOG( "[disconnect]" );

	diskWriterDriverThread->shutdown();
	diskWriterDriverThread->wait();
	delete diskWriterDriverThread;

	delete[] m_pOut_L;
	m_pOut_L = NULL;

	delete[] m_pOut_R;
	m_pOut_R = NULL;
}



unsigned DiskWriterDriver::getSampleRate()
{
	return m_nSampleRate;
}

Engine* DiskWriterDriver::get_engine()
{
	return m_engine;
}


};
