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

#ifndef COMPOSITE_NOTEPROPERTIESRULER_HPP
#define COMPOSITE_NOTEPROPERTIESRULER_HPP

#include "../EventListener.hpp"

#include <Tritium/memory.hpp>
#include <QtGui>

namespace Tritium
{
	class Pattern;
}

class PatternEditorPanel;

class NotePropertiesRuler : public QWidget, public EventListener
{
	Q_OBJECT
	public:
		enum NotePropertiesMode {
			VELOCITY,
			PAN,
			LEADLAG
		};

		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode );
		~NotePropertiesRuler();

		void zoomIn();
		void zoomOut();

		//public slots:
		void updateEditor();

	private:
		static const int m_nKeys = 24;
		static const int m_nBasePitch = 12;

		NotePropertiesMode m_mode;

		PatternEditorPanel *m_pPatternEditorPanel;
		Tritium::T<Tritium::Pattern>::shared_ptr m_pPattern;
		float m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;

		QPixmap *m_pBackground;

		void createVelocityBackground(QPixmap *pixmap);
		void createPanBackground(QPixmap *pixmap);
		void createLeadLagBackground(QPixmap *pixmap);
		void paintEvent(QPaintEvent *ev);
		void mousePressEvent(QMouseEvent *ev);
		void mouseMoveEvent(QMouseEvent *ev);
		void wheelEvent(QWheelEvent *ev);

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		//~ Implements EventListener interface

};


#endif // COMPOSITE_NOTEPROPERTIESRULER_HPP
