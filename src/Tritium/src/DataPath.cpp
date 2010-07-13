/*
 * Copyright(c) 2002-2008 Jonathan Dempsey, Alessandro Cominu
 *
 * This file is part of Tritium
 *
 * Tritium is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Tritium is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * DataPath.h
 * Header to define the path to the data files for Composite in such a
 * way that self-contained Mac OS X application bundles can be built.
 * Copyright (c) 2005 Jonathan Dempsey
 *
 */
#include <Tritium/DataPath.hpp>

#include <QFile>
#include <QApplication>

//#ifdef Q_OS_MACX
//#  include <Carbon.h>
//#endif

#include <iostream>
using namespace std;

#include "config.h"

namespace Tritium
{

QString DataPath::__data_path;

QString DataPath::get_data_path()
{
	#warning "TODO: QApplication used in libTritium"
#ifdef Q_OS_MACX

	QString qStringPath = qApp->applicationDirPath() + QString ( "/../Resources/data" ) ;
	return qStringPath;
#elif WIN32

	QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
	return qStringPath;

#else
	if ( __data_path.isEmpty() ) {
		QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
		__data_path = qStringPath;

		QFile file( __data_path );
		if ( !file.exists() ) {
			// try using the system wide data dir
			__data_path = DATA_PATH;
		}
	}
	return __data_path;
#endif
}

};

