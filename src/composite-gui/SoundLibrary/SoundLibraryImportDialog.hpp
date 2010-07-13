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

#ifndef COMPOSITE_SOUNDLIBRARYIMPORTDIALOG_HPP
#define COMPOSITE_SOUNDLIBRARYIMPORTDIALOG_HPP

#include "config.h"

#include "ui_SoundLibraryImportDialog_UI.h"
#include <Tritium/Preferences.hpp>

#include <vector>


struct SoundLibraryInfo
{
	QString m_sName;
	QString m_sURL;
	QString m_sInfo;
	QString m_sAuthor;
	QString m_sType;
	QString m_sLicense;
};


///
/// This dialog is used to import a SoundLibrary file from a local file or via HTTP.
///
class SoundLibraryImportDialog : public QDialog, public Ui_SoundLibraryImportDialog_UI
{
	Q_OBJECT
	public:
		SoundLibraryImportDialog( QWidget* pParent );
		~SoundLibraryImportDialog();

	private slots:
		void on_EditListBtn_clicked();
		void on_UpdateListBtn_clicked();
		void on_DownloadBtn_clicked();
		void on_BrowseBtn_clicked();
		void on_InstallBtn_clicked();

		void on_close_btn_clicked();

		void soundLibraryItemChanged( QTreeWidgetItem*, QTreeWidgetItem* );

	private:
		std::vector<SoundLibraryInfo> m_soundLibraryList;
		Tritium::Preferences *pPref;

		QTreeWidgetItem* m_pDrumkitsItem;
		QTreeWidgetItem* m_pSongItem;
		QTreeWidgetItem* m_pPatternItem;

		bool isSoundLibraryItemAlreadyInstalled( SoundLibraryInfo sInfo );
		void updateSoundLibraryList();
		void updateRepositoryCombo();
};


#endif // COMPOSITE_SOUNDLIBRARYIMPORTDIALOG_HPP
