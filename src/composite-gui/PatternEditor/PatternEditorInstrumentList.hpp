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


#ifndef COMPOSITE_PATTERNEDITORINSTRUMENTLIST_HPP
#define COMPOSITE_PATTERNEDITORINSTRUMENTLIST_HPP

#include "config.h"

#include <Tritium/globals.hpp>
#include <Tritium/memory.hpp>

#include <QtGui>

#include "../widgets/PixmapWidget.hpp"

namespace Tritium
{
	class Pattern;
}

class PatternEditorPanel;
class ToggleButton;

class InstrumentLine : public PixmapWidget
{
	Q_OBJECT

	public:
		InstrumentLine(QWidget* pParent);

		void setName(const QString& sName);
		void setSelected(bool isSelected);
		void setNumber(int nIndex);
		void setMuted(bool isMuted);
		void setSoloed( bool soloed );

	private slots:
		void functionClearNotes();
		void functionFillNotes();
		void functionRandomizeVelocity();
		void functionDeleteInstrument();
		void muteClicked();
		void soloClicked();


	private:
		QMenu *m_pFunctionPopup;
		QLabel *m_pNameLbl;
		bool m_bIsSelected;
		int m_nInstrumentNumber;	///< The related instrument number
		ToggleButton *m_pMuteBtn;
		ToggleButton *m_pSoloBtn;

		virtual void mousePressEvent(QMouseEvent *ev);
		Tritium::T<Tritium::Pattern>::shared_ptr getCurrentPattern();
};


class PatternEditorInstrumentList : public QWidget {
	Q_OBJECT

	public:
		PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel );
		~PatternEditorInstrumentList();

		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);


		virtual void dragEnterEvent(QDragEnterEvent *event);
		virtual void dropEvent(QDropEvent *event);


	public slots:
		void updateInstrumentLines();


	protected:
		PatternEditorPanel *m_pPatternEditorPanel;
		Tritium::Pattern *m_pPattern;
		uint m_nGridHeight;
		uint m_nEditorWidth;
		uint m_nEditorHeight;
		InstrumentLine* m_pInstrumentLine[MAX_INSTRUMENTS];
		QTimer *m_pUpdateTimer;

		QPoint __drag_start_position;

		InstrumentLine* createInstrumentLine();
		void moveInstrumentLine(int,int);

};


#endif // COMPOSITE_PATTERNEDITORINSTRUMENTLIST_HPP
