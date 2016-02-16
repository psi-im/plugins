/*
 * translateplugin.cpp - plugin
 * Copyright (C) 2009-2010  Kravtsov Nikolai
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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QApplication>
#include <QMap>
#include <QAction>
#include "psiplugin.h"
#include "shortcutaccessor.h"
#include "shortcutaccessinghost.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "activetabaccessor.h"
#include "activetabaccessinghost.h"
#include "plugininfoprovider.h"
#include "chattabaccessor.h"

#define constOld "oldsymbol"
#define constNew "newsymbol"
#define constShortCut "shortcut"
#define constNotTranslate "nottranslate"
#define constVersion "0.4.6"

static const QString mucData = "groupchat";
static const QString chatData = "chat";

class TranslatePlugin : public QObject
					  , public PsiPlugin
					  , public OptionAccessor
					  , public ShortcutAccessor
					  , public ActiveTabAccessor
					  , public PluginInfoProvider
					  , public ChatTabAccessor
{
	Q_OBJECT
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "com.psi-plus.TranslatePlugin")
#endif
	Q_INTERFACES(PsiPlugin OptionAccessor ShortcutAccessor ActiveTabAccessor PluginInfoProvider ChatTabAccessor)

public:
	TranslatePlugin();
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();

	virtual void applyOptions();
	virtual void restoreOptions();

	// OptionAccessor
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void optionChanged(const QString& option);

	// ShortcutsAccessor
	virtual void setShortcutAccessingHost(ShortcutAccessingHost* host);
	virtual void setShortcuts();
	//ActiveTabAccessor
	virtual void setActiveTabAccessingHost(ActiveTabAccessingHost* host);
	//Tabs
	virtual void setupChatTab(QWidget* tab, int account, const QString& contact);
	virtual void setupGCTab(QWidget* tab, int account, const QString& contact);
	virtual bool appendingChatMessage(int account, const QString& contact,
					  QString& body, QDomElement& html, bool local);

	virtual QString pluginInfo();
	virtual QPixmap icon() const;

private slots:
	void trans();
	void addToMap();
	void del();
	void grep();
	void onNewShortcutKey(QKeySequence);
	void changeItem(int,int);
	void storeItem(QTableWidgetItem*);
	void restoreMap();
	void hack();
	void actionDestroyed(QObject* obj);

private:
	void setupTab(QWidget* tab, const QString& data);
	bool enabled_;
	bool notTranslate;
	QMap<QString,QString> map;
	QMap<QString,QString> mapBakup;
	QTableWidget * table;
	QLineEdit *shortCutWidget;
	OptionAccessingHost* psiOptions;
	ShortcutAccessingHost* psiShortcuts;
	ActiveTabAccessingHost* activeTab;
	QString shortCut;
	QCheckBox *check_button;
	QString storage;
	QPointer<QWidget> options_;
	QList<QAction *> actions_;
};

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN(TranslatePlugin);
#endif

TranslatePlugin::TranslatePlugin()
	: enabled_(false)
	, notTranslate(false)
	, table(0)
	, shortCutWidget(0)
	, psiOptions(0)
	, psiShortcuts(0)
	, activeTab(0)
	, shortCut("Alt+Ctrl+t")
	, check_button(0)
{
	map.insert("~",QString::fromUtf8("Ё")); map.insert(QString::fromUtf8("Ё"),"~");
	map.insert("`",QString::fromUtf8("ё")); map.insert(QString::fromUtf8("ё"),"`");
	map.insert("#",QString::fromUtf8("№")); map.insert(QString::fromUtf8("№"),"#");
	map.insert("q",QString::fromUtf8("й")); map.insert(QString::fromUtf8("й"),"q");
	map.insert("w",QString::fromUtf8("ц")); map.insert(QString::fromUtf8("ц"),"w");
	map.insert("e",QString::fromUtf8("у")); map.insert(QString::fromUtf8("у"),"e");
	map.insert("r",QString::fromUtf8("к")); map.insert(QString::fromUtf8("к"),"r");
	map.insert("t",QString::fromUtf8("е")); map.insert(QString::fromUtf8("е"),"t");
	map.insert("y",QString::fromUtf8("н")); map.insert(QString::fromUtf8("н"),"y");
	map.insert("u",QString::fromUtf8("г")); map.insert(QString::fromUtf8("г"),"u");
	map.insert("i",QString::fromUtf8("ш")); map.insert(QString::fromUtf8("ш"),"i");
	map.insert("o",QString::fromUtf8("щ")); map.insert(QString::fromUtf8("щ"),"o");
	map.insert("p",QString::fromUtf8("з")); map.insert(QString::fromUtf8("з"),"p");
	map.insert("a",QString::fromUtf8("ф")); map.insert(QString::fromUtf8("ф"),"a");
	map.insert("s",QString::fromUtf8("ы")); map.insert(QString::fromUtf8("ы"),"s");
	map.insert("d",QString::fromUtf8("в")); map.insert(QString::fromUtf8("в"),"d");
	map.insert("f",QString::fromUtf8("а")); map.insert(QString::fromUtf8("а"),"f");
	map.insert("g",QString::fromUtf8("п")); map.insert(QString::fromUtf8("п"),"g");
	map.insert("h",QString::fromUtf8("р")); map.insert(QString::fromUtf8("р"),"h");
	map.insert("j",QString::fromUtf8("о")); map.insert(QString::fromUtf8("о"),"j");
	map.insert("k",QString::fromUtf8("л")); map.insert(QString::fromUtf8("л"),"k");
	map.insert("l",QString::fromUtf8("д")); map.insert(QString::fromUtf8("д"),"l");
	map.insert("z",QString::fromUtf8("я")); map.insert(QString::fromUtf8("я"),"z");
	map.insert("x",QString::fromUtf8("ч")); map.insert(QString::fromUtf8("ч"),"x");
	map.insert("c",QString::fromUtf8("с")); map.insert(QString::fromUtf8("с"),"c");
	map.insert("v",QString::fromUtf8("м")); map.insert(QString::fromUtf8("м"),"v");
	map.insert("b",QString::fromUtf8("и")); map.insert(QString::fromUtf8("и"),"b");
	map.insert("n",QString::fromUtf8("т")); map.insert(QString::fromUtf8("т"),"n");
	map.insert("m",QString::fromUtf8("ь")); map.insert(QString::fromUtf8("ь"),"m");
	map.insert("Q",QString::fromUtf8("Й")); map.insert(QString::fromUtf8("Й"),"Q");
	map.insert("W",QString::fromUtf8("Ц")); map.insert(QString::fromUtf8("Ц"),"W");
	map.insert("E",QString::fromUtf8("У")); map.insert(QString::fromUtf8("У"),"E");
	map.insert("R",QString::fromUtf8("К")); map.insert(QString::fromUtf8("К"),"R");
	map.insert("T",QString::fromUtf8("Е")); map.insert(QString::fromUtf8("Е"),"T");
	map.insert("Y",QString::fromUtf8("Н")); map.insert(QString::fromUtf8("Н"),"Y");
	map.insert("U",QString::fromUtf8("Г")); map.insert(QString::fromUtf8("Г"),"U");
	map.insert("I",QString::fromUtf8("Ш")); map.insert(QString::fromUtf8("Ш"),"I");
	map.insert("O",QString::fromUtf8("Щ")); map.insert(QString::fromUtf8("Щ"),"O");
	map.insert("P",QString::fromUtf8("З")); map.insert(QString::fromUtf8("З"),"P");
	map.insert("A",QString::fromUtf8("Ф")); map.insert(QString::fromUtf8("Ф"),"A");
	map.insert("S",QString::fromUtf8("Ы")); map.insert(QString::fromUtf8("Ы"),"S");
	map.insert("D",QString::fromUtf8("В")); map.insert(QString::fromUtf8("В"),"D");
	map.insert("F",QString::fromUtf8("А")); map.insert(QString::fromUtf8("А"),"F");
	map.insert("G",QString::fromUtf8("П")); map.insert(QString::fromUtf8("П"),"G");
	map.insert("H",QString::fromUtf8("Р")); map.insert(QString::fromUtf8("Р"),"H");
	map.insert("J",QString::fromUtf8("О")); map.insert(QString::fromUtf8("О"),"J");
	map.insert("K",QString::fromUtf8("Л")); map.insert(QString::fromUtf8("Л"),"K");
	map.insert("L",QString::fromUtf8("Д")); map.insert(QString::fromUtf8("Д"),"L");
	map.insert("Z",QString::fromUtf8("Я")); map.insert(QString::fromUtf8("Я"),"Z");
	map.insert("X",QString::fromUtf8("Ч")); map.insert(QString::fromUtf8("Ч"),"X");
	map.insert("C",QString::fromUtf8("С")); map.insert(QString::fromUtf8("С"),"C");
	map.insert("V",QString::fromUtf8("М")); map.insert(QString::fromUtf8("М"),"V");
	map.insert("B",QString::fromUtf8("И")); map.insert(QString::fromUtf8("И"),"B");
	map.insert("N",QString::fromUtf8("Т")); map.insert(QString::fromUtf8("Т"),"N");
	map.insert("M",QString::fromUtf8("Ь")); map.insert(QString::fromUtf8("Ь"),"M");
	map.insert("[",QString::fromUtf8("х")); map.insert(QString::fromUtf8("х"),"[");
	map.insert("{",QString::fromUtf8("Х")); map.insert(QString::fromUtf8("Х"),"{");
	map.insert("]",QString::fromUtf8("ъ")); map.insert(QString::fromUtf8("ъ"),"]");
	map.insert("}",QString::fromUtf8("Ъ")); map.insert(QString::fromUtf8("Ъ"),"}");
	map.insert("$",QString::fromUtf8(";")); map.insert(QString::fromUtf8(";"),"$");
	map.insert(";",QString::fromUtf8("ж")); map.insert(QString::fromUtf8("ж"),";");
	map.insert("^",QString::fromUtf8(":")); map.insert(QString::fromUtf8(":"),"^");
	map.insert(":",QString::fromUtf8("Ж")); map.insert(QString::fromUtf8("Ж"),":");
	map.insert("'",QString::fromUtf8("э")); map.insert(QString::fromUtf8("э"),"'");
	map.insert("@",QString::fromUtf8("\"")); map.insert(QString::fromUtf8("\""),"@");
	map.insert("\"",QString::fromUtf8("Э")); map.insert(QString::fromUtf8("Э"),"\"");
	map.insert("<",QString::fromUtf8("Б")); map.insert(QString::fromUtf8("Б"),"<");
	map.insert(">",QString::fromUtf8("Ю")); map.insert(QString::fromUtf8("Ю"),">");
	map.insert("&",QString::fromUtf8("?")); map.insert(QString::fromUtf8("?"),"&");
	map.insert("?",QString::fromUtf8(",")); map.insert(QString::fromUtf8(","),"?");
	map.insert(",",QString::fromUtf8("б")); map.insert(QString::fromUtf8("б"),",");
	map.insert("|",QString::fromUtf8("/")); map.insert(QString::fromUtf8("/"),"|");
	map.insert("/",QString::fromUtf8(".")); map.insert(QString::fromUtf8("."),"/");
	map.insert(".",QString::fromUtf8("ю")); map.insert(QString::fromUtf8("ю"),".");

	mapBakup = map;
}

QString TranslatePlugin::name() const
{
	return "Translate Plugin";
}

QString TranslatePlugin::shortName() const
{
	return "Translate";
}

QString TranslatePlugin::version() const
{
	return constVersion;
}

QWidget* TranslatePlugin::options()
{
	if (!enabled_) {
		return 0;
	}
	options_ = new QWidget();
	table = new QTableWidget(options_);
	table->setColumnCount(2);
	QStringList header;
	header <<tr("from")<<tr("to");
	table->setHorizontalHeaderLabels(header);
	table->verticalHeader()->setVisible(false);
	table->setTextElideMode(Qt::ElideMiddle);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setEditTriggers(QAbstractItemView::DoubleClicked);
	table->verticalHeader()->setDefaultSectionSize(20);
	table->verticalHeader()->setMinimumSectionSize(20);
	table->horizontalHeader()->setDefaultSectionSize(50);
	table->horizontalHeader()->setMinimumSectionSize(20);
	table->setColumnWidth(0,50);
	table->setColumnWidth(1,50);
	table->setMaximumWidth(120);
	QHBoxLayout *hBox = new QHBoxLayout(options_);
	QVBoxLayout *leftSide = new QVBoxLayout;
	leftSide->addWidget(table);
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	QPushButton *addButton = new QPushButton(tr("Add"), options_);
	QPushButton *delButton = new QPushButton(tr("Delete"), options_);
	buttonLayout->addWidget(addButton);
	buttonLayout->addWidget(delButton);
	leftSide->addLayout(buttonLayout);
	hBox->addLayout(leftSide);
	QVBoxLayout *rightSide = new QVBoxLayout;
	rightSide->addWidget(new QLabel(tr("ShortCut:")),5,Qt::AlignTop);
	QHBoxLayout *shortBox = new QHBoxLayout;
	shortCutWidget = new QLineEdit(options_);
	shortCutWidget->setFixedWidth(100);
	shortCutWidget->setText(shortCut);
	shortCutWidget->setDisabled(true);
	QPushButton * modShortCut = new QPushButton(tr("Modify"), options_);
	shortBox->addWidget(shortCutWidget,0,Qt::AlignLeft);
	shortBox->addWidget(modShortCut,200,Qt::AlignLeft);
	rightSide->addLayout(shortBox,30);
	check_button = new QCheckBox(tr("Not translating \"Nickname:\""), options_);
	check_button->setChecked(notTranslate);
	check_button->setProperty("isOption",true);
	rightSide->addWidget(check_button,30,Qt::AlignTop);
	QPushButton *restoreButton = new QPushButton(tr("Restore Defaults Settings"), options_);
	restoreButton->setFixedWidth(220);
	rightSide->addWidget(restoreButton,30,Qt::AlignBottom);
	if (!map.isEmpty()) {
		foreach(QString symbol, map.keys()){
			table->insertRow(table->rowCount());
			table->setItem(table->rowCount()-1,0,new QTableWidgetItem(symbol));
			table->setItem(table->rowCount()-1,1,new QTableWidgetItem(map.value(symbol)));
		}
	}
	hBox->addLayout(rightSide);
	connect(delButton,SIGNAL(clicked()),this,SLOT(del()));
	connect(addButton,SIGNAL(clicked()),this,SLOT(addToMap()));
	connect(modShortCut,SIGNAL(clicked()),this,SLOT(grep()));
	connect(restoreButton,SIGNAL(clicked()),this,SLOT(restoreMap()));
	connect(table,SIGNAL(cellChanged(int,int)),this,SLOT(changeItem(int,int)));
	connect(table,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(storeItem(QTableWidgetItem*)));
	return options_;
}

bool TranslatePlugin::enable()
{
	enabled_ = true;

	shortCut = psiOptions->getPluginOption(constShortCut, shortCut).toString();
	notTranslate = psiOptions->getPluginOption(constNotTranslate, notTranslate).toBool();
	//	psiShortcuts->connectShortcut(QKeySequence(shortCut),this, SLOT(trans()));

	foreach(QAction* act, actions_) {
		act->setShortcut(QKeySequence(shortCut));
	}

	QStringList oldList = psiOptions->getPluginOption(constOld, QStringList(map.keys())).toStringList();
	QStringList newList = psiOptions->getPluginOption(constNew, QStringList(map.values())).toStringList();
	int iterator = 0;
	map.clear();
	foreach(const QString& symbol, oldList){
		map.insert(symbol, newList.at(iterator++));
	}

	return true;
}

bool TranslatePlugin::disable()
{
	enabled_ = false;
	foreach(QAction* act, actions_) {
		act->disconnect(this, SLOT(trans()));
	}

	//	psiShortcuts->disconnectShortcut(QKeySequence(shortCut),this, SLOT(trans()));
	return true;
}

void TranslatePlugin::trans()
{
	if(!enabled_)
		return;

	QTextEdit * ed = activeTab->getEditBox();
	if (!ed) {
		return;
	}

	QTextCursor c = ed->textCursor();
	static QRegExp link("(xmpp:|mailto:|http://|https://|ftp://|news://|ed2k://|www.|ftp.)\\S+", Qt::CaseInsensitive);
	QStringList newStrings;

	bool isMuc = false;
	QAction* act = dynamic_cast<QAction*>(sender());
	if(act && act->data().toString() == mucData)
		isMuc = true;

	QString toReverse = c.selectedText();
	bool isSelect = true;
	QString nick("");
	if (toReverse.isEmpty()) {
		toReverse = ed->toPlainText();
		if (notTranslate && isMuc) {
			int index = toReverse.indexOf(":") + 1;
			nick = toReverse.left(index);
			toReverse = toReverse.right(toReverse.size() - index);
		}
		isSelect = false;
	}
	if(!nick.isEmpty())
		newStrings.append(nick);

	//запоминаем позицию курсора
	int pos = c.position();

	int index = link.indexIn(toReverse);
	while(index != -1 && !isSelect) {
		QString newStr;
		QString oldStr = toReverse.left(index);
		foreach(const QString& symbol, oldStr) {
			newStr.append(map.value(symbol,symbol));
		}
		newStrings << newStr << link.cap();
		toReverse = toReverse.right(toReverse.size() - (index + link.matchedLength()));
		index = link.indexIn(toReverse);
	}

	QString newStr;
	foreach(const QString& symbol, toReverse) {
		newStr.append(map.value(symbol,symbol));
	}
	newStrings << newStr;

	QString newString = newStrings.join("");

	if (!isSelect) {
		ed->setPlainText(newString);
		//восстанавливаем позицию курсора
		c.setPosition( pos );
		ed->setTextCursor( c );
	}
	else {
		int end = c.selectionEnd();
		int start = c.selectionStart();
		ed->textCursor().clearSelection();
		ed->textCursor().insertText(newString);
		c = ed->textCursor();
		if (pos == start) {
			c.setPosition(end);
			c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, end-start);
		}
		else {
			c.setPosition(start);
			c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, end-start);
		}
		ed->setTextCursor( c );
	}
}

void TranslatePlugin::addToMap()
{
	if (options_) {
		int curRow = table->currentRow();
		if (curRow == -1) {
			curRow = 0;
		}
		table->insertRow(curRow);
		table->setItem(curRow,0,new QTableWidgetItem());
		table->setItem(curRow,1,new QTableWidgetItem());

		hack();
	}
}

void TranslatePlugin::setOptionAccessingHost(OptionAccessingHost* host)
{
	psiOptions = host;
}

void TranslatePlugin::optionChanged(const QString& option)
{
	Q_UNUSED(option);
}

void TranslatePlugin::setShortcutAccessingHost(ShortcutAccessingHost* host)
{
	psiShortcuts = host;
}

void TranslatePlugin::setShortcuts()
{
	//	if (enabled_) {
	//		psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(trans()));
	//	}
}

void TranslatePlugin::applyOptions()
{
	if (!options_)
		return;

	//	psiShortcuts->disconnectShortcut(QKeySequence(shortCut), this, SLOT(trans()));
	shortCut = shortCutWidget->text();
	psiOptions->setPluginOption(constShortCut, shortCut);
	foreach(QAction* act, actions_) {
		act->setShortcut(QKeySequence(shortCut));
	}

	//	psiShortcuts->connectShortcut(QKeySequence(shortCut), this, SLOT(trans()));

	notTranslate = check_button->isChecked();
	psiOptions->setPluginOption(constNotTranslate, notTranslate);

	map.clear();
	int count = table->rowCount();
	for (int row = 0 ; row < count; row++) {
		if (!table->item(row,0)->text().isEmpty() && !table->item(row,1)->text().isEmpty()) {
			map.insert(table->item(row,0)->text().left(1),table->item(row,1)->text());
		}
	}

	psiOptions->setPluginOption(constOld, QStringList(map.keys()));
	psiOptions->setPluginOption(constNew, QStringList(map.values()));
}

void TranslatePlugin::restoreOptions()
{
	if (!options_)
		return;

	shortCutWidget->setText(shortCut);
	check_button->setChecked(notTranslate);
	foreach(const QString& symbol, map.keys()) {
		table->insertRow(table->rowCount());
		table->setItem(table->rowCount()-1,0,new QTableWidgetItem(symbol));
		table->setItem(table->rowCount()-1,1,new QTableWidgetItem(map.value(symbol)));
	}
}


void TranslatePlugin::del()
{
	if (table->currentRow() == -1) {
		return;
	}
	table->removeRow(table->currentRow());
	hack();
}

void TranslatePlugin::grep()
{
	psiShortcuts->requestNewShortcut(this, SLOT(onNewShortcutKey(QKeySequence)));
}

void TranslatePlugin::onNewShortcutKey(QKeySequence ks)
{
	shortCutWidget->setText(ks.toString(QKeySequence::NativeText));
}

void TranslatePlugin::changeItem(int row,int column)
{
	if (column == 0 && !storage.isEmpty()) {
		//если первая колонка, то её менять нельзя, возвращаем старое значение
		table->item(row,column)->setText(storage);
	} else {
		//иначе приравниваем ячейке значение первого символа
		if (table->item(row,column)->text().isEmpty()) {
			table->item(row,column)->setText(storage);
		} else {
			table->item(row,column)->setText(table->item(row,column)->text().left(1));
		}
	}
	hack();
}

void TranslatePlugin::storeItem(QTableWidgetItem* item)
{
	storage = item->text();
}

void TranslatePlugin::restoreMap()
{
	disconnect(table,SIGNAL(cellChanged(int,int)),this,SLOT(changeItem(int,int)));
	table->clear();
	table->setRowCount(0);
	foreach(const QString& symbol, mapBakup.keys()){
		table->insertRow(table->rowCount());
		table->setItem(table->rowCount()-1,0,new QTableWidgetItem(symbol));
		table->setItem(table->rowCount()-1,1,new QTableWidgetItem(mapBakup.value(symbol)));
	}
	connect(table,SIGNAL(cellChanged(int,int)),this,SLOT(changeItem(int,int)));
	hack();
}

void TranslatePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost* host)
{
	activeTab = host;
}

void TranslatePlugin::hack()
{
	check_button->toggle();
	check_button->toggle();
}

void TranslatePlugin::setupTab(QWidget* tab, const QString& data)
{
	QAction* act = new QAction(tab);
	tab->addAction(act);
	act->setData(data);
	act->setShortcut(QKeySequence(shortCut));
	act->setShortcutContext(Qt::WindowShortcut);
	connect(act, SIGNAL(triggered()), SLOT(trans()));
	connect(act, SIGNAL(destroyed(QObject*)), SLOT(actionDestroyed(QObject*)));
	actions_.append(act);
}

void TranslatePlugin::setupChatTab(QWidget* tab, int /*account*/, const QString& /*contact*/)
{
	setupTab(tab, chatData);
}

void TranslatePlugin::setupGCTab(QWidget* tab, int /*account*/, const QString& /*contact*/)
{
	setupTab(tab, mucData);
}

bool TranslatePlugin::appendingChatMessage(int/* account*/, const QString &/*contact*/,
					   QString &/*body*/, QDomElement &/*html*/, bool /*local*/)
{
	return false;
}

void TranslatePlugin::actionDestroyed(QObject *obj)
{
	QAction* act = static_cast<QAction*>(obj);
	actions_.removeAll(act);
}

QString TranslatePlugin::pluginInfo()
{
	return tr("Author: ") +	 "VampiRUS\n\n"
	       + trUtf8("This plugin allows you to convert selected text into another language.\n");
}

QPixmap TranslatePlugin::icon() const
{
	return QPixmap(":/icons/translate.png");
}

#include "translateplugin.moc"
