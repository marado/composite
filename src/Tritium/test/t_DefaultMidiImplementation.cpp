/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
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

/**
 * t_DefaultMidiImplementation.cpp
 *
 * This is just the template for a test.  If the template is not
 * executable, it will not be kept up-to-date.
 */

#include <Tritium/DefaultMidiImplementation.hpp>
#include <Tritium/SeqEvent.hpp>
#include <vector>
#include <cstdlib>

// CHANGE THIS TO MATCH YOUR FILE:
#define THIS_NAMESPACE t_DefaultMidiImplementation
#include "test_macros.hpp"
#include "test_config.hpp"

using namespace Tritium;

namespace THIS_NAMESPACE
{

    struct Fixture
    {
	// SETUP AND TEARDOWN OBJECTS FOR YOUR TESTS.
	DefaultMidiImplementation mi;
	SeqEvent ev;

	Fixture() : mi() {}
	~Fixture() {}
    };

} // namespace THIS_NAMESPACE

TEST_BEGIN( Fixture );

TEST_CASE( 010_defaults )
{
    CK( false == mi.translate(ev, 0, 0) );
}

TEST_CASE( 020_fuzz )
{
    /* Fuzz the MIDI Implementation with random data.
     * See if it crashes.
     */
    std::vector<uint8_t> noise;
    unsigned k;
    bool rv;

    for( k=0 ; k<4096 ; ++k ) {
	noise.push_back( rand() & 0xFF );
    }

    unsigned pos;
    for( pos=0 ; pos < noise.size() ; pos += k ) {
	k = rand() % 6;
	if( (pos+k) > noise.size() ) {
	    k = noise.size() - pos;
	}
	rv = mi.translate(ev, k, &noise[pos]);
    }
}

TEST_END()
