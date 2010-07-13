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

#ifndef COMPOSITE_PATTERNFILLDIALOG_HPP
#define COMPOSITE_PATTERNFILLDIALOG_HPP

#include "config.h"

#include <QtGui>
#include "ui_PatternFillDialog_UI.h"

namespace Tritium
{
	class Pattern;
}

struct FillRange {

        int fromVal;
        int toVal;
        bool bInsert;
};


///
/// Pattern Fill Dialog
///
class PatternFillDialog : public QDialog, public Ui_PatternFillDialog_UI
{
	Q_OBJECT
	public:
		PatternFillDialog( QWidget* parent, FillRange* range );
		~PatternFillDialog();

	private slots:
		void on_cancelBtn_clicked();
		void on_okBtn_clicked();
		void on_fromText_textChanged(const QString & text);
		void on_toText_textChanged(const QString & text);

	private:
		Tritium::Pattern* __pattern;
		FillRange* __fill_range;

		/// Does some name check
		void __text_changed();


};


#endif // COMPOSITE_PATTERNFILLDIALOG_HPP
