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
/* JackClient.hpp
 * Copyright(c) 2008 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This class manages the Jack client handle (returned by jack_client_open()).
 */

#ifndef TRITIUM_JACKCLIENT_HPP
#define TRITIUM_JACKCLIENT_HPP

#ifdef JACK_SUPPORT

#include <jack/types.h>
#include <Tritium/H2Exception.hpp>
#include <Tritium/Engine.hpp>
#include <QtCore/QString>
#include <vector>
#include <set>
#include <cassert>

namespace Tritium
{

class JackClient
{
public:
    JackClient(Engine* parent, bool init_jack = true);
    ~JackClient(void);

    jack_client_t* ref(void);
    int setAudioProcessCallback(JackProcessCallback process, void* arg);
    int setNonAudioProcessCallback(JackProcessCallback process);
    int clearAudioProcessCallback(void);
    int clearNonAudioProcessCallback(void);
    std::vector<QString> getMidiOutputPortList(void);
    void subscribe(void* child_obj);
    void unsubscribe(void* child_obj);

    void activate();
    void deactivate();

    #warning "TODO: These should be private:"
    void open(void);
    void close(void);

    void raise_error(Engine::error_t err);
    bool jack_is_up();

private:
    Engine* m_engine;
    jack_client_t* m_client;
    std::set<void*> m_children;
    JackProcessCallback m_audio_process;
    void* m_audio_process_arg;
    JackProcessCallback m_nonaudio_process;
    // void* m_nonaudio_process_arg; // always use m_audio_process_arg

    friend class Engine;
};

} // namespace Tritium

#endif // JACK_SUPPORT

#endif // TRITIUM_JACKCLIENT_HPP
