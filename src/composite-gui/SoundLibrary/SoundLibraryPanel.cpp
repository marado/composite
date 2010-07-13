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

#include "SoundLibraryPanel.hpp"

#include <QtGui>

#include "SoundLibraryTree.hpp"
#include "FileBrowser.hpp"

#include "SoundLibrarySaveDialog.hpp"
#include "SoundLibraryPropertiesDialog.hpp"
#include "SoundLibraryExportDialog.hpp"

#include "../CompositeApp.hpp"
#include "../widgets/Button.hpp"
#include "../widgets/PixmapWidget.hpp"
#include "../Skin.hpp"
#include "../SongEditor/SongEditorPanel.hpp"
#include "../PatternEditor/PatternEditorPanel.hpp"
#include "../PatternEditor/DrumPatternEditor.hpp"
#include "../PatternEditor/PatternEditorInstrumentList.hpp"
#include "../InstrumentRack.hpp"
#include "../InstrumentEditor/InstrumentEditorPanel.hpp"

#include <Tritium/ADSR.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/DataPath.hpp>
#include <Tritium/H2Exception.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/LocalFileMng.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Pattern.hpp>
#include <Tritium/Sample.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Song.hpp>
#include <Tritium/SoundLibrary.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

using namespace Tritium;

#include <cassert>

SoundLibraryPanel::SoundLibraryPanel( QWidget *pParent )
 : QWidget( pParent )
 , __sound_library_tree( NULL )
 , __drumkit_menu( NULL )
 , __instrument_menu( NULL )
 , __song_menu( NULL )
 , __pattern_menu( NULL )
 , __pattern_menu_list( NULL )
 , __system_drumkits_item( NULL )
 , __user_drumkits_item( NULL )
 , __song_item( NULL )
 , __pattern_item( NULL )
 , __pattern_item_list( NULL )
{
	//DEBUGLOG( "INIT" );
	__drumkit_menu = new QMenu( this );
	__drumkit_menu->addAction( trUtf8( "Load" ), this, SLOT( on_drumkitLoadAction() ) );
	__drumkit_menu->addAction( trUtf8( "Export" ), this, SLOT( on_drumkitExportAction() ) );
	__drumkit_menu->addAction( trUtf8( "Properties" ), this, SLOT( on_drumkitPropertiesAction() ) );
	__drumkit_menu->addSeparator();
	__drumkit_menu->addAction( trUtf8( "Delete" ), this, SLOT( on_drumkitDeleteAction() ) );

	__instrument_menu = new QMenu( this );
	__instrument_menu->addSeparator();
	__instrument_menu->addAction( trUtf8( "Delete" ), this, SLOT( on_instrumentDeleteAction() ) );

	__song_menu = new QMenu( this );
	__song_menu->addSeparator();
	__song_menu->addAction( trUtf8( "Load" ), this, SLOT( on_songLoadAction() ) );

	__pattern_menu = new QMenu( this );
	__pattern_menu->addSeparator();
	__pattern_menu->addAction( trUtf8( "Load" ), this, SLOT( on_patternLoadAction() ) );
	__pattern_menu->addAction( trUtf8( "Delete" ), this, SLOT( on_patternDeleteAction() ) );

	__pattern_menu_list = new QMenu( this );
	__pattern_menu_list->addSeparator();
	__pattern_menu_list->addAction( trUtf8( "Load" ), this, SLOT( on_patternLoadAction() ) );

// DRUMKIT LIST
	__sound_library_tree = new SoundLibraryTree( NULL );
	connect( __sound_library_tree, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( on_DrumkitList_ItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
	connect( __sound_library_tree, SIGNAL( itemActivated ( QTreeWidgetItem*, int ) ), this, SLOT( on_DrumkitList_itemActivated( QTreeWidgetItem*, int ) ) );
	connect( __sound_library_tree, SIGNAL( leftClicked(QPoint) ), this, SLOT( on_DrumkitList_leftClicked(QPoint)) );
	connect( __sound_library_tree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_DrumkitList_rightClicked(QPoint)) );
	connect( __sound_library_tree, SIGNAL( onMouseMove( QMouseEvent* ) ), this, SLOT( on_DrumkitList_mouseMove( QMouseEvent* ) ) );


	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );

	pVBox->addWidget( __sound_library_tree );
	

	this->setLayout( pVBox );

	__expand_pattern_list = g_engine->get_preferences()->__expandPatternItem;
	__expand_songs_list = g_engine->get_preferences()->__expandSongItem;

	updateDrumkitList();
}



SoundLibraryPanel::~SoundLibraryPanel()
{
	__system_drumkit_info_list.clear();
	__user_drumkit_info_list.clear();
}



void SoundLibraryPanel::updateDrumkitList()
{
	QString currentSL = g_engine->getCurrentDrumkitname() ; 

	LocalFileMng mng(g_engine);

	__sound_library_tree->clear();



	__system_drumkits_item = new QTreeWidgetItem( __sound_library_tree );
	__system_drumkits_item->setText( 0, trUtf8( "System drumkits" ) );
	__sound_library_tree->setItemExpanded( __system_drumkits_item, true );

	__user_drumkits_item = new QTreeWidgetItem( __sound_library_tree );
	__user_drumkits_item->setText( 0, trUtf8( "User drumkits" ) );
	__sound_library_tree->setItemExpanded( __user_drumkits_item, true );

	

	__system_drumkit_info_list.clear();
	__user_drumkit_info_list.clear();

	//User drumkit list
	std::vector<QString> userList = Drumkit::getUserDrumkitList(g_engine);
	for (uint i = 0; i < userList.size(); ++i) {
		QString absPath =  userList[i];
		T<Drumkit>::shared_ptr pInfo = mng.loadDrumkit( absPath );

		QString filenameforpattern = absPath + "/patterns/";
		

		if (pInfo) {

			__user_drumkit_info_list.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( __user_drumkits_item );
			pDrumkitItem->setText( 0, pInfo->getName() );
			if ( QString(pInfo->getName() ) == currentSL ){
				pDrumkitItem->setBackgroundColor( 0, QColor( 50, 50, 50) );
			}

			T<InstrumentList>::shared_ptr pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				T<Instrument>::shared_ptr pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + pInstr->get_name() );
				pInstrumentItem->setToolTip( 0, pInstr->get_name() );
			}
		}
	}


	//System drumkit list
	std::vector<QString> systemList = Drumkit::getSystemDrumkitList(g_engine);

	for (uint i = 0; i < systemList.size(); i++) {
		QString absPath = systemList[i];
		T<Drumkit>::shared_ptr pInfo = mng.loadDrumkit( absPath );
		if (pInfo) {
			__system_drumkit_info_list.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( __system_drumkits_item );
			pDrumkitItem->setText( 0, pInfo->getName() );
			if ( QString( pInfo->getName() ) == currentSL ){
				pDrumkitItem->setBackgroundColor( 0, QColor( 50, 50, 50) );
			}

			T<InstrumentList>::shared_ptr pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				T<Instrument>::shared_ptr pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + pInstr->get_name() );
				pInstrumentItem->setToolTip( 0, pInstr->get_name() );
			}
		}
	}


	
	//Songlist
	std::vector<QString> songList = mng.getSongList();

	if ( songList.size() > 0 ) {

		__song_item = new QTreeWidgetItem( __sound_library_tree );
		__song_item->setText( 0, trUtf8( "Songs" ) );
		__song_item->setToolTip( 0, "double click to expand the list" );
		__sound_library_tree->setItemExpanded( __song_item, __expand_songs_list );

		for (uint i = 0; i < songList.size(); i++) {
			QString absPath = DataPath::get_data_path() + "/songs/" + songList[i];
			QTreeWidgetItem* pSongItem = new QTreeWidgetItem( __song_item );
			pSongItem->setText( 0 , songList[ i ] );
			pSongItem->setToolTip( 0, songList[ i ] );
		}
	}


	//Pattern list
	std::vector<QString> patternDirList = mng.getPatternDirList();
	if ( patternDirList.size() > 0 ) {
		
		__pattern_item = new QTreeWidgetItem( __sound_library_tree );
		__pattern_item->setText( 0, trUtf8( "Patterns" ) );
		__pattern_item->setToolTip( 0, "double click to expand the list" );
		__sound_library_tree->setItemExpanded( __pattern_item, __expand_pattern_list );
			
		//this is to push the mng.getPatternList in all patterns/drumkit dirs
		for (uint i = 0; i < patternDirList.size(); ++i) {
			QString absPath =  patternDirList[i];
			mng.getPatternList( absPath );
		}
		
		//this is the second step to push the mng.funktion 
		std::vector<QString> allPatternDirList = mng.getallPatternList();
		std::vector<QString> patternNameList = mng.getAllPatternName();
		std::vector<QString> allCategoryNameList = mng.getAllCategoriesFromPattern();

		//now sorting via category
		if ( allCategoryNameList.size() > 0 ){
			for (uint i = 0; i < allCategoryNameList.size(); ++i) {
				QString categoryName = allCategoryNameList[i];
	
				QTreeWidgetItem* pCategoryItem = new QTreeWidgetItem( __pattern_item );
				pCategoryItem->setText( 0, categoryName  );
				for (uint i = 0; i < allPatternDirList.size(); ++i) {
					QString patternCategory = mng.getCategoryFromPatternName( allPatternDirList[i]);

					if ( patternCategory == categoryName ){
						QTreeWidgetItem* pPatternItem = new QTreeWidgetItem( pCategoryItem );
						pPatternItem->setText( 0, mng.getPatternNameFromPatternDir( allPatternDirList[i] ));
						pPatternItem->setToolTip( 0, mng.getDrumkitNameForPattern( allPatternDirList[i] ));

					}
				}
			}
		}
	}


}



void SoundLibraryPanel::on_DrumkitList_ItemChanged(
    QTreeWidgetItem * /*current*/,
    QTreeWidgetItem * /*previous*/ )
{
	test_expandedItems();
}



void SoundLibraryPanel::on_DrumkitList_itemActivated(
	QTreeWidgetItem * item,
	int /*column*/
	)
{
	if ( item == __system_drumkits_item
	     || item == __user_drumkits_item
	     || item == __system_drumkits_item->parent()
	     || item->parent() == __song_item
	     || item == __song_item
	     || item == __pattern_item
	     || item->parent() == __pattern_item
	     || item->parent()->parent() == __pattern_item
	     || item == __pattern_item_list
	     || item->parent() == __pattern_item_list
	     || item->parent()->parent() == __pattern_item_list ) {
		return;
	}

	if ( item->parent() == __system_drumkits_item
	     || item->parent() == __user_drumkits_item  ) {
		// e' stato selezionato un drumkit
	}
	else {
		// e' stato selezionato uno strumento
		QString selectedName = item->text(0);
		if( item->text(0) == "Patterns" ) return;

		QString sInstrName = selectedName.remove( 0, selectedName.indexOf( "] " ) + 2 );
		QString sDrumkitName = item->parent()->text(0);
		DEBUGLOG( QString(sDrumkitName) + ", instr:" + sInstrName );

		T<Instrument>::shared_ptr pInstrument = Instrument::load_instrument( g_engine, sDrumkitName, sInstrName );
		pInstrument->set_muted( false );

		g_engine->get_sampler()->preview_instrument( pInstrument );
	}
}







void SoundLibraryPanel::on_DrumkitList_rightClicked( QPoint pos )
{
	if( __sound_library_tree->currentItem() == NULL )
		return;
	
	if (
		( __sound_library_tree->currentItem()->parent() == NULL ) ||
		( __sound_library_tree->currentItem() == __user_drumkits_item ) ||
		( __sound_library_tree->currentItem() == __system_drumkits_item )
	) {
		return;
	}

	if ( __sound_library_tree->currentItem()->parent() == __song_item ) {
		__song_menu->popup( pos );
	}

	if ( __sound_library_tree->currentItem()->parent()->parent() == __pattern_item && __pattern_item != NULL ) {
		__pattern_menu->popup( pos );
	}

	if ( __sound_library_tree->currentItem()->parent() == __user_drumkits_item ) {
		__drumkit_menu->popup( pos );
	}
	else if ( __sound_library_tree->currentItem()->parent()->parent() == __user_drumkits_item ) {
		__instrument_menu->popup( pos );
	}
	//else if ( __sound_library_tree->currentItem()->parent()->parent()->parent() ==  __pattern_item_list ) {
	//	__pattern_menu_list->popup( pos );
	//}
	

	if ( __sound_library_tree->currentItem()->parent() == __system_drumkits_item ) {
		__drumkit_menu->popup( pos );
	}
	else if ( __sound_library_tree->currentItem()->parent()->parent() == __system_drumkits_item ) {
		__instrument_menu->popup( pos );
	}
}



void SoundLibraryPanel::on_DrumkitList_leftClicked( QPoint pos )
{
	__start_drag_position = pos;
}



void SoundLibraryPanel::on_DrumkitList_mouseMove( QMouseEvent *event)
{
	if (! ( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	if ( ( event->pos() - __start_drag_position ).manhattanLength() < QApplication::startDragDistance() ) {
		return;
	}
	
	if ( !__sound_library_tree->currentItem() ) {
		return;
	}

	if (
		( __sound_library_tree->currentItem()->parent() == __system_drumkits_item ) ||
		( __sound_library_tree->currentItem()->parent() == __user_drumkits_item )
	) {
 		// drumkit selection
		//DEBUGLOG( "ho selezionato un drumkit (system)" );
		return;
	}
	else {
		//DEBUGLOG( "ho selezionato uno strumento" );
		// instrument selection
		if ( __sound_library_tree->currentItem() == NULL )
		{
			return;
		}
		
		if ( __sound_library_tree->currentItem()->parent() == NULL )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent() == __song_item )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent()->text(0) == NULL )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent() == __pattern_item ) {
			return;
		}

		if ( __sound_library_tree->currentItem()->parent()->parent() == __pattern_item ) {

			LocalFileMng mng(g_engine);
		
			QString patternName = __sound_library_tree->currentItem()->text( 0 ) + ".h2pattern";
			QString drumkitname = __sound_library_tree->currentItem()->toolTip ( 0 );
			
			QString sDirectory;
		
			std::vector<QString> patternDirList = mng.getPatternDirList();
		
				for (uint i = 0; i < patternDirList.size(); ++i) {
					QString absPath =  patternDirList[i];
					mng.getPatternList( absPath );
				}
		
			std::vector<QString> allPatternDirList = mng.getallPatternList();
		
			for (uint i = 0; i < allPatternDirList.size(); ++i) {
				QString testName = allPatternDirList[i];
				if( testName.contains( patternName ) && testName.contains( drumkitname )){
		
					sDirectory = allPatternDirList[i];
				}
			}

			QString dragtype = "drag pattern";
			QString sText = dragtype + "::" + sDirectory;

			QDrag *pDrag = new QDrag(this);
			QMimeData *pMimeData = new QMimeData;

			pMimeData->setText( sText );
			pDrag->setMimeData( pMimeData);
			//drag->setPixmap(iconPixmap);

			pDrag->start( Qt::CopyAction | Qt::MoveAction );
			return;
		}

		QString sDrumkitName = __sound_library_tree->currentItem()->parent()->text(0);
		QString sInstrumentName = ( __sound_library_tree->currentItem()->text(0) ).remove( 0, __sound_library_tree->currentItem()->text(0).indexOf( "] " ) + 2 );

		QString sText = sDrumkitName + "::" + sInstrumentName;

		QDrag *pDrag = new QDrag(this);
		QMimeData *pMimeData = new QMimeData;

		pMimeData->setText( sText );
		pDrag->setMimeData( pMimeData);
		//drag->setPixmap(iconPixmap);

		pDrag->start( Qt::CopyAction | Qt::MoveAction );
	}
}



void SoundLibraryPanel::on_drumkitLoadAction()
{
	restore_background_color();

	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);

	T<Drumkit>::shared_ptr drumkitInfo;

	// find the drumkit in the list
	for ( uint i = 0; i < __system_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr pInfo = __system_drumkit_info_list[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	for ( uint i = 0; i < __user_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr pInfo = __user_drumkit_info_list[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	assert( drumkitInfo );

	QApplication::setOverrideCursor(Qt::WaitCursor);

	g_engine->loadDrumkit( drumkitInfo );
	g_engine->getSong()->set_modified( true );
	CompositeApp::get_instance()->onDrumkitLoad( drumkitInfo->getName() );
	CompositeApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()->updateEditor();

	InstrumentEditorPanel::get_instance()->updateInstrumentEditor();

	__sound_library_tree->currentItem()->setBackgroundColor ( 0, QColor( 50, 50, 50) );
	QApplication::restoreOverrideCursor();

}


void SoundLibraryPanel::update_background_color()
{
	restore_background_color();
	change_background_color();
}



void SoundLibraryPanel::restore_background_color()
{
	std::vector<QString> systemList = Drumkit::getSystemDrumkitList(g_engine);
	std::vector<QString> userList = Drumkit::getUserDrumkitList(g_engine);
	QString curlib =  g_engine->getCurrentDrumkitname();
 
	for (uint i = 0; i < systemList.size() ; i++){
		if (  !__system_drumkits_item->child( i ) )
			break;
		( __system_drumkits_item->child( i ) )->setBackground( 0, QBrush() );		
	}

	for (uint i = 0; i < userList.size() ; i++){
		if (  !__user_drumkits_item->child( i ) )
			break;
		( __user_drumkits_item->child( i ) )->setBackground(0, QBrush() );
	}
}



void SoundLibraryPanel::change_background_color()
{
	std::vector<QString> systemList = Drumkit::getSystemDrumkitList(g_engine);
	std::vector<QString> userList = Drumkit::getUserDrumkitList(g_engine);
	QString curlib =  g_engine->getCurrentDrumkitname();
 
	for (uint i = 0; i < systemList.size() ; i++){
		if (  !__system_drumkits_item->child( i ) )
			break;
		if ( ( __system_drumkits_item->child( i ) )->text( 0 ) == curlib ){
			( __system_drumkits_item->child( i ) )->setBackgroundColor ( 0, QColor( 50, 50, 50)  );
			break;
		}
	}

	for (uint i = 0; i < userList.size() ; i++){
		if (  !__user_drumkits_item->child( i ) )
			break;
		if ( ( __user_drumkits_item->child( i ))->text( 0 ) == curlib ){
			( __user_drumkits_item->child( i ) )->setBackgroundColor ( 0, QColor( 50, 50, 50)  );
			break;
		}
	}
}



void SoundLibraryPanel::on_drumkitDeleteAction()
{
	QString sSoundLibrary = __sound_library_tree->currentItem()->text( 0 );

	//if we delete the current loaded drumkit we can get truble with some empty pointers
	if ( sSoundLibrary == g_engine->getCurrentDrumkitname() ){
		QMessageBox::warning( this, "Composite", QString( "You try to delet the current loaded drumkit.\nThis is not possible!") );
		return;
	}

	bool bIsUserSoundLibrary = false;
	std::vector<QString> userList = Drumkit::getUserDrumkitList(g_engine);
	for ( uint i = 0; i < userList.size(); ++i ) {
		if ( userList[ i ].endsWith( sSoundLibrary ) ) {
			bIsUserSoundLibrary = true;
			break;
		}
	}

	if ( bIsUserSoundLibrary == false ) {
		QMessageBox::warning( this, "Composite", QString( "A system drumkit can't be deleted.") );
		return;
	}

	int res = QMessageBox::information( this, "Composite", tr( "Warning, the selected drumkit will be deleted from disk.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( res == 1 ) {
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	Drumkit::removeDrumkit( g_engine, sSoundLibrary );
	test_expandedItems();
	updateDrumkitList();
	QApplication::restoreOverrideCursor();

}



void SoundLibraryPanel::on_drumkitExportAction()
{
	SoundLibraryExportDialog exportDialog( this );
	exportDialog.exec();
}



void SoundLibraryPanel::on_drumkitPropertiesAction()
{
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);

	T<Drumkit>::shared_ptr drumkitInfo;

	// find the drumkit in the list
	for ( uint i = 0; i < __system_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr pInfo = __system_drumkit_info_list[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	for ( uint i = 0; i < __user_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr pInfo = __user_drumkit_info_list[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}

	assert( drumkitInfo );

	QString sPreDrumkitName = g_engine->getCurrentDrumkitname();

	T<Drumkit>::shared_ptr preDrumkitInfo;
	

	// find the drumkit in the list
	for ( uint i = 0; i < __system_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr prInfo = __system_drumkit_info_list[i];
		if ( prInfo->getName() == sPreDrumkitName ) {
			preDrumkitInfo = prInfo;
			break;
		}
	}
	for ( uint i = 0; i < __user_drumkit_info_list.size(); i++ ) {
		T<Drumkit>::shared_ptr prInfo = __user_drumkit_info_list[i];
		if ( prInfo->getName() == sPreDrumkitName ) {
			preDrumkitInfo = prInfo;
			break;
		}
	}

	if ( preDrumkitInfo == NULL ){
		QMessageBox::warning( this, "Composite", QString( "The current loaded song missing his soundlibrary.\nPlease load a existing soundlibrary first") );
		return;
	}
	assert( preDrumkitInfo );
	
	//open the soundlibrary save dialog 
	SoundLibraryPropertiesDialog dialog( this , drumkitInfo, preDrumkitInfo );
	dialog.exec();
}



void SoundLibraryPanel::on_instrumentDeleteAction()
{
	QMessageBox::warning( this, "Composite", QString( "Not implemented yet.") );
	ERRORLOG( "[on_instrumentDeleteAction] not implemented yet" );
}

void SoundLibraryPanel::on_songLoadAction()
{
	QString songName = __sound_library_tree->currentItem()->text( 0 );
	QString sDirectory = g_engine->get_preferences()->getDataDirectory()  + "songs";

	QString sFilename = sDirectory + "/" + songName + ".h2song";
	

	Engine *engine = g_engine;
	engine->get_transport()->stop();

	Tritium::LocalFileMng mng(g_engine);
	T<Song>::shared_ptr pSong = Song::load( g_engine, sFilename );
	if ( ! pSong ) {
		QMessageBox::information( this, "Composite", trUtf8("Error loading song.") );
		return;
	}

	// add the new loaded song in the "last used song" vector
	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();

	std::vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), sFilename );
	pPref->setRecentFiles( recentFiles );

	CompositeApp* h2app = CompositeApp::get_instance();

	h2app->setSong( pSong );

	//updateRecentUsedSongList();
	engine->setSelectedPatternNumber( 0 );
}



void SoundLibraryPanel::on_patternLoadAction()
{
	LocalFileMng mng(g_engine);

	QString patternName = __sound_library_tree->currentItem()->text( 0 ) + ".h2pattern";
	QString drumkitname = __sound_library_tree->currentItem()->toolTip ( 0 );
	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();
	
	QString sDirectory;

	std::vector<QString> patternDirList = mng.getPatternDirList();

	for (uint i = 0; i < patternDirList.size(); ++i) {
		QString absPath =  patternDirList[i];
		mng.getPatternList( absPath );
	}

	std::vector<QString> allPatternDirList = mng.getallPatternList();

	for (uint i = 0; i < allPatternDirList.size(); ++i) {
		QString testName = allPatternDirList[i];
		if( testName.contains( patternName ) && testName.contains( drumkitname )){
			sDirectory = allPatternDirList[i];		
		} 
	}

	T<Pattern>::shared_ptr err = mng.loadPattern (sDirectory );

	if ( err == 0 ) {
		ERRORLOG( "Error loading the pattern" );
	}
	else {
		T<Pattern>::shared_ptr pNewPattern = err;
		pPatternList->add ( pNewPattern );
		song->set_modified( true );
	}

	CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
}


void SoundLibraryPanel::on_patternDeleteAction()
{
	LocalFileMng mng(g_engine);

	QString patternName = __sound_library_tree->currentItem()->text( 0 ) + ".h2pattern";
	QString drumkitname = __sound_library_tree->currentItem()->toolTip ( 0 );
	
	QString sDirectory = "";

	std::vector<QString> patternDirList = mng.getPatternDirList();

	for (uint i = 0; i < patternDirList.size(); ++i) {
		QString absPath =  patternDirList[i];
		mng.getPatternList( absPath );
	}

	std::vector<QString> allPatternDirList = mng.getallPatternList();

	for (uint i = 0; i < allPatternDirList.size(); ++i) {
		QString testName = allPatternDirList[i];
		if( testName.contains( patternName ) && testName.contains( drumkitname )){
			sDirectory = allPatternDirList[i];		
		} 
	}

	int res = QMessageBox::information( this, "Composite", tr( "Warning, the selected pattern will be deleted from disk.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( res == 1 ) {
		return;
	}

	QFile rmfile(sDirectory );
	bool err = rmfile.remove();
	if ( err == false ) {
		ERRORLOG( "Error removing the pattern" );
	}

	test_expandedItems();
	updateDrumkitList();
}


void SoundLibraryPanel::test_expandedItems()
{
	assert( __sound_library_tree );
	__expand_songs_list = __sound_library_tree->isItemExpanded( __song_item );
	__expand_pattern_list = __sound_library_tree->isItemExpanded( __pattern_item );
	g_engine->get_preferences()->__expandSongItem = __expand_songs_list;
	g_engine->get_preferences()->__expandPatternItem = __expand_pattern_list;
	//ERRORLOG( QString("songs %1 patterns %2").arg(__expand_songs_list).arg(__expand_pattern_list) );
}
