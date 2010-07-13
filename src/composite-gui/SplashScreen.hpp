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


#ifndef COMPOSITE_SPLASHSCREEN_HPP
#define COMPOSITE_SPLASHSCREEN_HPP

#include <QLabel>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>

#include "CompositeApp.hpp"

/**
 * Fader and VuMeter widget
 */
class SplashScreen : public QSplashScreen
{
	Q_OBJECT
	public:
		SplashScreen();
		~SplashScreen();

	private slots:
		void onCloseTimer();
//		virtual void drawContents ( QPainter * painter );

	private:
		QPixmap *m_pBackground;
		static const uint width = 400;
		static const uint height = 300;
		//QTimer m_closeTimer;

};


#endif // COMPOSITE_SPLASHSCREEN_HPP
