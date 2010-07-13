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

#ifndef TRITIUM_EVENT_QUEUE_HPP
#define TRITIUM_EVENT_QUEUE_HPP

#define MAX_EVENTS 1024

namespace Tritium
{

enum EventType {
	EVENT_NONE,
	EVENT_STATE,
	EVENT_PATTERN_CHANGED,
	EVENT_PATTERN_MODIFIED,
	EVENT_SELECTED_PATTERN_CHANGED,
	EVENT_SELECTED_INSTRUMENT_CHANGED,
	EVENT_MIDI_ACTIVITY,
	EVENT_XRUN,
	EVENT_NOTEON,
	EVENT_ERROR,
	EVENT_METRONOME,
	EVENT_PROGRESS,
	EVENT_TRANSPORT,
	EVENT_JACK_TIME_MASTER
};


class Event
{
public:
	EventType type;
	int value;
};

///
/// Event queue: is the way the engine talks to the GUI
///
class EventQueue
{
public:
	EventQueue();
	~EventQueue();

	void push_event( EventType type, int nValue );
	Event pop_event();

private:
	int __read_index;
	int __write_index;
	Event __events_buffer[ MAX_EVENTS ];
};

} // namespace Tritium

#endif // TRITIUM_EVENT_QUEUE_HPP
