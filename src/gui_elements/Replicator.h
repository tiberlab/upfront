/*****************************************************************************/
/*  Copyright 2019 WSL Institute for Snow and Avalanche Research  SLF-DAVOS  */
/*****************************************************************************/
/* This file is part of INIshell.
   INIshell is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   INIshell is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with INIshell.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * A Replicator is a special panel that takes a widget in its options and reproduces a numbered clone
 * of it. It is one of the few panels that must handle INI keys that aren't present in the XML
 * (although the main work for this is done in MainWindow.cc).
 * 2019-10
 */

#ifndef REPLICATOR_H
#define REPLICATOR_H

#include "Atomic.h"
#include "Group.h"

#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtXml>

class Replicator : public Atomic {
	Q_OBJECT

	public:
		explicit Replicator(const QString &section, const QString &key, const QDomNode &options,
		    const bool &no_spacers, QWidget *parent = nullptr);
		int count() const { return container_->count(); }
		void clear(const bool &set_default = true) override;

	private:
		void setOptions(const QDomNode &options);
		int findLastItemRow() const;

		QDomNode templ_;
		Group *container_ = nullptr;
		QPushButton *plus_button_ = nullptr;

	private slots:
		void replicate(const int &panel_number);
		bool deleteLast();
		void onPropertySet() override;

};
#endif //REPLICATOR_H
