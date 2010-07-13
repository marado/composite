/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
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
#ifndef TRITIUM_SERIALIZATIONPRIVATE_HPP
#define TRITIUM_SERIALIZATIONPRIVATE_HPP

#include "WorkerThread.hpp"
#include <Tritium/Serialization.hpp>
#include <Tritium/ObjectBundle.hpp>
#include <QDomNode>
#include <QString>
#include <Tritium/memory.hpp>
#include <list>
#include <vector>
#include <deque>

class QUrl;

namespace Tritium
{
    class Drumkit;
    class EngineInterface;
    class InstrumentList;
    class LadspaFX;
    class Pattern;
    class Song;

    namespace Serialization
    {
	class SerializationQueue : public WorkerThreadClient
	{
	public:
	    SerializationQueue(EngineInterface* engine);
	    virtual ~SerializationQueue();

	    // WorkerThreadClient interface:
	    virtual bool events_waiting();
	    virtual int process();
	    virtual void shutdown();

	    void load_uri(const QString& uri,
			  ObjectBundle& report_to,
			  EngineInterface *engine);
	    void save_song(const QString& filename,
			   T<Song>::shared_ptr song,
			   SaveReport& report_t,
			   EngineInterface *engine,
			   bool overwrite);
	    void save_drumkit(const QString& filename,
			      T<Drumkit>::shared_ptr dk,
			      SaveReport& report_to,
			      EngineInterface *engine,
			      bool overwrite);
	    void save_pattern(const QString& filename,
			      T<Pattern>::shared_ptr pattern,
			      const QString& drumkit_name,
			      SaveReport& report_to,
			      EngineInterface *engine,
			      bool overwrite);

	private:
	    typedef enum {
		LoadUri,
		SaveSong,
		SaveDrumkit,
		SavePattern
	    } event_type_t;

	    typedef struct {
		event_type_t ev;
		QString uri;
		union {
		    ObjectBundle* report_load_to;
		    SaveReport* report_save_to;
		};
		EngineInterface *engine;
		T<Song>::shared_ptr song;
		T<Drumkit>::shared_ptr drumkit;
		T<Pattern>::shared_ptr pattern;
		QString drumkit_name;
		bool overwrite;
	    } event_data_t;

	    typedef std::list<event_data_t> queue_t;

	    bool m_kill;
	    queue_t m_queue;
	    EngineInterface *m_engine;

	protected:
	    void handle_load_uri(event_data_t& ev);
	    void handle_load_file(event_data_t& ev, const QString& filename);
	    void handle_save_song(event_data_t& ev, const QString& filename);
	    void handle_save_drumkit(event_data_t& ev, const QString& filename);
	    void handle_save_pattern(event_data_t& ev, const QString& filename);
	    void handle_load_song(event_data_t& ev, const QString& filename);
	    void handle_load_drumkit(event_data_t& ev, const QString& filename);
	    void handle_load_pattern(event_data_t& ev, const QString& filename);
	    void handle_load_tritium(event_data_t& ev, const QString& filename);

	    void handle_callback(
		event_data_t& ev,
		QString filename = QString(),
		bool error = false,
		QString error_message = QString()
		);

	    // Node translators
	    T<Song>::shared_ptr handle_load_song_node(
		QDomElement songNode,
		QStringList& errors
		);
	    void handle_load_instrumentlist_node(
		std::deque< T<Instrument>::shared_ptr >& inst_dest,
		std::deque< T<Mixer::Channel>::shared_ptr >& chan_dest,
		const QString& drumkit_path,
		QDomElement& inst_l_node,
		QStringList& errors
		);
	    void handle_load_instrument_node(
		QDomElement& instrumentNode,
		const QString& drumkit_path,
		T<Instrument>::shared_ptr& inst_rv,
		T<Mixer::Channel>::shared_ptr& chan_rv,
		QStringList& errors
		);
	    void handle_load_patternlist_node(
		std::deque< T<Pattern>::shared_ptr >& dest,
		QDomElement& pat_l_node,
		const std::deque< T<Instrument>::shared_ptr >& insts,
		QStringList& errors
		);
	    T<Pattern>::shared_ptr handle_load_pattern_node(
		QDomElement& pat_node,
		const std::deque< T<Instrument>::shared_ptr >& insts,
		QStringList& errors
		);
	    T<Pattern>::shared_ptr handle_load_pattern_node_pre094(
		QDomElement& pat_node,
		const std::deque< T<Instrument>::shared_ptr >& insts,
		QStringList& errors
		);
	    T<Pattern>::shared_ptr handle_load_pattern_node_094(
		QDomElement& pat_node,
		const std::deque< T<Instrument>::shared_ptr >& insts,
		QStringList& errors
		);
	    void handle_load_patternsequence_node(
		std::deque< QStringList >& pat_seq,
		QDomElement& pat_s_node,
		QStringList& errors
		);
	    void handle_load_ladspa_node(
		std::deque< T<LadspaFX>::shared_ptr >& dest,
		QDomElement& ladspaNode,
		QStringList& errors
		);
	    T<LadspaFX>::shared_ptr handle_load_fx_node(
		QDomElement& fx_node,
		QStringList& errors
		);

	    // Miscellaneous
	    bool ensure_default_exists(const QUrl& uri);

	};

	class SerializerImpl : public Serializer
	{
	public:
	    virtual void load_uri(const QString& uri,
				  ObjectBundle& report_to,
				  EngineInterface *engine);
	    virtual void save_song(const QString& filename,
				   T<Song>::shared_ptr song,
				   SaveReport& report_to,
				   EngineInterface *engine,
				   bool overwrite = false);
	    virtual void save_drumkit(const QString& filename,
				      T<Drumkit>::shared_ptr song,
				      SaveReport& report_to,
				      EngineInterface *engine,
				      bool overwrite = false);
	    virtual void save_pattern(const QString& filename,
				      T<Pattern>::shared_ptr pattern,
				      const QString& drumkit_name,
				      SaveReport& report_to,
				      EngineInterface *engine,
				      bool overwrite = false);

	    SerializerImpl(EngineInterface* engine);
	    virtual ~SerializerImpl();

	protected:
	    T<SerializationQueue>::shared_ptr m_queue;
	};

	class SerializerStandalone : public SerializerImpl
	{
	public:
	    SerializerStandalone(EngineInterface* engine);
	    virtual ~SerializerStandalone();

	private:
	    WorkerThread m_thread;
	};

	/**
	 * \brief ObjectBundle functor for synchronous loading.
	 *
	 * This is a convenience class for those that want to wait
	 * until things are loaded before continuing.
	 */
	class SynchronousObjectBundle : public ObjectBundle
	{
	public:
	    bool ready;

	    SynchronousObjectBundle() : ready(false) {}
	    virtual ~SynchronousObjectBundle() {}

	    virtual void operator()() { ready = true; }
	};

    } // namespace Serialization

} // namespace Tritium

#endif // TRITIUM_SERIALIZATIONPRIVATE_HPP
