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

#include "SoundLibraryExportDialog.hpp"
#include <Tritium/LocalFileMng.hpp>
#include <Tritium/SoundLibrary.hpp>

#include <Tritium/Engine.hpp>
#include <Tritium/Preferences.hpp>

#include <Tritium/ADSR.hpp>
#include <Tritium/Sample.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/H2Exception.hpp>
#include <Tritium/SoundLibrary.hpp>
#include <Tritium/DataPath.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

#include <memory>
#include <QtGui>

using namespace Tritium;

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent )
 : QDialog( pParent )
{
	setupUi( this );
	DEBUGLOG( "INIT" );
	setWindowTitle( trUtf8( "Export Sound Library" ) );
	setFixedSize( width(), height() );
	updateDrumkitList();
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
	DEBUGLOG( "DESTROY" );
	drumkitInfoList.clear();
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QString drumkitName = drumkitList->currentText();

	Tritium::LocalFileMng fileMng(g_engine);
	QString drumkitDir = fileMng.getDrumkitDirectory( drumkitName );

	QString saveDir = drumkitPathTxt->text();
	QString cmd = QString( "cd " ) + drumkitDir + "; tar czf \"" + saveDir + "/" + drumkitName + ".h2drumkit\" \"" + drumkitName + "\"";

	DEBUGLOG( "cmd: " + cmd );
	system( cmd.toLocal8Bit() );

	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Composite", "Drumkit exported." );
}

void SoundLibraryExportDialog::on_drumkitPathTxt_textChanged( QString str )
{
	QString path = drumkitPathTxt->text();
	if (path.isEmpty()) {
		exportBtn->setEnabled( false );
	}
	else {
		exportBtn->setEnabled( true );
	}
}

void SoundLibraryExportDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::Directory);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export drumkit" ) );

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if ( !filename.isEmpty() ) {
		drumkitPathTxt->setText( filename );
		lastUsedDir = fd->directory().absolutePath();
	}
	DEBUGLOG( "Filename: " + filename );
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	DEBUGLOG( "[updateDrumkitList]" );

	drumkitList->clear();
	drumkitInfoList.clear();

	//LocalFileMng mng;
	std::vector<QString> userList = Drumkit::getUserDrumkitList(g_engine);
	for (uint i = 0; i < userList.size(); i++) {
		QString absPath =  userList[i];

		T<Drumkit>::shared_ptr info = Drumkit::load( g_engine, absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->getName() );
		}
	}


	std::vector<QString> systemList = Drumkit::getSystemDrumkitList(g_engine);
	for (uint i = 0; i < systemList.size(); i++) {
		QString absPath = systemList[i];
		T<Drumkit>::shared_ptr info = Drumkit::load( g_engine, absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->getName() );
		}
	}

	
	/// \todo sort in exportTab_drumkitList
//	drumkitList->sort();

	drumkitList->setCurrentIndex( 0 );
}
