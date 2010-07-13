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

#ifndef COMPOSITE_CPULOADWIDGET_HPP
#define COMPOSITE_CPULOADWIDGET_HPP

#include "config.h"

#include <iostream>

#include "../EventListener.hpp"

#include <QtGui>

///
/// Shows CPU load
///
class CpuLoadWidget : public QWidget, public EventListener
{
	Q_OBJECT

	public:
		CpuLoadWidget(QWidget *pParent );
		~CpuLoadWidget();

		void setValue( float newValue );
		float getValue();

		void mousePressEvent(QMouseEvent *ev);
		void paintEvent(QPaintEvent *ev);

		void XRunEvent();

	public slots:
		void updateCpuLoadWidget();

	private:
		float m_fValue;
		uint m_nXRunValue;

		QPixmap m_back;
		QPixmap m_leds;
};


#endif // COMPOSITE_CPULOADWIDGET_HPP
