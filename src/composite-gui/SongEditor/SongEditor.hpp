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

#ifndef COMPOSITE_SONGEDITOR_HPP
#define COMPOSITE_SONGEDITOR_HPP

#include <vector>

#include <QtGui>

#include <Tritium/memory.hpp>
#include "../EventListener.hpp"
#include "../PatternFillDialog.hpp"

class Button;
class ToggleButton;
class SongEditor;
class SongEditorPatternList;
class SongEditorPositionRuler;


static const uint SONG_EDITOR_MIN_GRID_WIDTH = 8;
static const uint SONG_EDITOR_MAX_GRID_WIDTH = 16;

///
/// Song editor
///
class SongEditor : public QWidget
{
	Q_OBJECT

	public:
		SongEditor( QWidget *parent );
		~SongEditor();

		void createBackground();

		void cleanUp();

		int getGridWidth ();
		void setGridWidth( uint width);

	private:
		unsigned m_nGridHeight;
		unsigned m_nGridWidth;
		static const unsigned m_nMaxPatternSequence = 400;
		bool m_bSequenceChanged;
		bool m_bIsMoving;
		bool m_bIsCtrlPressed;

		QPixmap *m_pBackgroundPixmap;
		QPixmap *m_pSequencePixmap;

		std::vector<QPoint> m_selectedCells;
		std::vector<QPoint> m_movingCells;
		QPoint m_clickPoint;	// Usato come riferimento per le operazioni di spostamento
		bool m_bShowLasso;
		QRect m_lasso;

		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void keyPressEvent (QKeyEvent *ev);
		virtual void paintEvent(QPaintEvent *ev);

		void drawSequence();
		void drawPattern( int pos, int number );
};




///
/// Song editor pattern list
///
class SongEditorPatternList : public QWidget, public EventListener
{
	Q_OBJECT

	public:
		SongEditorPatternList( QWidget *parent );
		~SongEditorPatternList();

		void updateEditor();
		void createBackground();

	public slots:
		void patternPopup_edit();
		void patternPopup_save();
		void patternPopup_load();
		void patternPopup_properties();
		void patternPopup_delete();
		void patternPopup_copy();
		void patternPopup_fill();
		void inlineEditingFinished();
		void inlineEditingEntered();
		virtual void dragEnterEvent(QDragEnterEvent *event);
		virtual void dropEvent(QDropEvent *event);


	private:
		uint m_nGridHeight;
		uint m_nWidth;
		static const uint m_nInitialHeight = 10;

		QPixmap *m_pBackgroundPixmap;

		QPixmap m_labelBackgroundLight;
		QPixmap m_labelBackgroundDark;
		QPixmap m_labelBackgroundSelected;
		QPixmap m_playingPattern_on_Pixmap;
		QPixmap m_playingPattern_off_Pixmap;

		QMenu *m_pPatternPopup;
		QLineEdit *line;
		Tritium::T<Tritium::Pattern>::shared_ptr patternBeingEdited;
		void inlineEditPatternName( int row );

		virtual void mousePressEvent( QMouseEvent *ev );
		virtual void mouseDoubleClickEvent( QMouseEvent *ev );
		virtual void paintEvent( QPaintEvent *ev );

		void fillRangeWithPattern(FillRange* r, int nPattern);
		void togglePattern( int );

		virtual void patternChangedEvent();
		void mouseMoveEvent(QMouseEvent *event);
		void movePatternLine(int,int);
		QPoint __drag_start_position;

};


// class SongEditorPatternListener : public EventListener {
//
// }
//

class SongEditorPositionRuler : public QWidget
{
	Q_OBJECT

	public:
		SongEditorPositionRuler( QWidget *parent );
		~SongEditorPositionRuler();

		void createBackground();

		uint getGridWidth();
		void setGridWidth (uint width);

	public slots:
		void updatePosition();

	private:
		QTimer *m_pTimer;
		uint m_nGridWidth;
		static const uint m_nMaxPatternSequence = 400;
		static const uint m_nInitialWidth = m_nMaxPatternSequence * 16;
		static const uint m_nHeight = 25;

		QPixmap *m_pBackgroundPixmap;
		QPixmap m_tickPositionPixmap;
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mousePressEvent( QMouseEvent *ev );
		virtual void paintEvent( QPaintEvent *ev );
};


#endif // COMPOSITE_SONGEDITOR_HPP
