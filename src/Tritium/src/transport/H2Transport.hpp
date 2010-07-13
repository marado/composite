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
#ifndef TRITIUM_H2TRANSPORT_HPP
#define TRITIUM_H2TRANSPORT_HPP

#include <Tritium/Transport.hpp>

namespace Tritium
{
    class Engine;
    class H2TransportPrivate;
    class Song;
    class JackClient;

    /**
     * This is the only transport that Engine shall directly interact with.
     * It shall manage all transport backends (Internal, Jack, whatever).
     *
     * From the sequencer's point of view, it is expected to be used like this:
     *
     * int process(uint32_t nFrames) {
     *     TransportPosition pos;        // Defined in TransportMasterInterface.h
     *     Transport* xport = m_engine->get_transport();
     *     xport->get_position(&pos);
     *
     *     // Sequence notes based on [Bar:beat.tick] + bbt_offset
     *     // and **NOT** based on the frame number.
     *
     *     // Tell the transport to move to the next cycle
     *     xport->processed_frames(nFrames);
     * }
     */
    class H2Transport : public Transport
    {
    public:
        // Normal transport controls
        virtual int locate(uint32_t frame);
        virtual int locate(uint32_t bar, uint32_t beat, uint32_t tick);
        virtual void start(void);
        virtual void stop(void);
        virtual void get_position(TransportPosition* pos);

        // Interface for sequencer.  At the end of process(), declare the number
        // of frames processed.  This is needed so that the internal transport
        // master can keep track of time.
        virtual void processed_frames(uint32_t nFrames);
        virtual void set_current_song(T<Song>::shared_ptr s);

        // Convenience interface (mostly for GUI)
        virtual uint32_t get_current_frame(void);
	virtual TransportPosition::State get_state(void);

	// Special Methods for Jack Transport.
	bool setJackTimeMaster(T<JackClient>::shared_ptr parent, bool if_none_already = false);
	void clearJackTimeMaster();
	bool getJackTimeMaster();

        // Interface for application (i.e. GUI)
        // * get list of transport models
        // * set current transport model
        // * possibly set parameters on transport models.

        H2Transport(Engine* parent);
        virtual ~H2Transport();

    private:
        H2TransportPrivate* d;
    };


} // namespace Tritium

#endif // TRITIUM_H2TRANSPORT_HPP
