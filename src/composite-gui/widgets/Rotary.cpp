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
#include "Rotary.hpp"
#include "LCD.hpp"
#include "../Skin.hpp"

#include <Tritium/globals.hpp>
#include <Tritium/Logger.hpp>

RotaryTooltip::RotaryTooltip( QPoint /*pos*/ )
//  : QWidget( 0, "RotaryTooltip", Qt::WStyle_Customize| Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop| Qt::WX11BypassWM )
  : QWidget( 0, Qt::ToolTip )
{
	m_pDisplay = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4);
	m_pDisplay->move( 0, 0 );
	resize( m_pDisplay->size() );

	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Background, QColor( 49, 53, 61 ) );
	this->setPalette( defaultPalette );

}


void RotaryTooltip::showTip( QPoint pos, QString sText )
{
	move( pos );
	m_pDisplay->setText( sText );
	show();
}

RotaryTooltip::~RotaryTooltip()
{
//	delete m_pDisplay;
}





///////////////////

QPixmap* Rotary::m_background_normal = NULL;
QPixmap* Rotary::m_background_center = NULL;


Rotary::Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseIntSteps, bool bUseValueTip )
 : QWidget( parent )
 , m_bUseIntSteps( bUseIntSteps )
 , m_type( type )
 , m_fMin( 0.0 )
 , m_fMax( 1.0 )
 , m_bShowValueToolTip( bUseValueTip )
{
	setAttribute(Qt::WA_NoBackground);
	setToolTip( sToolTip );

	m_pValueToolTip = new RotaryTooltip( mapToGlobal( QPoint( 0, 0 ) ) );

	m_nWidgetWidth = 28;
	m_nWidgetHeight = 26;
	m_fValue = 0.0;

	if ( m_background_normal == NULL ) {
		m_background_normal = new QPixmap();
		if ( m_background_normal->load( Skin::getImagePath() + "/mixerPanel/rotary_images.png" ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	}
	if ( m_background_center == NULL ) {
		m_background_center = new QPixmap();
		if ( m_background_center->load( Skin::getImagePath() + "/mixerPanel/rotary_center_images.png" ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	}

	resize( m_nWidgetWidth, m_nWidgetHeight );
//	m_temp.resize( m_nWidgetWidth, m_nWidgetHeight );
}



Rotary::~ Rotary()
{
	delete m_pValueToolTip;
}



void Rotary::paintEvent( QPaintEvent* /*ev*/ )
{
	QPainter painter(this);

	float fRange = abs( m_fMax ) + abs( m_fMin );
	float fValue = abs( m_fMin ) + m_fValue;

	int nFrame;
	if ( m_bUseIntSteps ) {
		nFrame = (int)( 63.0 * ( (int)fValue / fRange ) );
	}
	else {
		nFrame = (int)( 63.0 * ( fValue / fRange ) );
	}

//	DEBUGLOG( "\nrange: " + toString( fRange ) );
//	DEBUGLOG( "norm value: " + toString( fValue ) );
//	DEBUGLOG( "frame: " + toString( nFrame ) );

	if ( m_type == TYPE_NORMAL ) {

		int xPos = m_nWidgetWidth * nFrame;
		painter.drawPixmap( rect(), *m_background_normal, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
	}
	else {
		// the image is broken...
		if ( nFrame > 62 ) {
			nFrame = 62;
		}
		int xPos = m_nWidgetWidth * nFrame;
		painter.drawPixmap( rect(), *m_background_center, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );

	}
}



void Rotary::setValue( float fValue )
{
	if ( fValue == m_fValue ) {
		return;
	}

	if ( fValue < m_fMin ) {
		fValue = m_fMin;
	}
	else if ( fValue > m_fMax ) {
		fValue = m_fMax;
	}

	if ( fValue != m_fValue ) {
		m_fValue = fValue;
		update();
	}
}



void Rotary::mousePressEvent(QMouseEvent *ev)
{
	setCursor( QCursor( Qt::SizeVerCursor ) );

	m_fMousePressValue = m_fValue;
	m_fMousePressY = ev->y();

	if ( m_bShowValueToolTip ) {
		QString tmp = QString("%1").arg(m_fValue, 0, 'f', 2);
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), tmp );
	}
}




void Rotary::mouseReleaseEvent( QMouseEvent * /*ev*/ )
{
	setCursor( QCursor( Qt::ArrowCursor ) );
	m_pValueToolTip->hide();
}




void Rotary::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	float stepfactor = 5.0; // course adjustment
	float delta = 1.0;

	// Control Modifier = fine adjustment
	if (ev->modifiers() == Qt::ControlModifier) {
		stepfactor = 1.0;
	}
	if ( !m_bUseIntSteps ) {
		float fRange = abs( m_fMax ) + abs( m_fMin );
		delta = fRange / 100.0;
	}
	if ( ev->delta() < 0 ) {
		delta = delta * -1.0;
	}
	setValue( getValue() + (delta * stepfactor) );
	emit valueChanged(this);
}



 void Rotary::mouseMoveEvent( QMouseEvent *ev ) {
	float fRange = abs( m_fMax ) + abs( m_fMin );

	float deltaY = ev->y() - m_fMousePressY;
	float fNewValue = ( m_fMousePressValue - ( deltaY / 100.0 * fRange ) );

	setValue( fNewValue );
	emit valueChanged(this);

	if ( m_bShowValueToolTip ) {
		QString tmp = QString("%1").arg(m_fValue, 0, 'f', 2);
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), tmp );
	}
}



void Rotary::setMin( float fMin )
{
	m_fMin = fMin;
	update();
}

float Rotary::getMin()
{
	return m_fMin;
}



void Rotary::setMax( float fMax )
{
	m_fMax = fMax;
	update();
}


float Rotary::getMax()
{
	return m_fMax;
}
