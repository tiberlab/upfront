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

#include "expressions.h"
#include "inishell.h"
#include "src/gui_elements/Atomic.h"

#include "lib/tinyexpr.h"

#include <QRegularExpression>

namespace expr {

/**
 * @brief Evaluate a string according to special syntax tokens.
 * @details This function checks for arithmetic expressions which are in accordance with SLF software,
 * environment variables, and INI keys, and tries to evaluate them.
 * @param[in] expression The string to check.
 * @param[out] evaluation_success True if the expression could be evaluated.
 * @param[in] needs_prefix If true, check arithmetic expression only if ${{...}}. If false, check without prefix.
 * @return True if the syntax to indicate an expression is correct.
 */
bool checkExpression(const QString &expression, bool &evaluation_success,
    const std::vector< std::pair<QString, QString> > &substitutions, const bool &needs_prefix)
{
	static const QString regex_envvar(R"(\${env:(.+)})");   //ex.: ${env:USER}
	QString regex_expression;
	if (needs_prefix)
		regex_expression = R"(\${{(.+)}})"; //ex.: ${{sin(pi)}}
	else
		regex_expression = R"((.+))"; //enable checks without ${{}
	static const QString regex_inikey(R"(\${(.+)})");       //ex.: ${GENERAL::BUFFER_SIZE}
	static const QRegularExpression rex_envvar(regex_envvar);
	const QRegularExpression rex_expression(regex_expression);
	static const QRegularExpression rex_inikey(regex_inikey);
	const QRegularExpressionMatch match_envvar(rex_envvar.match(expression));
	const QRegularExpressionMatch match_expression(rex_expression.match(expression));
	const QRegularExpressionMatch match_inikey(rex_inikey.match(expression));
	static const QString regex_scientific( R"(-?[\d.]+(?:[Ee]-?\d+)?)" );
	static const QRegularExpression rex_scientific( regex_scientific );
	const QRegularExpressionMatch match_scientific(rex_scientific.match(expression));
	static const size_t idx_total = 0;
	static const size_t idx_check = 1;

	if (expression.isEmpty()) {
		evaluation_success = false;
		return false;
	}

	if (match_envvar.captured(idx_total) == expression) {
		/* check if environment variable is set */
		const QByteArray envvar( qgetenv(match_envvar.captured(idx_check).toLocal8Bit()) );
		evaluation_success = !envvar.isNull();
		return true;
	} else if (match_expression.captured(idx_total) == expression) {
		/* check if arithmetic expression is valid */
		QString sub_expr( match_expression.captured(idx_check) );
		expr::doMetaSubstitutions(substitutions, sub_expr);
		int status_code;
		te_interp(sub_expr.toStdString().c_str(), &status_code);
		evaluation_success = (status_code == 0);
		return true;
	} else if (match_inikey.captured(idx_total) == expression) {
		/* check if INI key is in XML file */
		const QList<Atomic *> found_panels( getMainWindow()->
		    getPanelsForKey(match_inikey.captured(idx_check)) );
		evaluation_success = !found_panels.isEmpty();
		return true;
	} else if (match_scientific.captured(idx_total) == expression) {
		evaluation_success = true;
		return true;
	}
	evaluation_success = false;
	return false;
}

/**
 * @brief Parse "substitutions" option into container.
 * @details This is a helper function to translate XML settings to an internal container.
 * @param[in] options XML options for the panel.
 * @return Vector of desired. substitutions and their replacements.
 */
std::vector< std::pair<QString, QString> > parseSubstitutions(const QDomNode &options)
{
	/*
	 * Regex substitutions can be given in the XML because some processing elements
	 * will prepare the expression for tinyexpr first, enabling e. g. substitutions.
	 * In order not to hard code a list of substitutions which may differ for different
	 * software, these replacements can be user-set.
	 *
	 * Consider the Particle Filter as an example. The PF knows the variable "xx" but tinyexpr
	 * does not. It is replaced by a numeric value by the PF before passing the expression to
	 * tinyexpr. With substitutions you can mimick this and replace all "xx" with "1" (or
	 * whatever fits your case) so that INIshell can correctly style it as valid - because
	 * it is valid now for tinyexpr.
	 */
	std::vector<std::pair<QString, QString>> substitutions;
	for (QDomElement op = options.firstChildElement("substitution"); !op.isNull();
	    op = op.nextSiblingElement("substitution")) {
		std::pair<QString, QString> sub(op.attribute("find"), op.attribute("replace"));
		substitutions.emplace_back(sub);
	}
	return substitutions;
}

/**
 * @brief Mimic MeteoIO's data and meta data substitutions.
 * @details This way, the expression element in INIshell can style it as correct.
 * @param[in] substitutions The requested subsitutions a panel has read and stored from XML.
 * @param[in] expression Expression as entered by the user.
 * Will be transformed so that tinyexpr understands it.
 */
void doMetaSubstitutions(const std::vector<std::pair<QString, QString> > &substitutions, QString &expression)
{
	for (auto &sub : substitutions)
		expression.replace(QRegularExpression(sub.first), sub.second);
}


} //namespace expr
