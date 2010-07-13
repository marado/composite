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

#include <Tritium/EventQueue.hpp>

namespace Tritium
{

EventQueue::EventQueue()
		: __read_index( 0 )
		, __write_index( 0 )
{
	for ( int i = 0; i < MAX_EVENTS; ++i ) {
		__events_buffer[ i ].type = EVENT_NONE;
		__events_buffer[ i ].value = 0;
	}
}


EventQueue::~EventQueue()
{
}


void EventQueue::push_event( EventType type, int nValue )
{
	int nIndex = ++__write_index;
	nIndex = nIndex % MAX_EVENTS;
//	infoLog( "[pushEvent] " + toString( nIndex ) );
	Event ev;
	ev.type = type;
	ev.value = nValue;
	__events_buffer[ nIndex ] = ev;
}


Event EventQueue::pop_event()
{
	if ( __read_index == __write_index ) {
		Event ev;
		ev.type = EVENT_NONE;
		ev.value = 0;
		return ev;
	}
	int nIndex = ++__read_index;
	nIndex = nIndex % MAX_EVENTS;
//	infoLog( "[popEvent] " + toString( nIndex ) );
	return __events_buffer[ nIndex ];
}

};
