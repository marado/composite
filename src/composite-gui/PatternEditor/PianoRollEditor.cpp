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

/*
#include "PianoRollEditor.hpp"

#include <cassert>

#include <Tritium/Engine.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Note.hpp>
#include <Tritium/Preferences.hpp>
#include <Tritium/Pattern.hpp>
#include <Tritium/Logger.hpp>
using namespace Tritium;

#include "../CompositeApp.hpp"

PianoRollEditor::PianoRollEditor( QWidget *pParent )
 : QWidget( pParent )
 , m_pPattern( NULL )
{
	DEBUGLOG( "INIT" );

	m_nRowHeight = 10;
	m_nOctaves = 8;

	setAttribute(Qt::WA_NoBackground);
	setFocusPolicy(Qt::ClickFocus);

	unsigned nGridWidth = g_engine->get_preferences()->getPatternEditorGridWidth();

	unsigned nEditorWidth = 20 + nGridWidth * ( MAX_NOTES * 4 );
	unsigned nEditorHeight = m_nOctaves * 12 * m_nRowHeight;

	m_pBackground = new QPixmap( nEditorWidth, nEditorHeight );
	m_pTemp = new QPixmap( nEditorWidth, nEditorHeight );

	resize( nEditorWidth, nEditorHeight );

	createBackground();

	CompositeApp::get_instance()->addEventListener( this );
}



PianoRollEditor::~PianoRollEditor()
{
	DEBUGLOG( "DESTROY" );
}



void PianoRollEditor::selectedInstrumentChangedEvent()
{
	drawPattern();
}



void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );

	painter.drawPixmap( ev->rect(), *m_pTemp, ev->rect() );
}



void PianoRollEditor::createBackground()
{
	DEBUGLOG( "(re)creating the background" );

	QColor backgroundColor( 0, 0, 0 );
	m_pBackground->fill( backgroundColor );


	QColor octaveColor( 230, 230, 230 );
	QColor octaveAlternateColor( 210, 210, 210 );

	unsigned start_x = 20;
	unsigned end_x = width();

	QPainter p( m_pBackground );

	for ( uint octave = 0; octave < m_nOctaves; ++octave ) {
		unsigned start_y = octave * 12 * m_nRowHeight;

		if ( octave % 2 ) {
			p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveColor );
		}
		else {
			p.fillRect( start_x, start_y, end_x - start_x, 12 * m_nRowHeight, octaveAlternateColor );
		}
	}


	// horiz lines
	for ( uint row = 0; row < ( 12 * m_nOctaves ); ++row ) {
		unsigned y = row * m_nRowHeight;
		p.drawLine( start_x, y, end_x, y );
	}

}




void PianoRollEditor::selectedPatternChangedEvent()
{
	DEBUGLOG( "updating m_pPattern pointer" );

	Engine *pEngine = g_engine;
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}
}



void PianoRollEditor::drawPattern()
{
	if ( isVisible() == false ) {
		return;
	}


	DEBUGLOG( "draw pattern" );

	QPainter p( m_pTemp );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackground, rect() );


	// for each note...
	Pattern::note_map_t::iterator pos;
	for ( pos = m_pPattern->note_map.begin(); pos != m_pPattern->note_map.end(); pos++ ) {
		//cout << "note" << endl;
		//cout << "note n: " << pos->first << endl;
		Note *note = pos->second;
		assert( note );
		drawNote( note, &p );
	}

}



void PianoRollEditor::drawNote( Note *pNote, QPainter *pPainter )
{
	int nInstrument = -1;
	InstrumentList * pInstrList = g_engine->getSong()->get_instrument_list();
	for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr == pNote->get_instrument() ) {
 			nInstrument = nInstr;
			break;
		}
	}
	if ( nInstrument == -1 ) {
		ERRORLOG( "Instrument not found..skipping note" );
		return;
	}

	if ( nInstrument != g_engine->getSelectedInstrumentNumber() ) {
		return;
	}


	int nGridWidth = g_engine->get_preferences()->getPatternEditorGridWidth();

	uint start_x = 20 + pNote->get_position() * nGridWidth;
	uint start_y = height() - m_nRowHeight - ( m_nRowHeight * pNote->m_noteKey.m_key + ( 12 * pNote->m_noteKey.m_nOctave ) * m_nRowHeight ) + 1;
	uint w = 20;
	uint h = m_nRowHeight - 1;


	// external rect
	pPainter->fillRect( start_x, start_y, w, h, QColor( 0, 0, 0 ) );

	// internal rect
	pPainter->fillRect( start_x + 1, start_y + 1, w - 2, h - 2, QColor( 100, 100, 255 ) );
}



void PianoRollEditor::mousePressEvent(QMouseEvent*)
{
	DEBUGLOG("Mouse press event");
}



void PianoRollEditor::mouseReleaseEvent(QMouseEvent*)
{
	DEBUGLOG("Mouse release event" );
}

*/
