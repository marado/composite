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
#include <QObject>

#include <Tritium/EventQueue.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Mixer.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/Logger.hpp>

#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/Sampler.hpp>

#include <Tritium/Preferences.hpp>
#include <Tritium/Action.hpp>
#include <map>

using namespace Tritium;

namespace Tritium
{
	/* Internal helper function
	 */
	static bool setAbsoluteFXLevel(
		Engine* engine,
		int nLine,
		int fx_channel,
		int fx_param);
}


/* Class Action */
Action::Action( QString s )
{
	type = s;
	QString parameter1 = "0";
	QString parameter2 = "0" ;
}


/* Class ActionManager */

ActionManager::ActionManager(Engine* parent) :
    m_engine(parent)
{
	assert(parent);
	actionList <<""
	<< "PLAY" 
	<< "PLAY_TOGGLE"
	<< "STOP"
	<< "PAUSE"
	<< "MUTE"
	<< "UNMUTE"
	<< "MUTE_TOGGLE"
	<< ">>_NEXT_BAR"
	<< "<<_PREVIOUS_BAR"
	<< "BPM_INCR"
	<< "BPM_DECR"
	<< "BPM_CC_RELATIVE"
	<< "MASTER_VOLUME_RELATIVE"
	<< "MASTER_VOLUME_ABSOLUTE"
	<< "STRIP_VOLUME_RELATIVE"
	<< "STRIP_VOLUME_ABSOLUTE"
	<< "EFFECT1_LEVEL_RELATIVE"
	<< "EFFECT2_LEVEL_RELATIVE"
	<< "EFFECT3_LEVEL_RELATIVE"
	<< "EFFECT4_LEVEL_RELATIVE"
	<< "EFFECT1_LEVEL_ABSOLUTE"
	<< "EFFECT2_LEVEL_ABSOLUTE"
	<< "EFFECT3_LEVEL_ABSOLUTE"
	<< "EFFECT4_LEVEL_ABSOLUTE"
	<< "SELECT_NEXT_PATTERN"
	<< "PAN_RELATIVE"
	<< "PAN_ABSOULTE"
	<< "BEATCOUNTER"
	<< "TAP_TEMPO";

	eventList << ""
	<< "MMC_PLAY"
	<< "MMC_DEFERRED_PLAY"
	<< "MMC_STOP"
	<< "MMC_FAST_FORWARD"
	<< "MMC_REWIND"
	<< "MMC_RECORD_STROBE"
	<< "MMC_RECORD_EXIT"
	<< "MMC_PAUSE"
	<< "NOTE"
	<< "CC";
}


ActionManager::~ActionManager()
{
}

static bool Tritium::setAbsoluteFXLevel(
	Engine* engine,
	int nLine,
	int fx_channel,
	int fx_param)
{
	//helper function to set fx levels
			
	engine->setSelectedInstrumentNumber( nLine );

	T<Song>::shared_ptr song = engine->getSong();
	T<InstrumentList>::shared_ptr instrList = engine->get_sampler()->get_instrument_list();
	T<Instrument>::shared_ptr instr = instrList->get( nLine );
	if ( instr == NULL) return false;

	T<Tritium::Mixer::Channel>::shared_ptr chan = engine->get_mixer()->channel(nLine);
	if( chan && fx_param != 0 && fx_param < chan->send_count() ){
	    chan->send_gain(fx_channel, ((float)(fx_param/127.0)));
	} else {
	    chan->send_gain(fx_channel, 0.0f);
	}
		
	engine->setSelectedInstrumentNumber(nLine);
	
	return true;

}

bool ActionManager::handleAction( Action * pAction ){

	Engine *pEngine = m_engine;

	/* 
		return false if action is null 
		(for example if no Action exists for an event)
	*/
	if( pAction == NULL )	return false;

	QString sActionString = pAction->getType();

	
	if( sActionString == "PLAY" )
	{
		int nState = pEngine->getState();
		if ( nState == Engine::StateReady ){
			pEngine->get_transport()->start();
		}
		return true;
	}

	if( sActionString == "PLAY_TOGGLE" )
	{
		T<Transport>::shared_ptr xport = pEngine->get_transport();
		TransportPosition::State state = xport->get_state();
		switch ( state ) 
		{
		case TransportPosition::STOPPED:
			xport->start();
			break;

		case TransportPosition::ROLLING:
			xport->stop();
			break;

		default:
			ERRORLOG( "[Engine::ActionManager(PLAY): Unhandled case" );
		}

		return true;
	}

	if( sActionString == "PAUSE" )
	{	
		pEngine->sequencer_stop();
		return true;
	}

	if( sActionString == "STOP" )
	{	
		pEngine->sequencer_stop();
		pEngine->setPatternPos( 0 );
		return true;
	}

	if( sActionString == "MUTE" ){
		//mutes the master, not a single strip
		pEngine->getSong()->set_mute(true);
		return true;
	}

	if( sActionString == "UNMUTE" ){
		pEngine->getSong()->set_mute(false);
		return true;
	}

	if( sActionString == "MUTE_TOGGLE" ){
		pEngine->getSong()->set_mute( !pEngine->getSong()->get_mute() );
		return true;
	}

	if( sActionString == "BEATCOUNTER" ){
		pEngine->handleBeatCounter();
		return true;
	}

	if( sActionString == "TAP_TEMPO" ){
		pEngine->onTapTempoAccelEvent();
		return true;
	}



	
	if( sActionString == "EFFECT1_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( m_engine, nLine, 0 , fx_param );
	}

	if( sActionString == "EFFECT2_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( m_engine, nLine, 1 , fx_param );
	}

	if( sActionString == "EFFECT3_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( m_engine, nLine, 2 , fx_param );
	}

	if( sActionString == "EFFECT4_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( m_engine, nLine, 3 , fx_param );
	}
	

	
	

	if( sActionString == "MASTER_VOLUME_RELATIVE" ){
		//increments/decrements the volume of the whole song	
		
		bool ok;
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		Engine *engine = m_engine;
		T<Song>::shared_ptr song = engine->getSong();



		if( vol_param != 0 ){
				if ( vol_param == 1 && song->get_volume() < 1.5 ){
					song->set_volume( song->get_volume() + 0.05 );  
				}  else  {
					if( song->get_volume() >= 0.0 ){
						song->set_volume( song->get_volume() - 0.05 );
					}
				}
		} else {
			song->set_volume( 0 );
		}

	}

	

	if( sActionString == "MASTER_VOLUME_ABSOLUTE" ){
		//sets the volume of a master output to a given level (percentage)

		bool ok;
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			

		Engine *engine = m_engine;
		T<Song>::shared_ptr song = engine->getSong();


		if( vol_param != 0 ){
				song->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
		} else {
				song->set_volume( 0 );
		}

	}

	

	if( sActionString == "STRIP_VOLUME_RELATIVE" ){
		//increments/decrements the volume of one mixer strip	
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		m_engine->setSelectedInstrumentNumber( nLine );

		Engine *engine = m_engine;
		T<Song>::shared_ptr song = engine->getSong();
		T<InstrumentList>::shared_ptr instrList = engine->get_sampler()->get_instrument_list();
		T<Instrument>::shared_ptr instr = instrList->get( nLine );

		if ( instr == NULL) return 0;

		T<Tritium::Mixer::Channel>::shared_ptr chan;
		chan = engine->get_mixer()->channel(nLine);

		if( vol_param != 0 ){
				if ( vol_param == 1 ){
				    chan->gain( chan->gain() + 0.1f );
				}  else  {
				    chan->gain( chan->gain() - 0.1f );
				}
		} else {
		    chan->gain( 0.0f );
		}

		m_engine->setSelectedInstrumentNumber(nLine);
	}

	if( sActionString == "STRIP_VOLUME_ABSOLUTE" ){
		//sets the volume of a mixer strip to a given level (percentage)

		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		m_engine->setSelectedInstrumentNumber( nLine );

		Engine *engine = m_engine;
		T<Song>::shared_ptr song = engine->getSong();
		T<InstrumentList>::shared_ptr instrList = engine->get_sampler()->get_instrument_list();

		T<Instrument>::shared_ptr instr = instrList->get( nLine );

		if ( instr == NULL) return 0;

		T<Tritium::Mixer::Channel>::shared_ptr chan;
		chan = m_engine->get_mixer()->channel(nLine);

		if( vol_param != 0 ){
		    chan->gain( 1.5f * ((float)(vol_param/127.0f)) );
		} else {
		    chan->gain( 0.0f );
		}

		m_engine->setSelectedInstrumentNumber(nLine);
	}

	if( sActionString == "SELECT_NEXT_PATTERN"){
		bool ok;
		int row = pAction->getParameter1().toInt(&ok,10);
		pEngine->setSelectedPatternNumber( row );
		pEngine->sequencer_setNextPattern( row, false, true );
       		return true;
	}



	if( sActionString == "PAN_ABSOULTE" ){
		
		 // sets the absolute panning of a given mixer channel
		
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int pan_param = pAction->getParameter2().toInt(&ok,10);

		
		float pan_L;
		float pan_R;

		Engine *engine = m_engine;
		engine->setSelectedInstrumentNumber( nLine );
		T<Song>::shared_ptr song = engine->getSong();
		T<InstrumentList>::shared_ptr instrList = engine->get_sampler()->get_instrument_list();

		T<Instrument>::shared_ptr instr = instrList->get( nLine );
		
		if( instr == NULL )
			return false;
		
		pan_L = instr->get_pan_l();
		pan_R = instr->get_pan_r();

		// pan
		float fPanValue = 0.0;
		if (pan_R == 1.0) {
			fPanValue = 1.0 - (pan_L / 2.0);
		}
		else {
			fPanValue = pan_R / 2.0;
		}

		
		fPanValue = 1 * ( ((float) pan_param) / 127.0 );


		if (fPanValue >= 0.5) {
			pan_L = (1.0 - fPanValue) * 2;
			pan_R = 1.0;
		}
		else {
			pan_L = 1.0;
			pan_R = fPanValue * 2;
		}


		instr->set_pan_l( pan_L );
		instr->set_pan_r( pan_R );

		m_engine->setSelectedInstrumentNumber(nLine);

		return true;
	}

	if( sActionString == "PAN_RELATIVE" ){
		
		 // changes the panning of a given mixer channel
		// this is useful if the panning is set by a rotary control knob
		
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int pan_param = pAction->getParameter2().toInt(&ok,10);

		
		float pan_L;
		float pan_R;

		Engine *engine = m_engine;
		engine->setSelectedInstrumentNumber( nLine );
		T<Song>::shared_ptr song = engine->getSong();
		T<InstrumentList>::shared_ptr instrList = engine->get_sampler()->get_instrument_list();

		T<Instrument>::shared_ptr instr = instrList->get( nLine );
		
		if( instr == NULL )
			return false;
		
		pan_L = instr->get_pan_l();
		pan_R = instr->get_pan_r();

		// pan
		float fPanValue = 0.0;
		if (pan_R == 1.0) {
			fPanValue = 1.0 - (pan_L / 2.0);
		}
		else {
			fPanValue = pan_R / 2.0;
		}

		if( pan_param == 1 && fPanValue < 1 ){
			fPanValue += 0.05;
		}

		if( pan_param != 1 && fPanValue > 0 ){
			fPanValue -= 0.05;
		}

		if (fPanValue >= 0.5) {
			pan_L = (1.0 - fPanValue) * 2;
			pan_R = 1.0;
		}
		else {
			pan_L = 1.0;
			pan_R = fPanValue * 2;
		}


		instr->set_pan_l( pan_L );
		instr->set_pan_r( pan_R );

		m_engine->setSelectedInstrumentNumber(nLine);

		return true;
	}
	

	if( sActionString == "BPM_CC_RELATIVE" ){
		/*
		 * increments/decrements the BPM
		 * this is useful if the bpm is set by a rotary control knob
		*/

		m_engine->lock( RIGHT_HERE );

		int mult = 1;	

		//second parameter of cc command
		//this value should be 1 to decrement and something other then 1 to increment the bpm
		int cc_param = 1;

		//this Action should be triggered only by CC commands

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);
		cc_param = pAction->getParameter2().toInt(&ok,10);
			


		T<Song>::shared_ptr pSong = pEngine->getSong();


		
		if ( cc_param == 1 && pSong->get_bpm() < 300) {
			pEngine->setBPM( pSong->get_bpm() + 1*mult );
		}


		if ( cc_param != 1 && pSong->get_bpm()  > 40 ) {
			pEngine->setBPM( pSong->get_bpm() - 1*mult );
		}


		m_engine->unlock();

		return true;
	}


	if( sActionString == "BPM_INCR" ){
		m_engine->lock( RIGHT_HERE );

		int mult = 1;	

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);


		T<Song>::shared_ptr pSong = pEngine->getSong();
		if (pSong->get_bpm()  < 300) {
			pEngine->setBPM( pSong->get_bpm() + 1*mult );
		}
		m_engine->unlock();

		return true;
	}



	if( sActionString == "BPM_DECR" ){
		m_engine->lock( RIGHT_HERE );

		int mult = 1;	

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);

		T<Song>::shared_ptr pSong = pEngine->getSong();
		if (pSong->get_bpm()  > 40 ) {
			pEngine->setBPM( pSong->get_bpm() - 1*mult );
		}
		m_engine->unlock();
		
		return true;
	}

	if( sActionString == ">>_NEXT_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() +1 );
		return true;
	}

	if( sActionString == "<<_PREVIOUS_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() -1 );
		return true;
	}
	
	return false;
}
