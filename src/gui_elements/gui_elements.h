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
 * This file includes all our custom panels and provides an object factory for them.
 * 2019-10
 */

#ifndef GUI_ELEMENTS_H
#define GUI_ELEMENTS_H

#include "src/gui_elements/Checklist.h"
#include "src/gui_elements/Checkbox.h"
#include "src/gui_elements/Choice.h"
#include "src/gui_elements/Datepicker.h"
#include "src/gui_elements/Dropdown.h"
#include "src/gui_elements/FilePath.h"
#include "src/gui_elements/Group.h"
#include "src/gui_elements/GridPanel.h"
#include "src/gui_elements/Helptext.h"
#include "src/gui_elements/HorizontalPanel.h"
#include "src/gui_elements/Label.h"
#include "src/gui_elements/Number.h"
#include "src/gui_elements/Replicator.h"
#include "src/gui_elements/Selector.h"
#include "src/gui_elements/Spacer.h"
#include "src/gui_elements/Textfield.h"
#include "src/main/constants.h"

#include <QString>
#include <QWidget>
#include <QtXml>

QWidget * elementFactory(const QString &in_identifier, const QString &section, const QString &key,
    const QDomNode &options, const bool &no_spacers, QWidget *parent = nullptr);

#endif //GUI_ELEMENTS_H
