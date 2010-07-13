/*
 * Copyright(c) 2002-200/ Alessandro Cominu
 * Copyright (c) 2005 Jonathan Dempsey
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

#ifndef TRITIUM_DATA_PATH_HPP
#define TRITIUM_DATA_PATH_HPP

#include <QtCore>

namespace Tritium
{

class DataPath
{
public:
	static QString get_data_path();

private:
	static QString __data_path;
};

} // namespace Tritium

#endif // TRITIUM_DATA_PATH_HPP

