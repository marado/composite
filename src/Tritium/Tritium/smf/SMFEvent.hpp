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

#ifndef TRITIUM_SMFEVENT_HPP
#define TRITIUM_SMFEVENT_HPP

#include <vector>

namespace Tritium
{

class SMFBuffer
{
public:
	std::vector<char> getBuffer() {
		return m_buffer;
	}

	void writeByte( short int nByte );
	void writeWord( int nVal );
	void writeDWord( long nVal );
	void writeString( const QString& sMsg );
	void writeVarLen( long nVal );

	std::vector<char> m_buffer;

	SMFBuffer() { }
};



enum SMFEventType {
	NOTE_OFF = 128,
	NOTE_ON = 144
};



enum SMFMetaEventType {
	SEQUENCE_NUMBER = 0,
	TEXT_EVENT,
	COPYRIGHT_NOTICE,
	TRACK_NAME,
	INSTRUMENT_NAME,
	LYRIC,
	MARKER,
	CUE_POINT,
	END_OF_TRACK = 0x2f,
	SET_TEMPO = 0x51,
	TIME_SIGNATURE = 0x58,
	KEY_SIGNATURE
};


class SMFBase
{
public:
	virtual ~SMFBase() {}
	virtual std::vector<char> getBuffer() = 0;
};



class SMFEvent : public SMFBase
{
public:
	SMFEvent( const QString& sEventName, unsigned nTicks );
	virtual ~SMFEvent();

	int m_nTicks;
	int m_nDeltaTime;
};



class SMFTrackNameMetaEvent : public SMFEvent
{
public:
	SMFTrackNameMetaEvent( const QString& sTrackName, unsigned nDeltaTime );

	virtual std::vector<char> getBuffer();

private:
	QString m_sTrackName;

};



class SMFNoteOnEvent : public SMFEvent
{
public:
	SMFNoteOnEvent( unsigned nTicks, int nChannel, int nPitch, int nVelocity );

	virtual std::vector<char> getBuffer();

protected:
	unsigned m_nChannel;
	unsigned m_nPitch;
	unsigned m_nVelocity;
};



class SMFNoteOffEvent : public SMFEvent
{
public:
	SMFNoteOffEvent(  unsigned nTicks, int nChannel, int nPitch, int nVelocity );

	virtual std::vector<char> getBuffer();

protected:
	unsigned m_nChannel;
	unsigned m_nPitch;
	unsigned m_nVelocity;

};

} // namespace Tritium

#endif // TRITIUM_SMFEVENT_HPP
