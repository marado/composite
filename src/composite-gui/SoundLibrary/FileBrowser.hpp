/*
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * This file is part of Composite
 *
 * Composite is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Composite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMPOSITE_FILEBROWSER_HPP
#define COMPOSITE_FILEBROWSER_HPP

#include "config.h"

#include <QtGui>

class FileBrowser : public QWidget
{
	Q_OBJECT
	public:
		FileBrowser( QWidget* pParent );
		~FileBrowser();

	private slots:
		void on_fileList_ItemChanged( QListWidgetItem * current, QListWidgetItem * previous );
		void on_fileList_ItemActivated( QListWidgetItem* );

		void on_dirList_ItemActivated( QListWidgetItem* );
		void on_upBtnClicked();
		void on_playBtnClicked();

	private:
		QLabel *m_pDirectoryLabel;
		QPushButton* m_pUpBtn;
		QLabel *m_pFileInfo;
		QListWidget *m_pDirList;
		QListWidget *m_pFileList;
		QDir m_directory;


		void loadDirectoryTree( const QString& basedir );
		void updateFileInfo( QString sFilename, unsigned nSampleRate, unsigned nBytes );

};


#endif // COMPOSITE_FILEBROWSER_HPP
