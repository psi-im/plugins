/*
 * jd_mainwin.cpp - plugin
 * Copyright (C) 2011  Evgeny Khryukin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "jd_mainwin.h"
#include "model.h"

#include <QTextStream>
#include <QTimer>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

//#include <QDebug>

JDMainWin::JDMainWin(const QString &name, const QString &jid, int acc, QWidget *p)
	: QDialog(p, Qt::Window)
	, model_(0)
	, commands_(0)
	, refreshInProgres_(false)
	, yourJid_(name)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui_.setupUi(this);

	setWindowTitle(tr("Jabber Disk - %1").arg(name));

	model_ = new JDModel(jid, this);
	ui_.lv_disk->setModel(model_);

	commands_ = new JDCommands(acc, jid, this);

	ui_.pb_send->setShortcut(QKeySequence("Ctrl+Return"));
	connect(commands_, SIGNAL(incomingMessage(QString,JDCommands::Command)), SLOT(incomingMessage(QString,JDCommands::Command)));
	connect(commands_, SIGNAL(outgoingMessage(QString)), SLOT(outgoingMessage(QString)));
	connect(ui_.pb_refresh, SIGNAL(clicked()), SLOT(refresh()));
	connect(ui_.pb_send, SIGNAL(clicked()), SLOT(doSend()));
	connect(ui_.pb_clear, SIGNAL(clicked()), SLOT(clearLog()));

	connect(ui_.lv_disk, SIGNAL(newIndex(QModelIndex)), SLOT(indexChanged(QModelIndex)));
	connect(ui_.lv_disk, SIGNAL(contextMenu(QModelIndex)), SLOT(indexContextMenu(QModelIndex)));

	connect(model_, SIGNAL(moveItem(QString,QString)), SLOT(moveItem(QString,QString)));

	show();

	QTimer::singleShot(0, this, SLOT(refresh()));
}

JDMainWin::~JDMainWin()
{
}

void JDMainWin::incomingMessage(const QString &message, JDCommands::Command command)
{
	switch(command) {
	case JDCommands::CommandLs:
		parse(message);
		break;
	case JDCommands::CommandRm:
	case JDCommands::CommandMkDir:
	case JDCommands::CommandMv:
		QTimer::singleShot(100, this, SLOT(refresh()));
		break;
	case JDCommands::CommandGet:
	case JDCommands::CommandCd:
	case JDCommands::CommandSend:
	case JDCommands::CommandHash:
	case JDCommands::CommandLang:
	case JDCommands::CommandPwd:
	case JDCommands::CommandHelp:
	case JDCommands::CommandIntro:
	case JDCommands::CommandDu:
	case JDCommands::CommandLink:
	case JDCommands::CommandNoCommand:
		break;
	}
	appendMessage(message, false);
}

void JDMainWin::refresh()
{
	refreshInProgres_ = true;
	ui_.pb_refresh->setEnabled(false);
	ui_.pb_send->setEnabled(false);

	model_->clear();
	commands_->cd(JDModel::rootPath());

	currentDir_.clear();
	recursiveFind(currentDir_);

	ui_.lv_disk->expand(model_->rootIndex());
	ui_.lv_disk->setCurrentIndex(model_->rootIndex());

	ui_.pb_refresh->setEnabled(true);
	ui_.pb_send->setEnabled(true);
	refreshInProgres_ = false;
}

void JDMainWin::recursiveFind(const QString& dir)
{
	QString tmp = currentDir_;
	commands_->ls(dir);
	QStringList dirs = model_->dirs(dir);
	foreach(const QString& d, dirs) {
		currentDir_ += d;
		recursiveFind(currentDir_);
		currentDir_ = tmp;
	}
}

void JDMainWin::doSend()
{
	const QString mes = ui_.te_message->toPlainText();
	if(!mes.isEmpty()) {
		commands_->sendStanzaDirect(mes);
		ui_.te_message->clear();
	}
}

void JDMainWin::outgoingMessage(const QString &message)
{
	appendMessage(message);
}

void JDMainWin::appendMessage(const QString& message, bool outgoing)
{
#ifdef HAVE_QT5
	QString msg = message.toHtmlEscaped().replace("\n", "<br>");
#else
	QString msg = Qt::escape(message).replace("\n", "<br>");
#endif
	if (outgoing)
		msg = "<span style='color:blue'>" + tr("<b>You:</b> ") + msg+ "</span>";
	else
		msg = "<span style='color:red'>" + tr("<b>Disk:</b> ") + msg + "</span>";

	ui_.te_log->append(msg);
}

void JDMainWin::clearLog()
{
	ui_.te_log->clear();
}

void JDMainWin::parse(QString message)
{
	static const QRegExp dirRE("<dir> (\\S+/)");
	static const QRegExp fileRE("([0-9]+) - (.*) \\[(\\S+)\\] - (.*)");

	QTextStream str(&message, QIODevice::ReadOnly);
	while(!str.atEnd()) {
		QString line = str.readLine();
		if(dirRE.indexIn(line) != -1) {
			model_->addDir(currentDir_, dirRE.cap(1));
		}
		else if(fileRE.indexIn(line) != -1) {
			model_->addFile(currentDir_,fileRE.cap(2), fileRE.cap(3), fileRE.cap(4), fileRE.cap(1).toInt());
		}
	}
}

void JDMainWin::indexChanged(const QModelIndex& index)
{
	if(refreshInProgres_)
		return;

	const QString tmp = currentDir_;

	if(model_->data(index, JDModel::RoleType).toInt() != JDItem::File) {
		currentDir_ = model_->data(index, JDModel::RoleFullPath).toString();
	}
	else {
		currentDir_ = model_->data(index, JDModel::RoleParentPath).toString();
	}

	if(currentDir_ == JDModel::rootPath())
		currentDir_.clear();

	if(tmp != currentDir_) {
		if(!tmp.isEmpty())
			commands_->cd(JDModel::rootPath());

		if(!currentDir_.isEmpty())
			commands_->cd(currentDir_);
	}
}

void JDMainWin::indexContextMenu(const QModelIndex& index)
{
	QMenu m;
	JDItem::Type type = (JDItem::Type)index.data(JDModel::RoleType).toInt();
	QList<QAction*> aList;
	QAction* actRemove = new QAction(tr("Remove"), &m);
	QAction* actMake = new QAction(tr("Make dir"), &m);
	QAction* actGet = new QAction(tr("Get File"), &m);
	QAction* actSend = new QAction(tr("Send File"), &m);
	QAction* actHash = new QAction(tr("Hash"), &m);
	QAction* actLink = new QAction(tr("Link"), &m);
	QAction* actHelp = new QAction(tr("Help"), &m);
	QAction* actIntro = new QAction(tr("Intro"), &m);
	QAction* actDu = new QAction(tr("Statistics"), &m);
	QAction* actRename = new QAction(tr("Rename"), &m);

	QMenu move;
	move.setTitle(tr("Move to..."));
	QAction *actPublic = new QAction("public", &move);
	QAction *actAlbum = new QAction("album", &move);
	QAction *actPrivate = new QAction("private", &move);
	move.addActions(QList<QAction*>() << actPublic << actAlbum << actPrivate);

	if(type == JDItem::File) {
		aList << actRename << actGet << actSend << actLink << actHash << actRemove << move.menuAction();
	}
	else {
		aList << actMake;
	}
	if(type == JDItem::Dir) {
		aList << actRemove;
	}
	if(type == JDItem::None) {
		aList << actDu << actHelp << actIntro;
	}
	m.addActions(aList);

	QAction* result = m.exec(QCursor::pos());
	if(result == actRemove) {
		int rez = QMessageBox::question(this, tr("Remove Item"), tr("Are you sure?"),
						QMessageBox::Yes | QMessageBox::No);
		if(rez == QMessageBox::No)
			return;
		commands_->cd(JDModel::rootPath());
		commands_->rm(model_->data(index, JDModel::RoleFullPath).toString());
	}
	else if(result == actMake) {
		const QString name = QInputDialog::getText(this, tr("Input Dir Name"), QString());
		if(!name.isEmpty())
			commands_->mkDir(name);
	}
	else if(result == actGet)
		commands_->get(QString("#%1").arg(index.data(JDModel::RoleNumber).toInt()));
	else if(result == actHash)
		commands_->hash(QString("#%1").arg(index.data(JDModel::RoleNumber).toInt()));
	else if(result == actSend) {
		const QString name = QInputDialog::getText(this, tr("Input Full JID"), QString());
		if(!name.isEmpty()) {
			commands_->send(name, QString("#%1").arg(index.data(JDModel::RoleNumber).toInt()));
		}
	}
	else if(result == actDu)
		commands_->du();
	else if(result == actHelp)
		commands_->help();
	else if(result == actIntro)
		commands_->intro();
	else if(result == actLink)
		commands_->link(QString("#%1").arg(index.data(JDModel::RoleNumber).toInt()));
	else if(result == actRename) {
		const QString name = QInputDialog::getText(this, tr("Input New Name"), QString());
		if(!name.isEmpty()) {
			commands_->mv(QString("#%1").arg(index.data(JDModel::RoleNumber).toInt()), name);
		}
	}
	else if(result == actPublic || result == actAlbum || result == actPrivate) {
		const QString path = result->text();
		commands_->mv(QString("'//%1%%2/%3'").arg(yourJid_, model_->disk() ,index.data(JDModel::RoleFullPath).toString()),
					  QString("'//%1%%2/%3'").arg(yourJid_, path ,index.data(JDModel::RoleName).toString()));
	}
}

void JDMainWin::moveItem(const QString &oldPath, const QString &newPath)
{
	commands_->cd(JDModel::rootPath());
	commands_->mv(oldPath, newPath);
}

