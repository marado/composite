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

#include <Tritium/Preferences.hpp>
#include <Tritium/Engine.hpp>
#include <Tritium/Sampler.hpp>
#include <Tritium/Instrument.hpp>
#include <Tritium/InstrumentList.hpp>
#include <Tritium/Pattern.hpp>
#include <Tritium/EventQueue.hpp>
#include <Tritium/Transport.hpp>
#include <Tritium/Logger.hpp>
#include <Tritium/memory.hpp>

using namespace Tritium;

#include "CompositeApp.hpp"
#include "PatternEditorPanel.hpp"
#include "PatternEditorInstrumentList.hpp"
#include "PatternEditorRuler.hpp"
#include "NotePropertiesRuler.hpp"
#include "DrumPatternEditor.hpp"
#include "PianoRollEditor.hpp"

#include "../MainForm.hpp"
#include "../widgets/Button.hpp"
#include "../widgets/Fader.hpp"
#include "../widgets/PixmapWidget.hpp"
#include "../widgets/LCDCombo.hpp"

#include "../Skin.hpp"
#include "../SongEditor/SongEditorPanel.hpp"

#include <cmath>

#include <QtGui>


void PatternEditorPanel::updateSLnameLabel( )
{
	QFont font;
	font.setBold( true );
	pSLlabel->setFont( font );
	pSLlabel->setText( g_engine->getCurrentDrumkitname() );
} 



PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
 : QWidget( pParent )
 , m_pPattern()
 , m_bEnablePatternResize( true )
{
	setAcceptDrops(true);

	T<Preferences>::shared_ptr pPref = g_engine->get_preferences();
	

// Editor TOP
	PixmapWidget *editor_top = new PixmapWidget(0);
	editor_top->setPixmap("/patternEditor/editor_top.png", true);
	editor_top->setFixedHeight(24);

	PixmapWidget *editor_top_2 = new PixmapWidget(0);
	editor_top_2->setPixmap("/patternEditor/editor_top.png", true);
	editor_top_2->setFixedHeight(24);

	QHBoxLayout *editor_top_hbox = new QHBoxLayout(editor_top);
	editor_top_hbox->setSpacing(0);
	editor_top_hbox->setMargin(0);
	editor_top_hbox->setAlignment(Qt::AlignLeft);

	QHBoxLayout *editor_top_hbox_2 = new QHBoxLayout(editor_top_2);
	editor_top_hbox_2->setSpacing(0);
	editor_top_hbox_2->setMargin(0);
	editor_top_hbox_2->setAlignment(Qt::AlignLeft);


	//soundlibrary name
	pSLlabel = new QLabel( NULL );
	pSLlabel->setText( g_engine->getCurrentDrumkitname() );
	pSLlabel->setFixedSize( 170, 20 );
	pSLlabel->move( 10, 3 );
	pSLlabel->setToolTip( trUtf8("Loaded Soundlibrary") );
	editor_top_hbox->addWidget( pSLlabel ); 

//wolke some background images back_size_res
	PixmapWidget *pSizeResol = new PixmapWidget( NULL );
	pSizeResol->setFixedSize( 200, 20 );
	pSizeResol->setPixmap( "/patternEditor/background_res-new.png" );
	pSizeResol->move( 0, 3 );
	editor_top_hbox_2->addWidget( pSizeResol );

	// PATTERN size
	__pattern_size_combo = new LCDCombo(pSizeResol, 4);
	__pattern_size_combo->move( 34, 2 );
	__pattern_size_combo->setToolTip( trUtf8("Select pattern size") );
	for ( int i = 1; i <= 32; i++) {
		__pattern_size_combo->addItem( QString( "%1" ).arg( i ) );
	}
	__pattern_size_combo->update();
	connect(__pattern_size_combo, SIGNAL( valueChanged( QString ) ), this, SLOT( patternSizeChanged(QString) ) );
	//editor_top_hbox->addWidget(__pattern_size_combo);


	// GRID resolution
	__resolution_combo = new LCDCombo( pSizeResol , 7);
	__resolution_combo->setToolTip(trUtf8("Select grid resolution"));
	__resolution_combo->addItem( "4" );
	__resolution_combo->addItem( "8" );
	__resolution_combo->addItem( "16" );
	__resolution_combo->addItem( "32" );
	__resolution_combo->addItem( "64" );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( "4T" );
	__resolution_combo->addItem( "8T" );
	__resolution_combo->addItem( "16T" );
	__resolution_combo->addItem( "32T" );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( "off" );
	__resolution_combo->update();
	__resolution_combo->move( 121, 2 );
	connect(__resolution_combo, SIGNAL(valueChanged(QString)), this, SLOT(gridResolutionChanged(QString)));
	//editor_top_hbox->addWidget(__resolution_combo);


//wolke some background images hear note rec quant

	PixmapWidget *pRec = new PixmapWidget( NULL );
	pRec->setFixedSize( 150, 20 );
	pRec->setPixmap( "/patternEditor/background_rec-new.png" );
	pRec->move( 0, 3 );
	editor_top_hbox_2->addWidget( pRec );


	// Hear notes btn
	ToggleButton *hearNotesBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_hear_on.png",
			"/patternEditor/btn_hear_off.png",
			"/patternEditor/btn_hear_off.png",
			QSize(15, 13)
	);
	hearNotesBtn->move( 34, 3 );
	hearNotesBtn->setToolTip( trUtf8( "Hear new notes" ) );
	connect( hearNotesBtn, SIGNAL(clicked(Button*)), this, SLOT( hearNotesBtnClick(Button*)));
	//editor_top_hbox->addWidget(hearNotesBtn);
	// restore hear new notes button state
	hearNotesBtn->setPressed( pPref->getHearNewNotes() );


	// Record events btn
	ToggleButton* recordEventsBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_record_on.png",
			"/patternEditor/btn_record_off.png",
			"/patternEditor/btn_record_off.png",
			QSize(15, 13)
	);
	recordEventsBtn->move( 74, 3 );
	recordEventsBtn->setPressed( pPref->getRecordEvents());
	recordEventsBtn->setToolTip( trUtf8( "Record keyboard/midi events" ) );
	connect( recordEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( recordEventsBtnClick(Button*)));
	//editor_top_hbox->addWidget(recordEventsBtn);


	// quantize
	ToggleButton* quantizeEventsBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_quant_on.png",
			"/patternEditor/btn_quant_off.png",
			"/patternEditor/btn_quant_off.png",
			QSize(15, 13)
	);
	quantizeEventsBtn->move( 130, 3 );
	quantizeEventsBtn->setPressed( pPref->getQuantizeEvents());
	quantizeEventsBtn->setToolTip( trUtf8( "Quantize keyboard/midi events to grid" ) );
	connect( quantizeEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( quantizeEventsBtnClick(Button*)));
	//editor_top_hbox->addWidget(quantizeEventsBtn);


// 	PixmapWidget *pZoom = new PixmapWidget( NULL );
// 	pZoom->setFixedSize( 73, 20 );
// 	pZoom->setPixmap( "/patternEditor/background_zoom-new.png" );
// 	pZoom->move( 0, 3 );
// 	editor_top_hbox_2->addWidget( pZoom );

	// zoom-in btn
	Button *zoom_in_btn = new Button(
			NULL,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize(19, 13)
	);
// 	zoom_in_btn->move( 51, 3 );
// 	zoom_in_btn->setText("+");
	zoom_in_btn->setToolTip( trUtf8( "Zoom in" ) );
	connect(zoom_in_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomInBtnClicked(Button*) ) );
	//editor_top_hbox_2->addWidget(zoom_in_btn);


	// zoom-out btn
	Button *zoom_out_btn = new Button(
			NULL,
			"/songEditor/btn_minus_on.png",
			"/songEditor/btn_minus_off.png",
			"/songEditor/btn_minus_over.png",
			QSize(19, 13)
	);
// 	zoom_out_btn->move( 2, 3 );
	//zoom_out_btn->setText("-");
	zoom_out_btn->setToolTip( trUtf8( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomOutBtnClicked(Button*) ) );
	//editor_top_hbox_2->addWidget(zoom_out_btn);


// End Editor TOP


// RULER____________________________________

	// Ruler ScrollView
	m_pRulerScrollView = new QScrollArea( NULL );
	m_pRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setFixedHeight( 25 );
	// Ruler
	m_pPatternEditorRuler = new PatternEditorRuler( m_pRulerScrollView->viewport() );

	m_pRulerScrollView->setWidget( m_pPatternEditorRuler );

//~ RULER


// EDITOR _____________________________________
	// Editor scrollview
	m_pEditorScrollView = new QScrollArea( NULL );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );



	// Editor
	m_pDrumPatternEditor = new DrumPatternEditor( m_pEditorScrollView->viewport(), this );

	m_pEditorScrollView->setWidget( m_pDrumPatternEditor );

	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );



/*
	m_pPianoRollScrollView = new QScrollArea( NULL );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pPianoRollEditor = new PianoRollEditor( m_pPianoRollScrollView->viewport() );
	m_pPianoRollScrollView->setWidget( m_pPianoRollEditor );

	connect( m_pPianoRollScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );


	m_pPianoRollScrollView->hide();
*/
//~ EDITOR






// INSTRUMENT LIST
	// Instrument list scrollview
	m_pInstrListScrollView = new QScrollArea( NULL );
	m_pInstrListScrollView->setFrameShape( QFrame::NoFrame );
	m_pInstrListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pInstrListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	// Instrument list
	m_pInstrumentList = new PatternEditorInstrumentList( m_pInstrListScrollView->viewport(), this );
	m_pInstrListScrollView->setWidget( m_pInstrumentList );
	m_pInstrListScrollView->setFixedWidth( m_pInstrumentList->width() );

	connect( m_pInstrListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );
//~ INSTRUMENT LIST




// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new QScrollArea( NULL );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler( m_pNoteVelocityScrollView->viewport(), this, NotePropertiesRuler::VELOCITY );
	m_pNoteVelocityScrollView->setWidget( m_pNoteVelocityEditor );
	m_pNoteVelocityScrollView->setFixedHeight( 100 );
//~ NOTE_VELOCITY EDITOR


// NOTE_PAN EDITOR
	m_pNotePanScrollView = new QScrollArea( NULL );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler( m_pNotePanScrollView->viewport(), this, NotePropertiesRuler::PAN );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );
//~ NOTE_PAN EDITOR


// NOTE_LEADLAG EDITOR
	m_pNoteLeadLagScrollView = new QScrollArea( NULL );
	m_pNoteLeadLagScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteLeadLagScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagEditor = new NotePropertiesRuler( m_pNoteLeadLagScrollView->viewport(), this, NotePropertiesRuler::LEADLAG );
	m_pNoteLeadLagScrollView->setWidget( m_pNoteLeadLagEditor );
	m_pNoteLeadLagScrollView->setFixedHeight( 100 );
//~ NOTE_LEADLAG EDITOR

	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , NULL  );
	connect( m_pPatternEditorHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, NULL );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	QHBoxLayout *pPatternEditorHScrollBarLayout = new QHBoxLayout();
	pPatternEditorHScrollBarLayout->setSpacing( 0 );
	pPatternEditorHScrollBarLayout->setMargin( 0 );
	pPatternEditorHScrollBarLayout->addWidget( m_pPatternEditorHScrollBar );
	pPatternEditorHScrollBarLayout->addWidget( zoom_in_btn );
	pPatternEditorHScrollBarLayout->addWidget( zoom_out_btn );

	QWidget *pPatternEditorHScrollBarContainer = new QWidget();
	pPatternEditorHScrollBarContainer->setLayout( pPatternEditorHScrollBarLayout );


	QPalette label_palette;
	label_palette.setColor( QPalette::Foreground, QColor( 230, 230, 230 ) );

	QFont boldFont;
	boldFont.setBold( true );
	m_pPatternNameLbl = new QLabel( NULL );
	m_pPatternNameLbl->setFont( boldFont );
	m_pPatternNameLbl->setText( "pattern name label" );
	//m_pPatternNameLbl->setFixedWidth(200);
	m_pPatternNameLbl->setPalette(label_palette);






// NOTE_PROPERTIES BUTTONS
	PixmapWidget *pPropertiesPanel = new PixmapWidget( NULL );
	pPropertiesPanel->setColor( QColor( 58, 62, 72 ) );

	pPropertiesPanel->setFixedSize( 181, 100 );

	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );


	LCDCombo* pPropertiesCombo = new LCDCombo( NULL, 20);
	pPropertiesCombo->setToolTip(trUtf8("Select note properties"));
	pPropertiesCombo->addItem( trUtf8("Velocity") );
	pPropertiesCombo->addItem( trUtf8("Pan") );
	pPropertiesCombo->addItem( trUtf8("Lead and Lag") );
	pPropertiesCombo->update();
	connect( pPropertiesCombo, SIGNAL(valueChanged(QString)), this, SLOT(propertiesComboChanged(QString)));

	pPropertiesVBox->addWidget( pPropertiesCombo );


//~ NOTE_PROPERTIES BUTTONS


// LAYOUT
	QWidget *pMainPanel = new QWidget();

	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( editor_top, 0, 0);
	pGrid->addWidget( editor_top_2, 0, 1, 1, 3);
	pGrid->addWidget( m_pPatternNameLbl, 1, 0 );
	pGrid->addWidget( m_pRulerScrollView, 1, 1 );

	pGrid->addWidget( m_pInstrListScrollView, 2, 0 );

	pGrid->addWidget( m_pEditorScrollView, 2, 1 );
//	pGrid->addWidget( m_pPianoRollScrollView, 2, 1 );

	pGrid->addWidget( m_pPatternEditorVScrollBar, 2, 2 );
	pGrid->addWidget( pPatternEditorHScrollBarContainer, 10, 1 );
	pGrid->addWidget( m_pNoteVelocityScrollView, 4, 1 );
	pGrid->addWidget( m_pNotePanScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteLeadLagScrollView, 4, 1 );

	pGrid->addWidget( pPropertiesPanel, 4, 0 );
	pGrid->setRowStretch( 2, 100 );
	pMainPanel->setLayout( pGrid );





	// restore grid resolution
	int nIndex;
	if ( pPref->isPatternEditorUsingTriplets() == false ) {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 4:
				__resolution_combo->set_text( "4" );
				nIndex = 0;
				break;

			case 8:
				__resolution_combo->set_text( "8" );
				nIndex = 1;
				break;

			case 16:
				__resolution_combo->set_text( "16" );
				nIndex = 2;
				break;

			case 32:
				__resolution_combo->set_text( "32" );
				nIndex = 3;
				break;

			case 64:
				__resolution_combo->set_text( "64" );
				nIndex = 4;
				break;

			default:
			    ERRORLOG( QString("Wrong grid resolution: %1").arg( pPref->getPatternEditorGridResolution() ) );
				__resolution_combo->set_text( "4" );
				nIndex = 0;
		}
	}
	else {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 8:
				__resolution_combo->set_text( "4T" );
				nIndex = 5;
				break;

			case 16:
				__resolution_combo->set_text( "8T" );
				nIndex = 6;
				break;

			case 32:
				__resolution_combo->set_text( "16T" );
				nIndex = 7;
				break;

			case 64:
				__resolution_combo->set_text( "32T" );
				nIndex = 8;
				break;

			default:
				ERRORLOG( QString("Wrong grid resolution: %1").arg( pPref->getPatternEditorGridResolution() ) );
				__resolution_combo->set_text( "4T" );
				nIndex = 5;
		}
	}
	gridResolutionChanged(__resolution_combo->getText());






	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );

	CompositeApp::get_instance()->addEventListener( this );

	selectedPatternChangedEvent(); // force an update

	pPropertiesCombo->set_text( trUtf8("Velocity"));
}




PatternEditorPanel::~PatternEditorPanel()
{
}



void PatternEditorPanel::syncToExternalHorizontalScrollbar(int)
{
	//DEBUGLOG( "[syncToExternalHorizontalScrollbar]" );

	// drum Editor
	m_pEditorScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
	m_pEditorScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// piano roll Editor
//	m_pPianoRollScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
//	m_pPianoRollScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );


	// Ruler
	m_pRulerScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// Instrument list
	m_pInstrListScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// Velocity ruler
	m_pNoteVelocityScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// pan ruler
	m_pNotePanScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// leadlag ruler
	m_pNoteLeadLagScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
}


void PatternEditorPanel::on_patternEditorScroll(int nValue)
{
	//DEBUGLOG( "[on_patternEditorScroll] " + QString::number(nValue)  );
	m_pPatternEditorVScrollBar->setValue( nValue );	
	resizeEvent(NULL);
}




void PatternEditorPanel::gridResolutionChanged( QString str )
{
	int nResolution;
	bool bUseTriplets = false;

	if ( str.contains( "off" ) ) {
		nResolution=MAX_NOTES;
	}
	else if ( str.contains( "T" ) ) {
		bUseTriplets = true;
		QString temp = str;
		temp.chop( 1 );
		nResolution = temp.toInt() * 2;
	}
	else {
		nResolution = str.toInt();
	}

	//DEBUGLOG( to_string( nResolution ) );
	m_pDrumPatternEditor->setResolution( nResolution, bUseTriplets );

	g_engine->get_preferences()->setPatternEditorGridResolution( nResolution );
	g_engine->get_preferences()->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	PatternList *pPatternList = g_engine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = g_engine->getSelectedPatternNumber();

	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		// update pattern name text
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
		QString sCurrentPatternName = m_pPattern->get_name();
		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg( sCurrentPatternName ) ) );
		//m_pNameLCD->setText( sCurrentPatternName );
		m_pPatternNameLbl->setText( sCurrentPatternName );

		// update pattern size combobox
		int nPatternSize = m_pPattern->get_length();
		int nEighth = MAX_NOTES / 8;
		for ( int i = 1; i <= 32; i++ ) {
			if ( nPatternSize == nEighth * i ) {
				__pattern_size_combo->set_text( QString( "%1" ).arg( i ) );
				break;
			}
		}
	}
	else {
		m_pPattern.reset();

		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg(QString("No pattern selected.")) ) );
		//m_pNameLCD->setText( trUtf8( "No pattern selected" ) );
		m_pPatternNameLbl->setText( trUtf8( "No pattern selected" ) );
	}

	resizeEvent( NULL ); // force an update of the scrollbars
}



void PatternEditorPanel::hearNotesBtnClick(Button *ref)
{
	T<Preferences>::shared_ptr pref = ( g_engine->get_preferences() );
	pref->setHearNewNotes( ref->isPressed() );

	if (ref->isPressed() ) {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Hear new notes = On" ), 2000 );
	}
	else {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Hear new notes = Off" ), 2000 );
	}

}



void PatternEditorPanel::recordEventsBtnClick(Button *ref)
{
	T<Preferences>::shared_ptr pref = ( g_engine->get_preferences() );
	pref->setRecordEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Record keyboard/midi events = On" ), 2000 );
	}
	else {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Record keyboard/midi events = Off" ), 2000 );
	}


}


void PatternEditorPanel::quantizeEventsBtnClick(Button *ref)
{
	T<Preferences>::shared_ptr pref = ( g_engine->get_preferences() );
	pref->setQuantizeEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = On" ), 2000 );
	}
	else {
		( CompositeApp::get_instance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = Off" ), 2000 );
	}
}


void PatternEditorPanel::stateChangedEvent(int state)
{
	T<Transport>::shared_ptr xport = g_engine->get_transport();
	if ( (state == Engine::StateReady) && (xport->get_state() != TransportPosition::ROLLING) ) {
		m_bEnablePatternResize = true;
	}
	else {
		m_bEnablePatternResize = false;
	}
}

void PatternEditorPanel::transportEvent(TransportPosition::State state)
{
	int h2state = g_engine->getState();
	if( (h2state == Engine::StateReady) && (state != TransportPosition::ROLLING) ) {
		m_bEnablePatternResize = true;
	}
	else {
		m_bEnablePatternResize = false;
	}

}

void PatternEditorPanel::resizeEvent( QResizeEvent * /*ev*/ )
{
	QScrollArea *pScrollArea = m_pEditorScrollView;

/*
	if ( m_pPianoRollScrollView->isVisible() ) {
		pScrollArea = m_pPianoRollScrollView;
	}
	else {
		pScrollArea = m_pEditorScrollView;
	}
*/

	m_pPatternEditorHScrollBar->setMinimum( pScrollArea->horizontalScrollBar()->minimum() );
	m_pPatternEditorHScrollBar->setMaximum( pScrollArea->horizontalScrollBar()->maximum() );
	m_pPatternEditorHScrollBar->setSingleStep( pScrollArea->horizontalScrollBar()->singleStep() );
	m_pPatternEditorHScrollBar->setPageStep( pScrollArea->horizontalScrollBar()->pageStep() );

	m_pPatternEditorVScrollBar->setMinimum( pScrollArea->verticalScrollBar()->minimum() );
	m_pPatternEditorVScrollBar->setMaximum( pScrollArea->verticalScrollBar()->maximum() );
	m_pPatternEditorVScrollBar->setSingleStep( pScrollArea->verticalScrollBar()->singleStep() );
	m_pPatternEditorVScrollBar->setPageStep( pScrollArea->verticalScrollBar()->pageStep() );
}




void PatternEditorPanel::showEvent ( QShowEvent * /*ev*/ )
{
//	m_pPatternEditorVScrollBar->setValue( m_pPatternEditorVScrollBar->maximum() );
}


/// richiamato dall'uso dello scroll del mouse
void PatternEditorPanel::contentsMoving(int /*dummy*/)
{
	//DEBUGLOG( "contentsMoving" );
	syncToExternalHorizontalScrollbar(0);
}



void PatternEditorPanel::selectedInstrumentChangedEvent()
{
  //m_pNoteVelocityEditor->updateEditor();
  //m_pNotePanEditor->updateEditor();
  //m_pNoteLeadLagEditor->updateEditor();

	resizeEvent(NULL);	// force a scrollbar update
}



void PatternEditorPanel::showDrumEditorBtnClick(Button * /*ref*/)
{
	__show_drum_btn->setPressed( true );
	__show_piano_btn->setPressed( false );


//	m_pPianoRollScrollView->hide();
	m_pEditorScrollView->show();
	m_pInstrListScrollView->show();

	m_pDrumPatternEditor->selectedInstrumentChangedEvent(); // force an update

	// force a re-sync of extern scrollbars
	resizeEvent( NULL );
}



void PatternEditorPanel::showPianoEditorBtnClick(Button * /*ref*/)
{
	__show_piano_btn->setPressed( true );
	__show_drum_btn->setPressed( false );


//	m_pPianoRollScrollView->show();
	m_pEditorScrollView->hide();
	m_pInstrListScrollView->hide();

//	m_pPianoRollEditor->selectedInstrumentChangedEvent(); // force an update

	// force a re-sync of extern scrollbars
	resizeEvent( NULL );
}




void PatternEditorPanel::zoomInBtnClicked(Button * /*ref*/)
{
	if(m_pPatternEditorRuler->getGridWidth() >=24){
		return;
	}
	m_pPatternEditorRuler->zoomIn();
	m_pDrumPatternEditor->zoom_in();
	m_pNoteVelocityEditor->zoomIn();
	m_pNoteLeadLagEditor->zoomIn();
	m_pNotePanEditor->zoomIn();

	resizeEvent( NULL );
}



void PatternEditorPanel::zoomOutBtnClicked(Button * /*ref*/)
{
	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoom_out();
	m_pNoteVelocityEditor->zoomOut();
	m_pNoteLeadLagEditor->zoomOut();
	m_pNotePanEditor->zoomOut();

	resizeEvent( NULL );
}



void PatternEditorPanel::patternSizeChanged( QString str )
{
	DEBUGLOG( "pattern size changed" );

	uint nEighth = MAX_NOTES / 8;
	int nSelected = str.toInt();

	if ( !m_pPattern ) {
		return;
	}

	if ( m_pPattern->get_length() == nEighth * nSelected ) {
		// non e' necessario aggiornare
		return;
	}


	if ( !m_bEnablePatternResize ) {
		__pattern_size_combo->set_text(QString::number(m_pPattern->get_length() / nEighth ),false);
		QMessageBox::information( this, "Composite", trUtf8( "Is not possible to change the pattern size when playing." ) );
		return;
	}


	if ( nSelected > 0 && nSelected <= 32 ) {
		m_pPattern->set_length( nEighth * nSelected );
		//m_pPatternSizeLCD->setText( QString( "%1" ).arg( nSelected ) );
	}
	else {
		ERRORLOG( QString("[patternSizeChanged] Unhandled case %1").arg( nSelected ) );
	}

	m_pPatternEditorRuler->updateEditor( true );	// redraw all
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();
	m_pNoteLeadLagEditor->updateEditor();

	resizeEvent( NULL );

	g_engine->get_event_queue()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



void PatternEditorPanel::moveUpBtnClicked(Button *)
{
	Engine *engine = g_engine;
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	g_engine->lock( RIGHT_HERE );

	T<Song>::shared_ptr pSong = engine->getSong();
	T<InstrumentList>::shared_ptr pInstrumentList = g_engine->get_sampler()->get_instrument_list();

	if ( ( nSelectedInstrument - 1 ) >= 0 ) {
		T<Instrument>::shared_ptr pTemp = pInstrumentList->get( nSelectedInstrument - 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument - 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

/*
		// devo spostare tutte le note...
		PatternList *pPatternList = pSong->getPatternList();
		for ( int nPattern = 0; nPattern < pPatternList->getSize(); nPattern++ ) {
			Pattern *pPattern = pPatternList->get( nPattern );
			Sequence *pSeq1 = pPattern->m_pSequenceList->get( nSelectedInstrument );
			Sequence *pSeq2 = pPattern->m_pSequenceList->get( nSelectedInstrument - 1 );

			// swap notelist..
			map <int, Note*> noteList = pSeq1->m_noteList;
			pSeq1->m_noteList = pSeq2->m_noteList;
			pSeq2->m_noteList = noteList;
		}
*/
		g_engine->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument - 1 );

		pSong->set_modified( true );
	}
	else {
		g_engine->unlock();
	}
}



void PatternEditorPanel::moveDownBtnClicked(Button *)
{
	Engine *engine = g_engine;
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	g_engine->lock( RIGHT_HERE );

	T<Song>::shared_ptr pSong = engine->getSong();
	T<InstrumentList>::shared_ptr pInstrumentList = g_engine->get_sampler()->get_instrument_list();

	if ( ( nSelectedInstrument + 1 ) < (int)pInstrumentList->get_size() ) {
		T<Instrument>::shared_ptr pTemp = pInstrumentList->get( nSelectedInstrument + 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument + 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

/*
		// devo spostare tutte le note...
		PatternList *pPatternList = pSong->getPatternList();
		for ( int nPattern = 0; nPattern < pPatternList->getSize(); nPattern++ ) {
			Pattern *pPattern = pPatternList->get( nPattern );
			Sequence *pSeq1 = pPattern->m_pSequenceList->get( nSelectedInstrument );
			Sequence *pSeq2 = pPattern->m_pSequenceList->get( nSelectedInstrument + 1 );

			// swap notelist..
			map <int, Note*> noteList = pSeq1->m_noteList;
			pSeq1->m_noteList = pSeq2->m_noteList;
			pSeq2->m_noteList = noteList;
		}
*/
		g_engine->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument + 1 );

		pSong->set_modified( true );
	}
	else {
		g_engine->unlock();
	}

}




void PatternEditorPanel::dragEnterEvent(QDragEnterEvent *event)
{
	m_pInstrumentList->dragEnterEvent( event );
}



void PatternEditorPanel::dropEvent(QDropEvent *event)
{
	m_pInstrumentList->dropEvent( event );
}



void PatternEditorPanel::propertiesComboChanged( QString text )
{
	if ( text == trUtf8( "Velocity" ) ) {
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteVelocityScrollView->show();

		m_pNoteVelocityEditor->updateEditor();
	}
	else if ( text == trUtf8( "Pan" ) ) {
		m_pNoteVelocityScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNotePanScrollView->show();

		m_pNotePanEditor->updateEditor();
	}
	else if ( text == trUtf8( "Lead and Lag" ) ) {
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->show();
 
		m_pNoteLeadLagEditor->updateEditor();
	}
	else if ( text == trUtf8( "Cutoff" ) ) {
	}
	else if ( text == trUtf8( "Resonance" ) ) {
	}
	else {
		ERRORLOG( "Unknown text: " + text );
	}
}
