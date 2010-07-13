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

#include <Tritium/Logger.hpp>
#include <Tritium/IO/MidiInput.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Note.hpp>
#include <Tritium/Action.hpp>
#include <Tritium/MidiMap.hpp>
#include <Tritium/memory.hpp>
#include <cassert>

namespace Tritium
{

MidiInput::MidiInput( Engine* parent, const QString class_name ) :
	m_engine(parent),
	m_bActive( false )
{
	assert(parent);
	//DEBUGLOG( "INIT" );
	
}


MidiInput::~MidiInput()
{
	//DEBUGLOG( "DESTROY" );
}

void MidiInput::handleMidiMessage( const MidiMessage& msg )
{
	m_engine->get_event_queue()->push_event( EVENT_MIDI_ACTIVITY, -1 );

//	infoLog( "[handleMidiMessage]" );
//	infoLog( "[handleMidiMessage] channel: " + to_string( msg.m_nChannel ) );
//	infoLog( "[handleMidiMessage] val1: " + to_string( msg.m_nData1 ) );
//	infoLog( "[handleMidiMessage] val2: " + to_string( msg.m_nData2 ) );

	switch ( msg.m_type ) {
	case MidiMessage::SYSEX:
		handleSysexMessage( msg );
		break;

	case MidiMessage::NOTE_ON:
		handleNoteOnMessage( msg );
		break;

	case MidiMessage::NOTE_OFF:
		handleNoteOffMessage( msg );
		break;

	case MidiMessage::POLYPHONIC_KEY_PRESSURE:
		ERRORLOG( "POLYPHONIC_KEY_PRESSURE event not handled yet" );
		break;

	case MidiMessage::CONTROL_CHANGE:
		DEBUGLOG( QString( "[handleMidiMessage] CONTROL_CHANGE Parameter: %1, Value: %2").arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
		handleControlChangeMessage( msg );
		break;

	case MidiMessage::PROGRAM_CHANGE:
		DEBUGLOG( QString( "[handleMidiMessage] PROGRAM_CHANGE event, seting next pattern to %1" ).arg( msg.m_nData1 ) );
		m_engine->sequencer_setNextPattern(msg.m_nData1, false, false);
		break;

	case MidiMessage::CHANNEL_PRESSURE:
		ERRORLOG( "CHANNEL_PRESSURE event not handled yet" );
		break;

	case MidiMessage::PITCH_WHEEL:
		ERRORLOG( "PITCH_WHEEL event not handled yet" );
		break;

	case MidiMessage::SYSTEM_EXCLUSIVE:
		ERRORLOG( "SYSTEM_EXCLUSIVE event not handled yet" );
		break;

	case MidiMessage::START:
		DEBUGLOG( "START event" );
		m_engine->get_transport()->start();
		break;

	case MidiMessage::CONTINUE:
		ERRORLOG( "CONTINUE event not handled yet" );
		break;

	case MidiMessage::STOP:
		DEBUGLOG( "STOP event" );
		m_engine->get_transport()->stop();
		break;

	case MidiMessage::SONG_POS:
		ERRORLOG( "SONG_POS event not handled yet" );
		break;

	case MidiMessage::QUARTER_FRAME:
		DEBUGLOG( "QUARTER_FRAME event not handled yet" );
		break;

	case MidiMessage::UNKNOWN:
		ERRORLOG( "Unknown midi message" );
		break;

	default:
		ERRORLOG( QString( "unhandled midi message type: %1" ).arg( msg.m_type ) );
	}
}

void MidiInput::handleControlChangeMessage( const MidiMessage& msg )
{
	//DEBUGLOG( QString( "[handleMidiMessage] CONTROL_CHANGE Parameter: %1, Value: %2" ).arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
	
	T<ActionManager>::shared_ptr aH = m_engine->get_action_manager();
	MidiMap * mM = m_engine->get_preferences()->get_midi_map();

	Action * pAction; 

	pAction = mM->getCCAction( msg.m_nData1 );
	pAction->setParameter2( QString::number( msg.m_nData2 ) );

	aH->handleAction( pAction );

	m_engine->set_last_midi_event("CC", msg.m_nData1);
	

}

void MidiInput::handleNoteOnMessage( const MidiMessage& msg )
{
	DEBUGLOG( "handleNoteOnMessage" );


	int nMidiChannelFilter = m_engine->get_preferences()->m_nMidiChannelFilter;
	int nChannel = msg.m_nChannel;
	int nNote = msg.m_nData1;
	float fVelocity = msg.m_nData2 / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg );
		return;
	}


	bool bIsChannelValid = true;
	if ( nMidiChannelFilter != -1 ) {
		bIsChannelValid = ( nChannel == nMidiChannelFilter );
	}

	T<ActionManager>::shared_ptr aH = m_engine->get_action_manager();
	MidiMap * mM = m_engine->get_preferences()->get_midi_map();

	m_engine->set_last_midi_event("NOTE", msg.m_nData1);
	
	bool action = aH->handleAction( mM->getNoteAction( msg.m_nData1 ) );
	
	if ( action && m_engine->get_preferences()->m_bMidiDiscardNoteAfterAction)
	{
		return;
	}



	

	bool bPatternSelect = false;

	if ( bIsChannelValid ) {
		if ( bPatternSelect ) {
			int patternNumber = nNote - 36;
			DEBUGLOG( QString( "next pattern = %1" ).arg( patternNumber ) );

			m_engine->sequencer_setNextPattern( patternNumber, false, false );
		} else {
			static const float fPan_L = 1.0f;
			static const float fPan_R = 1.0f;

			int nInstrument = nNote - 36;
			if ( nInstrument < 0 ) {
				nInstrument = 0;
			}
			if ( nInstrument > ( MAX_INSTRUMENTS -1 ) ) {
				nInstrument = MAX_INSTRUMENTS - 1;
			}

			m_engine->addRealtimeNote( nInstrument, fVelocity, fPan_L, fPan_R, 0.0, true, msg.m_use_frame, msg.m_frame );
		}
	}
}



void MidiInput::handleNoteOffMessage( const MidiMessage& msg )
{
	DEBUGLOG( "handleNoteOffMessage" );
	if ( m_engine->get_preferences()->m_bMidiNoteOffIgnore ) {
		return;
	}

	T<Song>::shared_ptr pSong = m_engine->getSong();

	int nNote = msg.m_nData1;
	int nInstrument = nNote - 36;
	if ( nInstrument < 0 ) {
		nInstrument = 0;
	}
	if ( nInstrument > ( MAX_INSTRUMENTS -1 ) ) {
		nInstrument = MAX_INSTRUMENTS - 1;
	}
	T<Instrument>::shared_ptr pInstr = m_engine->get_sampler()->get_instrument_list()->get( nInstrument );
	// unsigned nPosition = 0;
	const float fVelocity = 0.0f;
	const float fPan_L = 0.5f;
	const float fPan_R = 0.5f;
	const int nLength = -1;
	const float fPitch = 0.0f;
	// XXX TO-DO: Position is ignored
	// if (msg.m_use_frame) {
	//     TransportPosition xpos;
	//     m_engine->get_transport()->get_position(&xpos);
	//     nPosition = xpos.tick_in_bar();
	// }
	Note *pNewNote = new Note( pInstr, fVelocity, fPan_L, fPan_R, nLength, fPitch );

	m_engine->midi_noteOff( pNewNote );
}



void MidiInput::handleSysexMessage( const MidiMessage& msg )
{

	/*
		General MMC message
		0	1	2	3	4	5
		F0	7F	id	6	cmd	247

		cmd:
		1	stop
		2	play
		3	Deferred play
		4	Fast Forward
		5	Rewind
		6	Record strobe (punch in)
		7	Record exit (punch out)
		9	Pause


		Goto MMC message
		0	1	2	3	4	5	6	7	8	9	10	11	12
		240	127	id	6	68	6	1	hr	mn	sc	fr	ff	247
	*/
	
	
	T<ActionManager>::shared_ptr aH = m_engine->get_action_manager();
	MidiMap * mM = m_engine->get_preferences()->get_midi_map();

	m_engine->set_last_midi_event("SYSEX", msg.m_nData1);



if ( msg.m_sysexData.size() == 6 ) {
		if (
		    ( msg.m_sysexData[0] == 0xF0 ) &&
		    ( msg.m_sysexData[1] == 127 ) &&
		    ( msg.m_sysexData[2] == 127 ) &&
		    ( msg.m_sysexData[3] == 6 ) ) {

			
			switch ( msg.m_sysexData[4] ) {

			case 1:	// STOP
			{ 
				m_engine->set_last_midi_event("MMC_STOP");
				aH->handleAction(mM->getMMCAction("MMC_STOP"));
				break;
			}

			case 2:	// PLAY
			{
				m_engine->set_last_midi_event("MMC_PLAY");
				aH->handleAction(mM->getMMCAction("MMC_PLAY"));
				break;
			}

			case 3:	//DEFERRED PLAY
			{
				m_engine->set_last_midi_event("MMC_PLAY");
				aH->handleAction(mM->getMMCAction("MMC_PLAY"));
				break;
			}

			case 4:	// FAST FWD
				m_engine->set_last_midi_event("MMC_FAST_FWD");
				aH->handleAction(mM->getMMCAction("MMC_FAST_FWD"));
				
				break;

			case 5:	// REWIND
				m_engine->set_last_midi_event("MMC_REWIND");
				aH->handleAction(mM->getMMCAction("MMC_REWIND"));
				break;

			case 6:	// RECORD STROBE (PUNCH IN)
				m_engine->set_last_midi_event("MMC_RECORD_STROBE");
				aH->handleAction(mM->getMMCAction("MMC_RECORD_STROBE"));
				break;

			case 7:	// RECORD EXIT (PUNCH OUT)
				m_engine->set_last_midi_event("MMC_RECORD_EXIT");
				aH->handleAction(mM->getMMCAction("MMC_RECORD_EXIT"));
				break;

			case 9:	//PAUSE
				m_engine->set_last_midi_event("MMC_PAUSE");
				aH->handleAction(mM->getMMCAction("MMC_PAUSE"));
				break;

			default:
				DEBUGLOG( "Unknown MMC Command" );
//					midiDump( buf, nBytes );
			}
		}
	} else if ( msg.m_sysexData.size() == 13 ) {
		ERRORLOG( "MMC GOTO Message not implemented yet" );
//		midiDump( buf, nBytes );
		//int id = buf[2];
		int hr = msg.m_sysexData[7];
		int mn = msg.m_sysexData[8];
		int sc = msg.m_sysexData[9];
		int fr = msg.m_sysexData[10];
		int ff = msg.m_sysexData[11];
		DEBUGLOG( QString("[handleSysexMessage] GOTO %1:%2:%3:%4:%5")
			 .arg(hr).arg(mn).arg(sc).arg(fr).arg(ff) );
	} else {
		// sysex dump
		QString sDump;
		for ( int i = 0; i < ( int )msg.m_sysexData.size(); ++i ) {
			QString hx = QString::number((unsigned)msg.m_sysexData[i], 16);
			sDump += QString("%1 ").arg( hx, 2, '0');
		}
		DEBUGLOG( QString( "Unknown SysEx message: (%1) [%2]" ).arg( msg.m_sysexData.size() ).arg( sDump ) );
	}
}

int MidiInput::processAudio(uint32_t /*nframes*/)
{
    return 0;
}

int MidiInput::processNonAudio(uint32_t /*nframes*/)
{
    return 0;
}


};

