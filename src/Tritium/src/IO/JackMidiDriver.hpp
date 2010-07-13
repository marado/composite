/*
 * Copyright(c) 2008,2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
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
/* JackMidiDriver.hpp
 * Copyright(c) 2008 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * Note: This class implements it's own Jack Client object and process
 * callback.  Once working, it should be merged with the JackOutput
 * class into a superclass called something like JackDriver.
 */

#ifndef TRITIUM_JACKMIDIDRIVER_HPP
#define TRITIUM_JACKMIDIDRIVER_HPP

#ifdef JACK_SUPPORT

#include <Tritium/IO/MidiInput.hpp>
#include <Tritium/H2Exception.hpp>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <vector>
#include <QtCore/QString>
#include <Tritium/memory.hpp>

namespace Tritium
{

class JackClient;

class JackMidiDriver : public MidiInput
{
public:
	JackMidiDriver(T<JackClient>::shared_ptr parent, Engine* e_parent);
	~JackMidiDriver();

	// Reimplemented from MidiInput
	void open(void);
	void close(void);
	virtual std::vector<QString> getOutputPortList(void);

	int processAudio(jack_nframes_t nframes);
	int processNonAudio(jack_nframes_t nframes);

private:
	T<JackClient>::shared_ptr m_jack_client;
	jack_port_t* m_port;

	int process(jack_nframes_t nframes, bool use_frame);

}; // JackMidiDriver

} // namespace Tritium

#endif // JACK_SUPPORT

#endif // TRITIUM_JACKMIDIDRIVER_HPP
