/*
 * optionswidget.cpp - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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

#include <QCloseEvent>
#include <QKeyEvent>

#include "optionswidget.h"
#include "editserverdlg.h"
#include "server.h"
#include "options.h"
#include "defines.h"

//--------------------------------------------------------
//---GrepShortcutKeyDialog from libpsi with some changes--
//--------------------------------------------------------
class GrepShortcutKeyDialog : public QDialog
{
	Q_OBJECT
public:
	GrepShortcutKeyDialog(QWidget* p = 0)
		: QDialog(p)
		, gotKey(false)
	{
		setAttribute(Qt::WA_DeleteOnClose);
		setModal(true);
		setWindowTitle(tr("New Shortcut"));
		QHBoxLayout *l = new QHBoxLayout(this);
		le = new QLineEdit();
		l->addWidget(le);
		QPushButton *cancelButton = new QPushButton(tr("Cancel"));
		l->addWidget(cancelButton);
		connect(cancelButton, SIGNAL(clicked()), SLOT(close()));

		displayPressedKeys(QKeySequence());

		adjustSize();
		setFixedSize(size());
	}

	void show()
	{
		QDialog::show();
		grabKeyboard();
	}

protected:
	void closeEvent(QCloseEvent *event)
	{
		releaseKeyboard();
		event->accept();
	}

	void keyPressEvent(QKeyEvent* event)
	{
		displayPressedKeys(getKeySequence(event));

		if (!isValid(event->key()) || gotKey)
			return;

		gotKey = true;
		emit newShortcutKey(getKeySequence(event));
		close();
	}

	void keyReleaseEvent(QKeyEvent* event)
	{
		displayPressedKeys(getKeySequence(event));
	}

signals:
	void newShortcutKey(const QKeySequence& key);

private:
	void displayPressedKeys(const QKeySequence& keys)
	{
		QString str = keys.toString(QKeySequence::NativeText);
		if (str.isEmpty())
			str = tr("Set Keys");
		le->setText(str);
	}

	QKeySequence getKeySequence(QKeyEvent* event) const
	{
		return QKeySequence((isValid(event->key()) ? event->key() : 0)
				    + (event->modifiers() & ~Qt::KeypadModifier));
	}

	bool isValid(int key) const
	{
		switch (key) {
		case 0:
		case Qt::Key_unknown:
			return false;
		}

		return !isModifier(key);
	}

	bool isModifier(int key) const
	{
		switch (key) {
		case Qt::Key_Shift:
		case Qt::Key_Control:
		case Qt::Key_Meta:
		case Qt::Key_Alt:
		case Qt::Key_AltGr:
		case Qt::Key_Super_L:
		case Qt::Key_Super_R:
		case Qt::Key_Menu:
			return true;
		}
		return false;
	}

	bool gotKey;
	QLineEdit* le;
};



//---------------------------------------------------
//-------------------OptionsWidget-------------------
//---------------------------------------------------
OptionsWidget::OptionsWidget(QWidget* p)
	: QWidget(p)
{
	ui_.setupUi(this);
	ui_.cb_hack->setVisible(false);

	Options* o = Options::instance();
	shortCut = o->getOption(constShortCut, QVariant(shortCut)).toString();
	format = o->getOption(constFormat, QVariant(format)).toString();
	fileName = o->getOption(constFileName, QVariant(fileName)).toString();
	servers = o->getOption(constServerList).toStringList();
	defaultAction = o->getOption(constDefaultAction, QVariant(Desktop)).toInt();

	connect(ui_.pb_add, SIGNAL(clicked()), this, SLOT(addServer()));
	connect(ui_.pb_del, SIGNAL(clicked()), this, SLOT(delServer()));
	connect(ui_.pb_edit, SIGNAL(clicked()), this, SLOT(editServer()));
	connect(ui_.lw_servers, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editServer()));
	connect(ui_.lw_servers, SIGNAL(currentRowChanged(int)), this, SLOT(applyButtonActivate()));
	connect(ui_.pb_modify, SIGNAL(clicked()), this, SLOT(requstNewShortcut()));
}


void OptionsWidget::addServer()
{
	EditServerDlg *esd = new EditServerDlg(this);
	connect(esd, SIGNAL(okPressed(QString)), this, SLOT(addNewServer(QString)));
	esd->show();
}

void OptionsWidget::delServer()
{
	Server *s = (Server*)ui_.lw_servers->currentItem();
	if(!s)
		return;
	ui_.lw_servers->removeItemWidget(s);
	delete(s);
	applyButtonActivate();
}

void OptionsWidget::editServer()
{
	Server *s = (Server*)ui_.lw_servers->currentItem();
	if(!s)
		return;
	EditServerDlg *esd = new EditServerDlg(this);
	connect(esd, SIGNAL(okPressed(QString)), this, SLOT(applyButtonActivate()));
	esd->setServer(s);
	esd->show();
}

void OptionsWidget::addNewServer(const QString& settings)
{
	Server *s = new Server(ui_.lw_servers);
	s->setFromString(settings);
	s->setText(s->displayName());

	applyButtonActivate();
}

void OptionsWidget::applyButtonActivate()
{
	ui_.cb_hack->toggle();
}

void OptionsWidget::applyOptions()
{
	Options* o = Options::instance();

	shortCut = ui_.le_shortcut->text();
	o->setOption(constShortCut, QVariant(shortCut));

	format = ui_.cb_format->currentText();
	o->setOption(constFormat, QVariant(format));

	fileName = ui_.le_filename->text();
	o->setOption(constFileName, QVariant(fileName));

	servers.clear();
	for(int i = 0; i < ui_.lw_servers->count(); i++) {
		Server *s = (Server *)ui_.lw_servers->item(i);
		servers.append(s->settingsToString());
	}
	o->setOption(constServerList, QVariant(servers));

	if(ui_.rb_desktop->isChecked())
		defaultAction = Desktop;
	else
		defaultAction = Area;
	o->setOption(constDefaultAction, defaultAction);

}

void OptionsWidget::restoreOptions()
{
	QStringList l = QStringList() << "jpg" << "png";
	ui_.cb_format->addItems(l);
	int index = ui_.cb_format->findText(format);
	if(index != -1)
		ui_.cb_format->setCurrentIndex(index);
	ui_.le_filename->setText(fileName);
	ui_.le_shortcut->setText(shortCut);
	foreach(QString settings, servers) {
		Server *s = new Server(ui_.lw_servers);
		s->setFromString(settings);
		s->setText(s->displayName());
	}
	ui_.rb_desktop->setChecked(defaultAction == Desktop);
	ui_.rb_area->setChecked(defaultAction == Area);
}

void OptionsWidget::requstNewShortcut()
{
	GrepShortcutKeyDialog *gs = new GrepShortcutKeyDialog(this);
	connect(gs, SIGNAL(newShortcutKey(QKeySequence)), this, SLOT(onNewShortcut(QKeySequence)));
	gs->show();
}

void OptionsWidget::onNewShortcut(const QKeySequence& ks)
{
	ui_.le_shortcut->setText(ks.toString(QKeySequence::NativeText));
}

#include "optionswidget.moc"
