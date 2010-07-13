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

#include <assert.h>
#include <algorithm>
#include <memory>

#include <Tritium/Song.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Pattern.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>
using namespace Tritium;


#include "SongEditor.hpp"
#include "SongEditorPanel.hpp"
#include "SoundLibrary/SoundLibraryPanel.hpp"
#include "../PatternEditor/PatternEditorPanel.hpp"
#include "../CompositeApp.hpp"
#include "../InstrumentRack.hpp"
#include "../widgets/Button.hpp"
#include "../PatternFillDialog.hpp"
#include "../PatternPropertiesDialog.hpp"
#include "../SongPropertiesDialog.hpp"
#include "../Skin.hpp"
#include <Tritium/LocalFileMng.hpp>


using namespace std;

SongEditor::SongEditor( QWidget *parent )
 : QWidget( parent )
 , m_bSequenceChanged( true )
 , m_bIsMoving( false )
 , m_bShowLasso( false )
{
	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy (Qt::StrongFocus);

	m_nGridWidth = 16;
	m_nGridHeight = 18;

	int m_nInitialWidth = 10 + m_nMaxPatternSequence * m_nGridWidth;
	int m_nInitialHeight = 10;

	this->resize( QSize(m_nInitialWidth, m_nInitialHeight) );

	createBackground();	// create m_backgroundPixmap pixmap

	update();
}



SongEditor::~SongEditor()
{
}



int SongEditor::getGridWidth ()
{
	return m_nGridWidth;
}



void SongEditor::setGridWidth( uint width )
{
	if ( ( SONG_EDITOR_MIN_GRID_WIDTH <= width ) && ( SONG_EDITOR_MAX_GRID_WIDTH >= width ) ) {
		m_nGridWidth = width;
		this->resize ( 10 + m_nMaxPatternSequence * m_nGridWidth, height() );
	}
}



void SongEditor::keyPressEvent ( QKeyEvent * ev )
{
	Engine *pEngine = g_engine;
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	T<Song::pattern_group_t>::shared_ptr pColumns = pEngine->getSong()->get_pattern_group_vector();

	if ( ev->key() == Qt::Key_Delete ) {
		if ( m_selectedCells.size() != 0 ) {
			g_engine->lock( RIGHT_HERE );
			// delete all selected cells
			for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				T<PatternList>::shared_ptr pColumn = (*pColumns)[ cell.x() ];
				pColumn->del(pPatternList->get( cell.y() ) );
			}
			g_engine->unlock();

			m_selectedCells.clear();
			m_bSequenceChanged = true;
			update();
		}
		return;
	}

	ev->ignore();
}



void SongEditor::mousePressEvent( QMouseEvent *ev )
{
	if ( ev->x() < 10 ) {
		return;
	}

	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)m_nGridWidth;

	if ( ev->modifiers() == Qt::ControlModifier ) {
		DEBUGLOG( "[mousePressEvent] CTRL pressed!" );
		m_bIsCtrlPressed = true;
	}
	else {
		m_bIsCtrlPressed = false;
	}

	Engine *pEngine = g_engine;
	T<Song>::shared_ptr pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	// don't lock the audio driver before checking that...
	if ( nRow >= (int)pPatternList->get_size() || nRow < 0 || nColumn < 0 ) { return; }
	g_engine->lock( RIGHT_HERE );


	SongEditorActionMode actionMode = CompositeApp::get_instance()->getSongEditorPanel()->getActionMode();
	if ( actionMode == SELECT_ACTION ) {

		bool bOverExistingPattern = false;
		for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
			QPoint cell = m_selectedCells[ i ];
			if ( cell.x() == nColumn && cell.y() == nRow ) {
				bOverExistingPattern = true;
				break;
			}
		}

		if ( bOverExistingPattern ) {
			// MOVE PATTERNS
//			DEBUGLOG( "[mousePressEvent] Move patterns" );
			m_bIsMoving = true;
			m_bShowLasso = false;
			m_movingCells = m_selectedCells;

			m_clickPoint.setX( nColumn );
			m_clickPoint.setY( nRow );
		}
		else {
//			DEBUGLOG( "[mousePressEvent] Select patterns" );
			// select patterns
			m_bShowLasso = true;
			m_lasso.setCoords( ev->x(), ev->y(), ev->x(), ev->y() );
			setCursor( QCursor( Qt::CrossCursor ) );
			m_selectedCells.clear();
			m_selectedCells.push_back( QPoint( nColumn, nRow ) );
		}
	}
	else if ( actionMode == DRAW_ACTION ) {
		T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get( nRow );
		T<Song::pattern_group_t>::shared_ptr pColumns = pSong->get_pattern_group_vector();	// E' la lista di "colonne" di pattern
		if ( nColumn < (int)pColumns->size() ) {
			T<PatternList>::shared_ptr pColumn = ( *pColumns )[ nColumn ];

			bool bFound = false;
			unsigned nColumnIndex = 0;
			for ( nColumnIndex = 0; nColumnIndex < pColumn->get_size(); nColumnIndex++) {
				if ( pColumn->get( nColumnIndex ) == pPattern ) { // il pattern e' gia presente
					bFound = true;
					break;
				}
			}

			if ( bFound ) {
				// DELETE PATTERN
//				DEBUGLOG( "[mousePressEvent] delete pattern" );
				pColumn->del( nColumnIndex );

				// elimino le colonne vuote
				for ( int i = pColumns->size() - 1; i >= 0; i-- ) {
					T<PatternList>::shared_ptr pColumn = ( *pColumns )[ i ];
					if ( pColumn->get_size() == 0 ) {
						pColumns->erase( pColumns->begin() + i );
						pColumn.reset();
					}
					else {
						break;
					}
				}
			}
			else {
				if ( nColumn < (int)pColumns->size() ) {
					// ADD PATTERN
//					DEBUGLOG( "[mousePressEvent] add pattern" );
					m_selectedCells.clear();
					pColumn->add( pPattern );
				}
			}
		}
		else {
			// ADD PATTERN (with spaces..)
			m_selectedCells.clear();
			int nSpaces = nColumn - pColumns->size();
//			DEBUGLOG( "[mousePressEvent] add pattern (with " + to_string( nSpaces ) + " spaces)" );

			T<PatternList>::shared_ptr pColumn( new PatternList() );
			pColumns->push_back( pColumn );

			for ( int i = 0; i < nSpaces; i++ ) {
				pColumn.reset( new PatternList() );
				pColumns->push_back( pColumn );
			}
			pColumn->add( pPattern );
		}
		pSong->set_modified( true );
	}

	g_engine->unlock();

	// update
	m_bSequenceChanged = true;
	update();
}



void SongEditor::mouseMoveEvent(QMouseEvent *ev)
{
	int nRow = ev->y() / m_nGridHeight;
	int nColumn = ( (int)ev->x() - 10 ) / (int)m_nGridWidth;
	PatternList *pPatternList = g_engine->getSong()->get_pattern_list();
	T<Song::pattern_group_t>::shared_ptr pColumns = g_engine->getSong()->get_pattern_group_vector();

	if ( m_bIsMoving ) {
//		WARNINGLOG( "[mouseMoveEvent] Move patterns not implemented yet" );

		int nRowDiff = nRow  - m_clickPoint.y();
		int nColumnDiff = nColumn - m_clickPoint.x();

//		DEBUGLOG( "[mouseMoveEvent] row diff: "+ to_string( nRowDiff ) );
//		DEBUGLOG( "[mouseMoveEvent] col diff: "+ to_string( nColumnDiff ) );

		for ( int i = 0; i < (int)m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			m_movingCells[ i ].setX( m_selectedCells[ i ].x() + nColumnDiff );
			m_movingCells[ i ].setY( m_selectedCells[ i ].y() + nRowDiff );
		}

		m_bSequenceChanged = true;
		update();
		return;
	}

	if ( m_bShowLasso ) {
		// SELECTION
		setCursor( QCursor( Qt::CrossCursor ) );
		int x = ev->x();
		int y = ev->y();
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
		m_lasso.setBottomRight( QPoint( x, y ) );

		// aggiorno la lista di celle selezionate
		m_selectedCells.clear();

		int nStartColumn = (int)( ( m_lasso.left() - 10.0 ) / m_nGridWidth );
		int nEndColumn = nColumn;
		if ( nStartColumn > nEndColumn ) {
			int nTemp = nEndColumn;
			nEndColumn = nStartColumn;
			nStartColumn = nTemp;
		}

		int nStartRow = m_lasso.top() / m_nGridHeight;
		int nEndRow = nRow;
		if ( nStartRow > nEndRow ) {
			int nTemp = nEndRow;
			nEndRow = nStartRow;
			nStartRow = nTemp;
		}

		for ( int nRow = nStartRow; nRow <= nEndRow; nRow++ ) {
			for ( int nCol = nStartColumn; nCol <= nEndColumn; nCol++ ) {
				if ( nRow >= (int)pPatternList->get_size() || nRow < 0 || nCol < 0 ) {
					return;
				}
				T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get( nRow );

				if ( nCol < (int)pColumns->size() ) {
					T<PatternList>::shared_ptr pColumn = ( *pColumns )[ nCol ];

					for ( uint i = 0; i < pColumn->get_size(); i++) {
						if ( pColumn->get(i) == pPattern ) { // esiste un pattern in questa posizione
							m_selectedCells.push_back( QPoint( nCol, nRow ) );
						}
					}
				}
			}
		}

		m_bSequenceChanged = true;
		update();
	}

}



void SongEditor::mouseReleaseEvent( QMouseEvent * /*ev*/ )
{
	Engine *pEngine = g_engine;

	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	T<Song::pattern_group_t>::shared_ptr pColumns = pEngine->getSong()->get_pattern_group_vector();

	if ( m_bIsMoving ) {	// fine dello spostamento dei pattern
		g_engine->lock( RIGHT_HERE );
		// create the new patterns
		for ( uint i = 0; i < m_movingCells.size(); i++ ) {
			QPoint cell = m_movingCells[ i ];
			if ( cell.x() < 0 || cell.y() < 0 || cell.y() >= (int)pPatternList->get_size() ) {
				// skip
				continue;
			}
			// aggiungo un pattern per volta
			T<PatternList>::shared_ptr pColumn;
			if ( cell.x() < (int)pColumns->size() ) {
				pColumn = (*pColumns)[ cell.x() ];
			}
			else {
				// creo dei patternlist vuoti
				int nSpaces = cell.x() - pColumns->size();
				for ( int i = 0; i <= nSpaces; i++ ) {
					pColumn.reset( new PatternList() );
					pColumns->push_back( pColumn );
				}
			}
			pColumn->add( pPatternList->get( cell.y() ) );
		}

		if ( m_bIsCtrlPressed ) {	// COPY
		}
		else {	// MOVE
			// remove the old patterns
			for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
				QPoint cell = m_selectedCells[ i ];
				T<PatternList>::shared_ptr pColumn;
				if ( cell.x() < (int)pColumns->size() ) {
					pColumn = (*pColumns)[ cell.x() ];
				}
				else {
					pColumn.reset( new PatternList() );
					pColumns->push_back( pColumn );
				}
				pColumn->del(pPatternList->get( cell.y() ) );
			}
		}

		// remove the empty patternlist at the end of the song
		for ( int i = pColumns->size() - 1; i != 0 ; i-- ) {
			T<PatternList>::shared_ptr pList = (*pColumns)[ i ];
			int nSize = pList->get_size();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				pList.reset();
			}
			else {
				break;
			}
		}


		pEngine->getSong()->set_modified( true );
		g_engine->unlock();

		m_bIsMoving = false;
		m_movingCells.clear();
		m_selectedCells.clear();
	}

	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bShowLasso = false;
	m_bSequenceChanged = true;
	m_bIsCtrlPressed = false;
	update();
}



void SongEditor::paintEvent( QPaintEvent *ev )
{
/*	DEBUGLOG(
			"[paintEvent] x: " + to_string( ev->rect().x() ) +
			" y: " + to_string( ev->rect().y() ) +
			" w: " + to_string( ev->rect().width() ) +
			" h: " + to_string( ev->rect().height() )
	);
*/

	// ridisegno tutto solo se sono cambiate le note
	if (m_bSequenceChanged) {
		m_bSequenceChanged = false;
		drawSequence();
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pSequencePixmap, ev->rect() );

	if ( m_bShowLasso ) {
		QPen pen( Qt::white );
		pen.setStyle( Qt::DotLine );
		painter.setPen( pen );
		painter.drawRect( m_lasso );
	}
}



void SongEditor::createBackground()
{
	UIStyle *pStyle = g_engine->get_preferences()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );
	QColor linesColor( pStyle->m_songEditor_lineColor.getRed(), pStyle->m_songEditor_lineColor.getGreen(), pStyle->m_songEditor_lineColor.getBlue() );

	Engine *pEngine = g_engine;
	T<Song>::shared_ptr pSong = pEngine->getSong();

	uint nPatterns = pSong->get_pattern_list()->get_size();

	static int nOldHeight = -1;
	int nNewHeight = m_nGridHeight * nPatterns;

	if (nOldHeight != nNewHeight) {	
		// cambiamento di dimensioni...
		if (nNewHeight == 0) {
			nNewHeight = 1;	// the pixmap should not be empty
		}

		m_pBackgroundPixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		m_pSequencePixmap = new QPixmap( width(), nNewHeight );	// initialize the pixmap
		this->resize( QSize( width(), nNewHeight ) );
	}

	m_pBackgroundPixmap->fill( alternateRowColor );

	QPainter p( m_pBackgroundPixmap );
	p.setPen( linesColor );

/*	// sfondo per celle scure (alternato)
	for (uint i = 0; i < nPatterns; i++) {
		if ( ( i % 2) != 0) {
			uint y = m_nGridHeight * i;
			p.fillRect ( 0, y, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
			p.fillRect ( 0, y + 2, m_nMaxPatternSequence * m_nGridWidth, m_nGridHeight - 4, alternateRowColor );
			p.fillRect ( 0, y + m_nGridHeight - 2, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
		}
	}
*/
	// celle...
	p.setPen( linesColor );

	// vertical lines
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;
		int x1 = x;
		int x2 = x + m_nGridWidth;

		p.drawLine( x1, 0, x1, m_nGridHeight * nPatterns );
		p.drawLine( x2, 0, x2, m_nGridHeight * nPatterns );
	}

	p.setPen( linesColor );
	// horizontal lines
	for (uint i = 0; i < nPatterns; i++) {
		uint y = m_nGridHeight * i;

		int y1 = y + 2;
		int y2 = y + m_nGridHeight - 2;

		p.drawLine( 0, y1, (m_nMaxPatternSequence * m_nGridWidth), y1 );
		p.drawLine( 0, y2, (m_nMaxPatternSequence * m_nGridWidth), y2 );
	}


	p.setPen( backgroundColor );
	// horizontal lines (erase..)
	for (uint i = 0; i < nPatterns + 1; i++) {
		uint y = m_nGridHeight * i;

		p.fillRect( 0, y, m_nMaxPatternSequence * m_nGridWidth, 2, backgroundColor );
		p.drawLine( 0, y + m_nGridHeight - 1, m_nMaxPatternSequence * m_nGridWidth, y + m_nGridHeight - 1 );
	}

	//~ celle
	m_bSequenceChanged = true;
}

void SongEditor::cleanUp(){

	delete m_pBackgroundPixmap;
	delete m_pSequencePixmap;
}

void SongEditor::drawSequence()
{
	QPainter p;
	p.begin( m_pSequencePixmap );
	p.drawPixmap( rect(), *m_pBackgroundPixmap, rect() );
	p.end();

	T<Song>::shared_ptr song = g_engine->getSong();
	PatternList *patList = song->get_pattern_list();
	T<Song::pattern_group_t>::shared_ptr pColumns = song->get_pattern_group_vector();
	uint listLength = patList->get_size();
	for (uint i = 0; i < pColumns->size(); i++) {
		T<PatternList>::shared_ptr pColumn = (*pColumns)[ i ];

		for (uint nPat = 0; nPat < pColumn->get_size(); ++nPat) {
			T<Tritium::Pattern>::shared_ptr pat = pColumn->get( nPat );

			int position = -1;
			// find the position in pattern list
			for (uint j = 0; j < listLength; j++) {
				T<Tritium::Pattern>::shared_ptr pat2 = patList->get( j );
				if (pat == pat2) {
					position = j;
					break;
				}
			}
			if (position == -1) {
				DEBUGLOG( QString("[drawSequence] position == -1, group = %1").arg( i ) );
			}
			drawPattern( i, position );
		}
	}

	// Moving cells
	p.begin( m_pSequencePixmap );
//	p.setRasterOp( Qt::XorROP );

// comix: this composition mode seems to be not available on Mac
	p.setCompositionMode( QPainter::CompositionMode_Xor );
	QPen pen( Qt::gray );
	pen.setStyle( Qt::DotLine );
	p.setPen( pen );
	for ( uint i = 0; i < m_movingCells.size(); i++ ) {
		int x = 10 + m_nGridWidth * ( m_movingCells[ i ] ).x();
		int y = m_nGridHeight * ( m_movingCells[ i ] ).y();

		QColor patternColor;
		patternColor.setRgb( 255, 255, 255 );
		p.fillRect( x + 2, y + 4, m_nGridWidth - 3, m_nGridHeight - 7, patternColor );
	}

}



void SongEditor::drawPattern( int pos, int number )
{
	T<Preferences>::shared_ptr pref = g_engine->get_preferences();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QPainter p( m_pSequencePixmap );
	QColor patternColor( pStyle->m_songEditor_pattern1Color.getRed(), pStyle->m_songEditor_pattern1Color.getGreen(), pStyle->m_songEditor_pattern1Color.getBlue() );

	bool bIsSelected = false;
	for ( uint i = 0; i < m_selectedCells.size(); i++ ) {
		QPoint point = m_selectedCells[ i ];
		if ( point.x() == pos && point.y() == number ) {
			bIsSelected = true;
			break;
		}
	}

	if ( bIsSelected ) {
		patternColor = patternColor.dark( 130 );
	}

	int x = 10 + m_nGridWidth * pos;
	int y = m_nGridHeight * number;

// 	p.setPen( patternColor.light( 120 ) );  // willie - For the bevel - haven't yet figured how it's supposed to work...
	p.fillRect( x + 1, y + 3, m_nGridWidth - 1, m_nGridHeight - 5, patternColor );
}





// :::::::::::::::::::





SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent )
 , EventListener()
 , m_pBackgroundPixmap( NULL )
{
	m_nWidth = 200;
	m_nGridHeight = 18;
	setAttribute(Qt::WA_NoBackground);
	
	setAcceptDrops(true);

	line = new QLineEdit( "Inline Pattern Name", this );
	line->setFrame( false );
	line->hide();
	connect( line, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( line, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_labelBackgroundLight.load( Skin::getImagePath() + "/songEditor/songEditorLabelBG.png" );
	m_labelBackgroundDark.load( Skin::getImagePath() + "/songEditor/songEditorLabelABG.png" );
	m_labelBackgroundSelected.load( Skin::getImagePath() + "/songEditor/songEditorLabelSBG.png" );
	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( trUtf8("Edit"),  this, SLOT( patternPopup_edit() ) );
	m_pPatternPopup->addAction( trUtf8("Copy"),  this, SLOT( patternPopup_copy() ) );
	m_pPatternPopup->addAction( trUtf8("Delete"),  this, SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( trUtf8("Fill/Clear ..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->addAction( trUtf8("Properties"),  this, SLOT( patternPopup_properties() ) );
	m_pPatternPopup->addAction( trUtf8("Load Pattern"),  this, SLOT( patternPopup_load() ) );
	m_pPatternPopup->addAction( trUtf8("Save Pattern"),  this, SLOT( patternPopup_save() ) );

	CompositeApp::get_instance()->addEventListener( this );

	createBackground();
	update();
}



SongEditorPatternList::~SongEditorPatternList()
{
}


void SongEditorPatternList::patternChangedEvent() {
	createBackground();
	update();
}


/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);

	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	if ( row >= (int)patternList->get_size() ) {
		return;
	}

	if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ){
		togglePattern( row );
	} else {
		engine->setSelectedPatternNumber( row );
		if (ev->button() == Qt::RightButton)  {
	/*
			if ( song->getMode() == Song::PATTERN_MODE ) {
	
				PatternList *pCurrentPatternList = engine->getCurrentPatternList();
				if ( pCurrentPatternList->get_size() == 0 ) {
					// nessun pattern e' attivo. seleziono subito questo.
					pCurrentPatternList->add( patternList->get( row ) );
				}
				else {
					engine->setNextPattern( row );
				}
			}
	*/
			m_pPatternPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
		}
	}

	createBackground();
	update();
}


///
/// Start/stop playing a pattern in "pattern mode"
///
void SongEditorPatternList::togglePattern( int row ) {

	Engine *engine = g_engine;
/*	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();*/

// 	PatternList *pCurrentPatternList = engine->getCurrentPatternList();

// 	bool isPatternPlaying = false;
	engine->sequencer_setNextPattern( row, false, true );

// 	for ( uint i = 0; i < pCurrentPatternList->get_size(); ++i ) {
// 		if ( pCurrentPatternList->get( i ) == patternList->get( row ) ) {
// 			// the pattern is already playing, stop it!
// 			isPatternPlaying = true;
// 			break;
// 		}
// 	}
//
// 	if ( isPatternPlaying ) {
// 		//pCurrentPatternList->del( patternList->get( row ) );
// 		engine->sequencer_setNextPattern( row, false, true );	// remove from the playing pattern list
// 	}
// 	else {
// 		// the pattern is not playing, add it to the list
// 		//pCurrentPatternList->add( patternList->get( row ) );
// 		engine->sequencer_setNextPattern( row, true, false );	// add to the playing pattern list
// 	}

	createBackground();
	update();
}


void SongEditorPatternList::mouseDoubleClickEvent( QMouseEvent *ev )
{
	int row = (ev->y() / m_nGridHeight);
	inlineEditPatternName( row );
}

void SongEditorPatternList::inlineEditPatternName( int row )
{
	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	if ( row >= (int)patternList->get_size() ) {
		return;
	}
	patternBeingEdited = patternList->get( row );
	line->setGeometry( 23, row * m_nGridHeight , m_nWidth - 23, m_nGridHeight  );
	line->setText( patternBeingEdited->get_name() );
	line->selectAll();
	line->show();
	line->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( patternBeingEdited != NULL );
	if ( PatternPropertiesDialog::nameCheck( line->text() ) )
	{
		patternBeingEdited->set_name( line->text() );
		g_engine->getSong()->set_modified( true );
		g_engine->get_event_queue()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		createBackground();
		update();
	}
// 	patternBeingEdited = NULL;
}


void SongEditorPatternList::inlineEditingFinished()
{
	patternBeingEdited.reset();
	line->hide();
}


void SongEditorPatternList::paintEvent( QPaintEvent *ev )
{
	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, ev->rect() );
}



void SongEditorPatternList::updateEditor()
{
	if(!isVisible()) {
		return;
	}

	update();
}



void SongEditorPatternList::createBackground()
{
	T<Preferences>::shared_ptr pref = g_engine->get_preferences();
	UIStyle *pStyle = pref->getDefaultUIStyle();
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );

	QString family = pref->getApplicationFontFamily();
	int size = pref->getApplicationFontPointSize();
	QFont textFont( family, size );

	QFont boldTextFont( textFont);
	boldTextFont.setPointSize(10);
	boldTextFont.setBold( true );

	Engine *pEngine = g_engine;
	T<Song>::shared_ptr pSong = pEngine->getSong();
	int nPatterns = pSong->get_pattern_list()->get_size();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();

	static int oldHeight = -1;
	int newHeight = m_nGridHeight * nPatterns;

	if (oldHeight != newHeight) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nWidth, newHeight );	// initialize the pixmap
		this->resize( m_nWidth, newHeight );
	}
	m_pBackgroundPixmap->fill( Qt::black );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( boldTextFont );

	for ( int i = 0; i < nPatterns; i++ ) {
		uint y = m_nGridHeight * i;
		if ( i == nSelectedPattern ) {
			p.drawPixmap( QPoint( 0, y ), m_labelBackgroundSelected );
		}
		else {
			if ( ( i % 2) == 0 ) {
				p.drawPixmap( QPoint( 0, y ), m_labelBackgroundDark );
			}
			else {
				p.drawPixmap( QPoint( 0, y ), m_labelBackgroundLight );
			}
		}
	}

	T<PatternList>::shared_ptr pCurrentPatternList = pEngine->getCurrentPatternList();

	/// paint the foreground (pattern name etc.)
	for ( int i = 0; i < nPatterns; i++ ) {
		T<Tritium::Pattern>::shared_ptr pPattern = pSong->get_pattern_list()->get(i);
		//uint y = m_nGridHeight * i;

		// Text
		bool bNext = false, bActive = false;
/*		for (uint j = 0; j < pCurrentPatternList->get_size(); j++) {
			if ( pPattern == pCurrentPatternList->get(j) ) {
				bActive = true;
				break;
			}
		}*/
		if ( pCurrentPatternList->index_of( pPattern ) != -1 ) bActive = true;
		if ( pEngine->getNextPatterns()->index_of( pPattern ) != -1 ) bNext = true;

		if ( i == nSelectedPattern ) {
			p.setPen( QColor( 0,0,0 ) );
		}
		else {
			p.setPen( textColor );
		}

		uint text_y = i * m_nGridHeight;
		if ( bNext ) {
			p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_off_Pixmap );
		}
		else if (bActive) {
//			p.drawText( 5, text_y - 1 - m_nGridHeight, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, ">" );
		
			//mark active pattern with triangular
			if( ! pref->patternModePlaysSelected() ){
				p.drawPixmap( QPoint( 5, text_y + 3 ), m_playingPattern_on_Pixmap );
			}
		}


		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2, Qt::AlignVCenter, pPattern->get_name() );
	}

}



void SongEditorPatternList::patternPopup_load()
{

	Engine *engine = g_engine;
	int tmpselectedpatternpos = engine->getSelectedPatternNumber();
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();
	T<Instrument>::shared_ptr instr = engine->get_sampler()->get_instrument_list()->get( 0 );
	assert( instr );
	
	QDir dirPattern( g_engine->get_preferences()->getDataDirectory() + "/patterns" );
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Pattern (*.h2pattern)") );
	fd->setDirectory(dirPattern );

	fd->setWindowTitle( trUtf8( "Open Pattern" ) );

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}
	else
	{
		return;
	}

	LocalFileMng mng(g_engine);
	LocalFileMng fileMng(g_engine);
	T<Pattern>::shared_ptr err = fileMng.loadPattern( filename );
	if ( err == 0 ) {
		ERRORLOG( "Error loading the pattern" );
	}else{
		T<Pattern>::shared_ptr pNewPattern = err;
		pPatternList->add( pNewPattern );
		song->set_modified( true );
		createBackground();
		update();
	}

	int listsize = pPatternList->get_size();
	engine->setSelectedPatternNumber( listsize -1 );
	T<Pattern>::shared_ptr pTemp = pPatternList->get( engine->getSelectedPatternNumber() );
	pPatternList->replace( pPatternList->get( tmpselectedpatternpos ), listsize -1);
	pPatternList->replace( pTemp, tmpselectedpatternpos );
	listsize = pPatternList->get_size();
	engine->setSelectedPatternNumber( listsize -1 );
	patternPopup_delete();
	engine->setSelectedPatternNumber( tmpselectedpatternpos );
	CompositeApp::get_instance()->getSongEditorPanel()->updateAll();

}



void SongEditorPatternList::patternPopup_save()
{	
	Engine *engine = g_engine;
	int nSelectedPattern = engine->getSelectedPatternNumber();
	T<Song>::shared_ptr song = engine->getSong();
	T<Pattern>::shared_ptr pat = song->get_pattern_list()->get( nSelectedPattern );

	QString patternname = pat->get_name();

	LocalFileMng fileMng(g_engine);
	int err = fileMng.savePattern( song , nSelectedPattern, patternname, patternname, 1 );
	if ( err == 1 ) {
		int res = QMessageBox::information( this, "Composite", tr( "The pattern-file exists. \nOverwrite the existing pattern?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( res == 0 ) {
			int err2 = fileMng.savePattern( song , nSelectedPattern, patternname, patternname, 3 );
			if( err2 == 1){
				ERRORLOG( "Error saving the pattern" );
				return;
			} //if err2
		}else{ // res cancel 
			return;
		} //if res	
	} //if err
	

#ifdef WIN32
	Sleep ( 10 );
#else
	usleep ( 10000 );
#endif 
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	CompositeApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}



void SongEditorPatternList::patternPopup_edit()
{
	CompositeApp::get_instance()->getPatternEditorPanel()->show();
	CompositeApp::get_instance()->getPatternEditorPanel()->setFocus();
}



void SongEditorPatternList::patternPopup_properties()
{
	Engine *engine = g_engine;
	T<Song>::shared_ptr song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();

	int nSelectedPattern = engine->getSelectedPatternNumber();
	T<Tritium::Pattern>::shared_ptr pattern = patternList->get( nSelectedPattern );

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog(this, pattern, false);
	if (dialog->exec() == QDialog::Accepted) {
// 		Engine *engine = g_engine;
// 		Song *song = engine->getSong();
		song->set_modified( true );
		g_engine->get_event_queue()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
		createBackground();
		update();
	}
	delete dialog;
	dialog = NULL;
}



void SongEditorPatternList::patternPopup_delete()
{
	Engine *pEngine = g_engine;

//	int state = engine->get_transport()->get_state();
// 	// per ora non lascio possibile la cancellazione del pattern durante l'esecuzione
// 	// da togliere quando correggo il bug
//         if (state == PLAYING) {
//                 QMessageBox::information( this, "Composite", trUtf8("Can't delete the pattern while the audio engine is playing"));
//                 return;
//         }

	if ( pEngine->getSong()->get_mode() == Song::PATTERN_MODE ) {
		pEngine->sequencer_setNextPattern( -1, false, false );	// reimposto il prossimo pattern a NULL, altrimenti viene scelto quello che sto distruggendo ora...
	}

//	pEngine->sequencer_stop();

// "lock engine" I am not sure, but think this is unnecessarily. -wolke-
//	g_engine->lock( RIGHT_HERE );

	T<Song>::shared_ptr song = pEngine->getSong();
	PatternList *pSongPatternList = song->get_pattern_list();

	T<Pattern>::shared_ptr pattern = pSongPatternList->get( pEngine->getSelectedPatternNumber() );
	DEBUGLOG( QString("[patternPopup_delete] Delete pattern: %1 @%2").arg(pattern->get_name()).arg( (long)pattern.get() ) );
	pSongPatternList->del(pattern);

	T<Song::pattern_group_t>::shared_ptr patternGroupVect = song->get_pattern_group_vector();

	uint i = 0;
	while (i < patternGroupVect->size() ) {
		T<PatternList>::shared_ptr list = (*patternGroupVect)[i];

		uint j = 0;
		while ( j < list->get_size() ) {
			T<Tritium::Pattern>::shared_ptr pOldPattern = list->get( j );
			if (pOldPattern == pattern ) {
				list->del( j );
				continue;
			}
			j++;
		}
// 		for (uint j = 0; j < list->get_size(); j++) {
// 			Pattern *pOldPattern = list->get( j );
// 			if (pOldPattern == pattern ) {
// 				list->del( j );
// 			}
// 		}

/*		if (list->get_size() == 0 ) {
			patternGroupVect->erase( patternGroupVect->begin() + i );
			delete list;
			list = NULL;
		}
		else {
*/			i++;
//		}
	}


	T<PatternList>::shared_ptr list = pEngine->getCurrentPatternList();
	list->del( pattern );
	// se esiste, seleziono il primo pattern
	if ( pSongPatternList->get_size() > 0 ) {
		T<Tritium::Pattern>::shared_ptr pFirstPattern = pSongPatternList->get( 0 );
		list->add( pFirstPattern );
		// Cambio due volte...cosi' il pattern editor viene costretto ad aggiornarsi
		pEngine->setSelectedPatternNumber( -1 );
		pEngine->setSelectedPatternNumber( 0 );
	}
	else {
		// there's no patterns..	
		T<Pattern>::shared_ptr emptyPattern = Pattern::get_empty_pattern();
		emptyPattern->set_name( trUtf8("Pattern 1") );
		emptyPattern->set_category( trUtf8("not_categorized") );
		pSongPatternList->add( emptyPattern );
		pEngine->setSelectedPatternNumber( -1 );
		pEngine->setSelectedPatternNumber( 0 );
	}

	pattern.reset();

	song->set_modified( true );

// "unlock" I am not sure, but think this is unnecessarily. -wolke-
//	g_engine->unlock();

	( CompositeApp::get_instance() )->getSongEditorPanel()->updateAll();
}



void SongEditorPatternList::patternPopup_copy()
{
	Engine *pEngine = g_engine;
	T<Song>::shared_ptr pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get( nSelectedPattern );

	T<Tritium::Pattern>::shared_ptr pNewPattern = pPattern->copy();
	pPatternList->add( pNewPattern );

	// rename the copied pattern
	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, pNewPattern, true );
	if ( dialog->exec() == QDialog::Accepted ) {
		pSong->set_modified( true );
		pEngine->setSelectedPatternNumber(pPatternList->get_size() - 1);	// select the last pattern (the copied one)
		if (pSong->get_mode() == Song::PATTERN_MODE) {
			pEngine->sequencer_setNextPattern( pPatternList->get_size() - 1, false, false );	// select the last pattern (the new copied pattern)
		}
	}
	else {
		pPatternList->del( pNewPattern );
		pNewPattern.reset();
	}
	delete dialog;

	CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
}

void SongEditorPatternList::patternPopup_fill()
{
	Engine *pEngine = g_engine;
	int nSelectedPattern = pEngine->getSelectedPatternNumber();
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );
	SongEditorPanel *pSEPanel = CompositeApp::get_instance()->getSongEditorPanel();


	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {
		fillRangeWithPattern(&range, nSelectedPattern);
		pSEPanel->updateAll();
	}

	delete dialog;

}


void SongEditorPatternList::fillRangeWithPattern(FillRange* pRange, int nPattern)
{
	Engine *pEngine = g_engine;
	g_engine->lock( RIGHT_HERE );


	T<Song>::shared_ptr pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get( nPattern );
	T<Song::pattern_group_t>::shared_ptr pColumns = pSong->get_pattern_group_vector();	// E' la lista di "colonne" di pattern
	T<PatternList>::shared_ptr pColumn;

	int nColumn, nColumnIndex;
	bool bHasPattern = false;
	int fromVal = pRange->fromVal - 1;
	int toVal   = pRange->toVal;



	// Add patternlists to PatternGroupVector as necessary

	int nDelta = toVal - pColumns->size() + 1;

	for ( int i = 0; i < nDelta; i++ ) {
		pColumn.reset( new PatternList() );
		pColumns->push_back( pColumn );
	}


	// Fill or Clear each cell in range

	for ( nColumn = fromVal; nColumn < toVal; nColumn++ ) {

		// expand Pattern
		pColumn = ( *pColumns )[ nColumn ];

		bHasPattern = false;

		// check whether the pattern (and column) already exists
		for ( nColumnIndex = 0; pColumn && nColumnIndex < (int)pColumn->get_size(); nColumnIndex++) {

			if ( pColumn->get( nColumnIndex ) == pPattern ) {
				bHasPattern = true;
				break;
			}
		}

		if ( pRange->bInsert && !bHasPattern ) {       //fill
			pColumn->add( pPattern);
		}
		else if ( !pRange->bInsert && bHasPattern ) {  // clear
			pColumn->del( pPattern);
		}
	}

		// remove all the empty patternlists at the end of the song
		for ( int i = pColumns->size() - 1; i != 0 ; i-- ) {
			T<PatternList>::shared_ptr pList = (*pColumns)[ i ];
			int nSize = pList->get_size();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
				pList.reset();
			}
			else {
				break;
			}
		}
	g_engine->unlock();


	// Update
	pSong->set_modified( true );
}


///drag & drop
void SongEditorPatternList::dragEnterEvent(QDragEnterEvent *event)
{
	if ( event->mimeData()->hasFormat("text/plain") ) {
			event->acceptProposedAction();
	}
}


void SongEditorPatternList::dropEvent(QDropEvent *event)
{
	QString sText = event->mimeData()->text();

	if( sText.startsWith("Songs:") || sText.startsWith("move instrument:") ){
		event->acceptProposedAction();
		return;
	}

	if (sText.startsWith("move pattern:")) {
		Engine *engine = g_engine;
		int nSourcePattern = engine->getSelectedPatternNumber();

		int nTargetPattern = event->pos().y() / m_nGridHeight;

		if ( nSourcePattern == nTargetPattern ) {
			event->acceptProposedAction();
			return;
		}

		movePatternLine( nSourcePattern , nTargetPattern );

		event->acceptProposedAction();
	}else {


		PatternList *pPatternList = g_engine->getSong()->get_pattern_list();

		QStringList tokens = sText.split( "::" );
		QString sPatternName = tokens.at( 1 );

		int nTargetPattern = event->pos().y() / m_nGridHeight;

		LocalFileMng mng(g_engine);
		T<Pattern>::shared_ptr err = mng.loadPattern( sPatternName );
		if ( err == 0 ) {
			ERRORLOG( "Error loading the pattern" );
		}else{
			T<Pattern>::shared_ptr pNewPattern = err;
			pPatternList->add( pNewPattern );

			for (int nPatr = pPatternList->get_size() +1 ; nPatr >= nTargetPattern; nPatr--) {
				T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pNewPattern, nTargetPattern );

			g_engine->getSong()->set_modified( true );
			createBackground();
			update();
		}
		CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
		event->acceptProposedAction();
		
	}
}



void SongEditorPatternList::movePatternLine( int nSourcePattern , int nTargetPattern )
{
		Engine *engine = g_engine;

		T<Song>::shared_ptr pSong = engine->getSong();
		PatternList *pPatternList = pSong->get_pattern_list();



		// move instruments...
		T<Tritium::Pattern>::shared_ptr pSourcePattern = pPatternList->get( nSourcePattern );//Instrument *pSourceInstr = pPatternList->get(nSourcePattern);
		if ( nSourcePattern < nTargetPattern) {
			for (int nPatr = nSourcePattern; nPatr < nTargetPattern; nPatr++) {
				T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get(nPatr + 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pSourcePattern, nTargetPattern );
		}
		else {
			for (int nPatr = nSourcePattern; nPatr >= nTargetPattern; nPatr--) {
				T<Tritium::Pattern>::shared_ptr pPattern = pPatternList->get(nPatr - 1);
				pPatternList->replace( pPattern, nPatr );
			}
			pPatternList->replace( pSourcePattern, nTargetPattern );
		}
		engine->setSelectedPatternNumber( nTargetPattern );
		CompositeApp::get_instance()->getSongEditorPanel()->updateAll();
		
}


void SongEditorPatternList::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ( abs(event->pos().y() - __drag_start_position.y()) < (int)m_nGridHeight) {
		return;
	}


	QString sText = QString("move pattern:%1");

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);
	//drag->setPixmap(iconPixmap);

	pDrag->start( Qt::CopyAction | Qt::MoveAction );

	QWidget::mouseMoveEvent(event);
}

// ::::::::::::::::::::::::::



SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
{
	setAttribute(Qt::WA_NoBackground);

	m_nGridWidth = 16;

	resize( m_nInitialWidth, m_nHeight );
	setFixedHeight( m_nHeight );

	m_pBackgroundPixmap = new QPixmap( m_nInitialWidth, m_nHeight );	// initialize the pixmap

	createBackground();	// create m_backgroundPixmap pixmap

	// create tick position pixmap
	bool ok = m_tickPositionPixmap.load( Skin::getImagePath() + "/patternEditor/tickPosition.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	update();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updatePosition()));
	m_pTimer->start(200);
}



SongEditorPositionRuler::~SongEditorPositionRuler() {
	m_pTimer->stop();
}


uint SongEditorPositionRuler::getGridWidth()
{
	return m_nGridWidth;
}

void SongEditorPositionRuler::setGridWidth( uint width )
{
	if ( SONG_EDITOR_MIN_GRID_WIDTH <= width && SONG_EDITOR_MAX_GRID_WIDTH >= width )
	{
		m_nGridWidth = width;
		createBackground ();
	}
}


void SongEditorPositionRuler::createBackground()
{
	UIStyle *pStyle = g_engine->get_preferences()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_songEditor_backgroundColor.getRed(), pStyle->m_songEditor_backgroundColor.getGreen(), pStyle->m_songEditor_backgroundColor.getBlue() );
	QColor textColor( pStyle->m_songEditor_textColor.getRed(), pStyle->m_songEditor_textColor.getGreen(), pStyle->m_songEditor_textColor.getBlue() );
	QColor alternateRowColor( pStyle->m_songEditor_alternateRowColor.getRed(), pStyle->m_songEditor_alternateRowColor.getGreen(), pStyle->m_songEditor_alternateRowColor.getBlue() );

	m_pBackgroundPixmap->fill( backgroundColor );

	T<Preferences>::shared_ptr pref = g_engine->get_preferences();
	QString family = pref->getApplicationFontFamily();
	int size = pref->getApplicationFontPointSize();
	QFont font( family, size );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( font );

	QString tmp;
	for (uint i = 0; i < m_nMaxPatternSequence + 1; i++) {
		uint x = 10 + i * m_nGridWidth;
		if ( (i % 4) == 0 ) {
			p.setPen( textColor );
			tmp = QString("%1").arg( i+1 );
			p.drawText( x - m_nGridWidth, 0, m_nGridWidth * 2, height(), Qt::AlignCenter, tmp );
		}
		else {
			p.setPen( textColor );
			p.drawLine( x, 10, x, m_nHeight - 10 );
		}
	}

	p.setPen( QColor(35, 39, 51) );
	p.drawLine( 0, 0, width(), 0 );

	p.fillRect ( 0, height() - 3, width(), 2, alternateRowColor );

}



void SongEditorPositionRuler::mouseMoveEvent(QMouseEvent *ev)
{
	mousePressEvent( ev );
}



void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{
	int column = (ev->x() / m_nGridWidth);

	if ( column >= (int)g_engine->getSong()->get_pattern_group_vector()->size() ) {
		return;
	}
	
	// disabling son relocates while in pattern mode as it causes weird behaviour. (jakob lund)
	if ( g_engine->getSong()->get_mode() == Song::PATTERN_MODE ) {
		return;
	}

	int nPatternPos = g_engine->getPatternPos();
	if ( nPatternPos != column ) {
		g_engine->setPatternPos( column );
		update();
	}
}



void SongEditorPositionRuler::paintEvent( QPaintEvent *ev )
{
	if (!isVisible()) {
		return;
	}
	
	Engine *H = g_engine;

	float fPos = H->getPatternPos();

	if ( H->getCurrentPatternList()->get_size() != 0 ) {
		T<Tritium::Pattern>::shared_ptr pPattern = H->getCurrentPatternList()->get( 0 );
		fPos += (float)H->getTickPosition() / (float)pPattern->get_length();
	}
	else {
		// nessun pattern, uso la grandezza di default
		fPos += (float)H->getTickPosition() / (float)MAX_NOTES;
	}

	if ( H->getSong()->get_mode() == Song::PATTERN_MODE ) {
		fPos = -1;
	}

	QPainter painter(this);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, ev->rect() );

	if (fPos != -1) {
		uint x = (int)( 10 + fPos * m_nGridWidth - 11 / 2 );
		painter.drawPixmap( QRect( x, height() / 2, 11, 8), m_tickPositionPixmap, QRect(0, 0, 11, 8) );
	}
}



void SongEditorPositionRuler::updatePosition()
{
	update();
}


