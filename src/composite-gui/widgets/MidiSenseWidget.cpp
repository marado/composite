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

#include "MidiSenseWidget.hpp"
#include <Tritium/Engine.hpp>
#include <Tritium/Logger.hpp>


MidiSenseWidget::MidiSenseWidget(QWidget* pParent) : QDialog( pParent )
{
	setWindowTitle( "Waiting.." );
	setFixedSize( 200, 100 );	
	
	m_pURLLabel = new QLabel( this );
	m_pURLLabel->setAlignment( Qt::AlignCenter );
	m_pURLLabel->setText( "Waiting for midi input..." );
	
	
	QVBoxLayout* pVBox = new QVBoxLayout( this );
	pVBox->addWidget( m_pURLLabel );
	setLayout( pVBox );
	
	Tritium::Engine *pEngine = Tritium::g_engine;
	pEngine->set_last_midi_event("", 0);
	
	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMidi() ) );

	m_pUpdateTimer->start( 100 );
};

MidiSenseWidget::~MidiSenseWidget(){
	DEBUGLOG("DESTROY");
	m_pUpdateTimer->stop();
}

void MidiSenseWidget::updateMidi(){
	Tritium::Engine *pEngine = Tritium::g_engine;
	if( pEngine->have_last_midi_event() ) {
	    pEngine->get_last_midi_event(&lastMidiEvent, &lastMidiEventParameter);
	    close();
	}
}

