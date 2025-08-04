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
 * Evaluating strings by trying to find special syntax tokens.
 * 2020-03
 */


#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <QString>
#include <QtXml>

#include <vector>
#include <utility>

namespace expr {

bool checkExpression(const QString &expression, bool &evaluation_success,
    const std::vector< std::pair<QString, QString> > &substitutions, const bool &needs_prefix = true);
std::vector< std::pair<QString, QString> > parseSubstitutions(const QDomNode &options);
void doMetaSubstitutions(const std::vector< std::pair<QString, QString> > &substitutions, QString &expression);

#endif // EXPRESSIONS_H

} //namespace expr
