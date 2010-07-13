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
#include <Tritium/Pattern.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/Note.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/memory.hpp>

#include <vector>
#include <cassert>
namespace Tritium
{

Pattern::Pattern( const QString& name, const QString& category, unsigned length )
{
//	DEBUGLOG( "INIT: " + m_sName );
	set_name( name );
	set_category( category );
	set_length( length );
}



Pattern::~Pattern()
{
//	DEBUGLOG( "DESTROY: " + m_sName );

	// delete all Notes
	Pattern::note_map_t::iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = pos->second;
		delete pNote;
	}
}


void Pattern::purge_instrument( T<Instrument>::shared_ptr I, Engine* engine )
{
	bool locked = false;
	std::list< Note* > slate;
	
	Pattern::note_map_t::iterator pos = note_map.begin();
	while ( pos != note_map.end() ) {
		Note *pNote = pos->second;
		assert( pNote );
		
		if ( pNote->get_instrument() == I ) {
			if ( !locked ) {
				engine->lock( RIGHT_HERE );
				locked = true;
			}
			slate.push_back( pNote );
			
			note_map.erase( pos++ );
		} else {
			++pos;
		}
	}
	
	if ( locked ) {
		engine->unlock();
		while ( slate.size() ) {
			delete slate.front();
			slate.pop_front();
		}
	}
}


bool Pattern::references_instrument( T<Instrument>::shared_ptr I )
{
	Pattern::note_map_t::const_iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = pos->second;
		assert( pNote );
		if ( pNote->get_instrument() == I ) {
			return true;
		}
	}
	return false;
}



/// Returns an empty Pattern
T<Pattern>::shared_ptr Pattern::get_empty_pattern()
{
	T<Pattern>::shared_ptr pat( new Pattern( "Pattern", "not_categorized" ) );
	return pat;
}



T<Pattern>::shared_ptr Pattern::copy()
{
//	ERRORLOG( "not implemented yet!!!" );

	T<Pattern>::shared_ptr newPat( new Pattern( __name, __category ) );
	newPat->set_length( get_length() );

	Pattern::note_map_t::iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = new Note( pos->second );
		newPat->note_map.insert( std::make_pair( pos->first, pNote ) );
	}

	return newPat;
}



void Pattern::debug_dump()
{
	DEBUGLOG( "Pattern dump" );
	DEBUGLOG( "Pattern name: " + __name );
	DEBUGLOG( "Pattern category: " + __category );
	DEBUGLOG( QString("Pattern length: %1").arg( get_length() ) );
}



// ::::::::::::::::::::::::



PatternList::PatternList()
{
//	infoLog("Init");
}



PatternList::~PatternList()
{
//	infoLog("destroy");

	// find single patterns. (skip duplicates)
	std::vector< T<Pattern>::shared_ptr > temp;
	for ( unsigned int i = 0; i < list.size(); ++i ) {
		T<Pattern>::shared_ptr pat = list[i];

		// pat exists in temp?
		bool exists = false;
		for ( unsigned int j = 0; j < temp.size(); ++j ) {
			if ( pat == temp[j] ) {
				exists = true;
				break;
			}
		}
		if ( !exists ) {
			temp.push_back( pat );
		}
	}

	// delete patterns
	for ( unsigned int i = 0; i < temp.size(); ++i ) {
		T<Pattern>::shared_ptr pat = temp[i];
		if ( pat != NULL ) {
			pat.reset();
		}
	}
}



void PatternList::add( T<Pattern>::shared_ptr newPattern )
{
	list.push_back( newPattern );
}



T<Pattern>::shared_ptr PatternList::get( int nPos )
{
	if ( nPos >= ( int )list.size() ) {
		ERRORLOG( QString("Pattern index out of bounds. nPos > list.size() - %1 > %2")
			  .arg( nPos )
			  .arg( list.size() )
			);
		return T<Pattern>::shared_ptr();
	}
//	assert( nPos < (int)list.size() );
	return list[ nPos ];
}



unsigned PatternList::get_size()
{
	return list.size();
}



void PatternList::clear()
{
	list.clear();
}



/// Replace an existent pattern with another one
void PatternList::replace( T<Pattern>::shared_ptr newPattern, unsigned int pos )
{
	if ( pos >= ( unsigned )list.size() ) {
		ERRORLOG( QString("Pattern index out of bounds in PatternList::replace. pos >= list.size() - %1 > %2")
			  .arg( pos )
			  .arg( list.size() )
			);
		return;
	}
	list.insert( list.begin() + pos, newPattern );	// insert the new pattern
	// remove the old pattern
	list.erase( list.begin() + pos + 1 );
}



int PatternList::index_of( T<Pattern>::shared_ptr pattern )
{
	if ( get_size() < 1 ) return -1;

	std::vector< T<Pattern>::shared_ptr >::iterator i;

	int r = 0;
	for ( i = list.begin(); i != list.end(); ++i ) {
		if ( *i == pattern ) return r;
		++r;
	}
	return -1;
}



T<Pattern>::shared_ptr PatternList::del( T<Pattern>::shared_ptr p )
{
	bool did_delete = false;
	if ( get_size() < 1 ) return T<Pattern>::shared_ptr();

	std::vector< T<Pattern>::shared_ptr >::iterator i;

	for ( i = list.begin(); i != list.end(); i++ ) {
		if ( *i == p ) {
			i = list.erase( i );
			did_delete = true;
			break;
			// NOTE: Do we need to delete EVERY instance of p in the list? Better to avoid adding more than one copy of a pattern in the first place! Using the iterator i after modifying the list is dangerous, so either just delete one OR...
			// ... or roll back the iterator after deleting :
// 			i--;
		}
	}
	if ( did_delete ) return p;
	return T<Pattern>::shared_ptr();
}




void PatternList::del( unsigned pos )
{
	if ( pos >= ( unsigned )list.size() ) {
		ERRORLOG( QString("Pattern index out of bounds in PatternList::del. pos >= list.size() - %1 > %2")
			  .arg( pos )
			  .arg( list.size() )
			);
		return;
	}
	list.erase( list.begin()+pos );
}


};
