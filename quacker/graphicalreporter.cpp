/*
 *  Quackle -- Crossword game artificial intelligence and analysis tool
 *  Copyright (C) 2005-2014 Jason Katz-Brown and John O'Laughlin.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include <board.h>
#include <computerplayer.h>
#include <game.h>
#include <quackleio/util.h>

#include "graphicalboard.h"
#include "graphicalreporter.h"

const char *kHtmlHeader =
"<html>\n"
"<head>\n"
"<title>Quackle Graphical Game Report</title>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">\n"
"</head>\n"
"<body bgcolor=white>\n"
"<h1>Graphical Game Report</h1>\n"
"<p><i>Generated by Quackle crossword game AI and analysis software</i>\n"
"<br /><a href=\"http://quackle.org\">http://quackle.org</a></p>\n"
"\n\n"
;

GraphicalReporter::GraphicalReporter(const QString &outputDirectory, bool generateImages)
	: m_output(outputDirectory), m_generateImages(generateImages)
{
}

void GraphicalReporter::reportHeader(const Quackle::Game &game)
{
	openIndex();
	m_indexStream << kHtmlHeader;
	m_indexStream << QuackleIO::Util::uvStringToQString(game.currentPosition().board().htmlKey());
}

void GraphicalReporter::reportGame(const Quackle::Game &game, Quackle::ComputerPlayer *computerPlayer)
{
	reportHeader(game);

	for (Quackle::PositionList::const_iterator it = game.history().begin(); it != game.history().end(); ++it)
	{
		reportPosition(*it, computerPlayer);
	}
}

void GraphicalReporter::reportPosition(const Quackle::GamePosition &position, Quackle::ComputerPlayer *computerPlayer)
{
	openIndex();

	const QSize pictureSize(500, 500);

	Quackle::GamePosition positionCopy = position;

	{
		QString title;

		if (!position.gameOver())
		{
			title = GraphicalBoard::tr("<h2>%1: Turn %2</h2>").arg(QuackleIO::Util::uvStringToQString(position.currentPlayer().name())).arg(position.turnNumber());
		}
		else
		{
			title = GraphicalBoard::tr("<h2>Game over.</h2>");
		}

		if (m_generateImages)
		{
			QPixmap pixmap;
			positionCopy.resetMoveMade();
			GraphicalBoardFrame::staticDrawPosition(positionCopy, pictureSize, &pixmap);

			QImage image = pixmap.toImage();

			const QString filebasename = QString("%1-%2-position.png").arg(position.turnNumber()).arg(QuackleIO::Util::uvStringToQString(position.currentPlayer().name()));
			const QString filename = makeFilename(filebasename);

			if (image.save(filename, "PNG"))
			{
				m_indexStream << QString("<a href=\"%1\">%2</a>").arg(filebasename).arg(title) << endl;
			}
			else
			{
				QMessageBox::critical(0, GraphicalBoard::tr("Error Writing File - Quacker"), GraphicalBoard::tr("Could not write image %1.").arg(filename));        
			}

			m_indexStream << "<p><img src=\"" << filebasename << "\"></p>" << endl;
		}
		else
		{
			m_indexStream << title;

			const int boardTileSize = position.gameOver()? 45 : 25;
			m_indexStream << QuackleIO::Util::sanitizeUserVisibleLetterString(QuackleIO::Util::uvStringToQString(position.board().htmlBoard(boardTileSize))) << endl;
		}
	}

	const Quackle::PlayerList players(position.endgameAdjustedScores());

	m_indexStream << "<table cellspacing=6>" << endl;
	for (Quackle::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
	{
		m_indexStream << "<tr>";

		m_indexStream << "<td>";
		if ((*it) == position.currentPlayer())
			m_indexStream << "&rarr;";
		else
			m_indexStream << "&nbsp;";
		m_indexStream << "</td>";

		m_indexStream
		<< "<td>" << QuackleIO::Util::uvStringToQString((*it).name()) << "</td>"
		<< "<td>" << QuackleIO::Util::sanitizeUserVisibleLetterString(QuackleIO::Util::uvStringToQString((*it).rack().toString())) << "</td>"
		<< "<td>" << (*it).score() << "</td>"
		<< "</tr>"
		<< endl;
	}
	m_indexStream << "</table>" << endl;

	if (computerPlayer && !position.gameOver())
	{
		computerPlayer->setPosition(position);

		if (position.committedMove().isAMove())
			computerPlayer->considerMove(position.committedMove());

		const unsigned int movesToShow = 5;
		Quackle::MoveList moves = computerPlayer->moves(movesToShow);

		if (!moves.contains(position.committedMove()))
		{
			if (moves.size() == movesToShow)
				moves.pop_back();

			moves.push_back(position.committedMove());
		}

		m_indexStream << "<ol>" << endl;
		for (Quackle::MoveList::const_iterator it = moves.begin(); it != moves.end(); ++it)
		{
			QString item;
			switch ((*it).action)
			{
			case Quackle::Move::Place:
			{
				if (m_generateImages)
				{
					QPixmap pixmap;

					positionCopy.setMoveMade(*it);
					GraphicalBoardFrame::staticDrawPosition(positionCopy, pictureSize, &pixmap);

					QImage image = pixmap.toImage();

					const QString filebasename = QString("%1-%2-%3-%4.png").arg(position.turnNumber()).arg(QuackleIO::Util::uvStringToQString(position.currentPlayer().name())).arg(QuackleIO::Util::letterStringToQString((*it).prettyTiles())).arg(QuackleIO::Util::uvStringToQString((*it).positionString()));
					const QString filename = makeFilename(filebasename);

					if (image.save(filename, "PNG"))
					{
						item = QString("<a href=\"%1\">%2</a> %3").arg(filebasename);
					}
					else
					{
						QMessageBox::critical(0, GraphicalBoard::tr("Error Writing File - Quacker"), GraphicalBoard::tr("Could not write image %1.").arg(filename));        
					}
				}
				else
				{
					item = "%1 %2";
				}

				item = item.arg(QuackleIO::Util::sanitizeUserVisibleLetterString(QuackleIO::Util::moveToDetailedString(*it))).arg((*it).score);
				break;
			}

			case Quackle::Move::Exchange:
			default:
				item = QuackleIO::Util::moveToDetailedString(*it);
				break;
			}

			if (*it == position.committedMove())
				item += QString(" &nbsp;&larr;");

			if (!item.isEmpty())
				m_indexStream << "<li>" << item << "</li>" << endl;
		}
		m_indexStream << "</ol>" << endl;
	}

	m_indexStream << "\n\n";
}

QString GraphicalReporter::makeFilename(const QString &filename) const
{
	return QString("%1/%2").arg(m_output).arg(filename);
}

void GraphicalReporter::openIndex()
{
	if (!m_indexStream.device())
	{
		m_indexFile.setFileName(m_generateImages? makeFilename("index.html") : m_output);
		if (!m_indexFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox::critical(0, GraphicalBoard::tr("Error Writing File - Quacker"), GraphicalBoard::tr("Could not open %1 for writing.").arg(m_indexFile.fileName()));        
			return;    
		}

		m_indexStream.setDevice(&m_indexFile);
	}
	m_indexStream.setCodec(QTextCodec::codecForName("UTF-8"));
}

