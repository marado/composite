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

#ifndef TRITIUM_SOUNDLIBRARY_HPP
#define TRITIUM_SOUNDLIBRARY_HPP

#include <Tritium/memory.hpp>
#include <Tritium/Mixer.hpp>
#include <vector>
#include <deque>

namespace Tritium
{

    class InstrumentList;


    /**
     * \brief	SoundLibrary class.
     */
    class SoundLibrary
    {
    public:
	SoundLibrary();
	~SoundLibrary();

    private:
    };

    class EngineInterface;

    /**
     * \brief	Drumkit info
     */
    class Drumkit
    {
    public:
	typedef std::deque< T<Mixer::Channel>::shared_ptr > channel_list_t;

	Drumkit();
	~Drumkit();

	/// Loads a single Drumkit
	static T<Drumkit>::shared_ptr load( EngineInterface* engine, const QString& sFilename );

	/// Lists the User drumkit list
	static std::vector<QString> getUserDrumkitList(EngineInterface* engine);

	/// Lists the System drumkit list
	static std::vector<QString> getSystemDrumkitList(EngineInterface* engine);

	/// Installs a drumkit
	static void install( EngineInterface* engine, const QString& filename );

	// Save a drumkit
	static void save( EngineInterface* engine, const QString& sName, const QString& sAuthor, const QString& sInfo, const QString& sLicense );


	/// Remove a Drumkit from the disk
	static void removeDrumkit( EngineInterface* engine, const QString& sDrumkitName );

	T<InstrumentList>::shared_ptr getInstrumentList() {
	    return m_pInstrumentList;
	}
	void setInstrumentList( T<InstrumentList>::shared_ptr instr ) {
	    this->m_pInstrumentList = instr;
	}

	// Mixer data for each instrument in the instrumentlist.
	channel_list_t& channels() {
	    return m_channels;
	}

	void setName( const QString& name ) {
	    this->m_sName = name;
	}
	const QString& getName() {
	    return m_sName;
	}

	void setAuthor( const QString& author ) {
	    this->m_sAuthor = author;
	}
	const QString& getAuthor() {
	    return m_sAuthor;
	}

	void setInfo( const QString& info ) {
	    this->m_sInfo = info;
	}
	const QString& getInfo() {
	    return m_sInfo;
	}

	void setLicense( const QString& license ) {
	    this->m_sLicense = license;
	}
	const QString& getLicense() {
	    return m_sLicense;
	}

	void dump();

    private:
	T<InstrumentList>::shared_ptr m_pInstrumentList;
	channel_list_t m_channels;
	QString m_sName;
	QString m_sAuthor;
	QString m_sInfo;
	QString m_sLicense;
    };

} // namespace Tritium

#endif // TRITIUM_SOUNDLIBRARY_HPP
