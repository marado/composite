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

#ifndef COMPOSITE_PREFERENCESDIALOG_HPP
#define COMPOSITE_PREFERENCESDIALOG_HPP

#include "config.h"

#include "ui_PreferencesDialog_UI.h"

///
/// Preferences Dialog
///
class PreferencesDialog : public QDialog, private Ui_PreferencesDialog_UI
{
	Q_OBJECT
	public:
		PreferencesDialog( QWidget* parent );
		~PreferencesDialog();

	private slots:
		void on_okBtn_clicked();
		void on_cancelBtn_clicked();
		void on_selectApplicationFontBtn_clicked();
		void on_selectMixerFontBtn_clicked();
		void on_restartDriverBtn_clicked();
		void on_driverComboBox_activated( int index );
		void on_bufferSizeSpinBox_valueChanged( int i );
		void on_sampleRateComboBox_editTextChanged( const QString& text );
		void on_midiPortComboBox_activated( int index );
		void on_styleComboBox_activated( int index );
		void on_m_pMidiDriverComboBox_currentIndexChanged( const QString& text );

	private:
		bool m_bNeedDriverRestart;

		void updateDriverInfo();
};

#endif // COMPOSITE_PREFERENCESDIALOG_HPP
