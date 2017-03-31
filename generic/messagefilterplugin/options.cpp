/*
 * options.cpp - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "options.h"

#include "ui_options.h"
#include "optionaccessinghost.h"

#include <QComboBox>
#include <QLineEdit>
#include <QDebug>

Options::Options(const QList<Rule> &rules, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Options)
	, _optionHost(0)
	, _rules(rules)
	, _currentRule(-1)
{
	ui->setupUi(this);

    ui->btAddRule->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui->btRemoveRule->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
    ui->btUpRule->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    ui->btDownRule->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));

    ui->btAddCondition->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    ui->btRemoveCondition->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
    ui->btUpCondition->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    ui->btDownCondition->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));

	ui->wRulePane->setEnabled(false);

	updateRuleButtons();
	updateConditionButtons();

	foreach (const Rule &rule, _rules) {
		ui->lwRules->addItem(rule.name);
	}
}

Options::~Options()
{
	delete ui;
}

void Options::update()
{
}

void Options::saveSettings()
{
	if (_currentRule >= 0) {
		_rules[_currentRule].name = ui->lneRuleName->text();
		_rules[_currentRule].showMessage = ui->chkShowMessage->isChecked();
		
		for (int i = 0; i < ui->twConditions->rowCount(); ++i)
			saveCondition(_currentRule, i);
	}

	_optionHost->setPluginOption("rules.size", _rules.size());
	for (int i = 0; i < _rules.size(); ++i) {
		QString optionName = QString("rules.l%1.").arg(i);
		Rule rule = _rules.at(i);
		_optionHost->setPluginOption(optionName + "name", rule.name);
		_optionHost->setPluginOption(optionName + "show-message", rule.showMessage);
		_optionHost->setPluginOption(optionName + "conditions.size", rule.conditions.size());
		for (int j = 0; j < rule.conditions.size(); ++j) {
			Condition condition = rule.conditions.at(j);
			QString optionName1 = QString("%1conditions.l%2.").arg(optionName).arg(j);
			_optionHost->setPluginOption(optionName1 + "type", static_cast<int>(condition.type));
			_optionHost->setPluginOption(optionName1 + "comparsion", static_cast<int>(condition.comparsion));
			_optionHost->setPluginOption(optionName1 + "text", condition.text);
		}
	}
}

void Options::addRule()
{
	ui->lwRules->addItem("New rule");

	Rule rule;
	rule.name = "New rule";
	rule.showMessage = false;
	
	_rules << rule;
	ui->lwRules->setCurrentRow(ui->lwRules->count() - 1);
	addCondition();
}

void Options::removeRule()
{
	int rule = ui->lwRules->currentRow();
	ui->lwRules->setCurrentRow(-1);
	_rules.removeAt(rule);
	delete ui->lwRules->takeItem(rule);

	ui->lwRules->setCurrentRow(rule == _rules.size() ? -1 : rule);
}

void Options::upRule()
{
	int rule = ui->lwRules->currentRow();
	_rules.swap(rule, rule - 1);

	QListWidgetItem *item = ui->lwRules->takeItem(rule);
	ui->lwRules->insertItem(rule - 1, item);

	ui->lwRules->setCurrentRow(rule - 1);
}

void Options::downRule()
{
	int rule = ui->lwRules->currentRow();
	_rules.swap(rule, rule + 1);

	QListWidgetItem *item = ui->lwRules->takeItem(rule);
	ui->lwRules->insertItem(rule + 1, item);

	ui->lwRules->setCurrentRow(rule + 1);
}

void Options::updateRuleButtons()
{
	if (ui->lwRules->currentRow() < 0) {
		ui->btRemoveRule->setEnabled(false);
		ui->btUpRule->setEnabled(false);
		ui->btDownRule->setEnabled(false);
	}
	else {
		ui->btRemoveRule->setEnabled(true);
		if (ui->lwRules->currentRow() > 0)
			ui->btUpRule->setEnabled(true);
		else
			ui->btUpRule->setEnabled(false);
		
		if (ui->lwRules->currentRow() <  ui->lwRules->count() - 1)
			ui->btDownRule->setEnabled(true);
		else
			ui->btDownRule->setEnabled(false);
	}
}

void Options::addCondition()
{
	Condition condition;
	condition.type = From;
	condition.comparsion = Equal;
	condition.text = "";

	_rules[ui->lwRules->currentRow()].conditions << condition;
	setRulePane(ui->lwRules->currentRow());
}

void Options::removeCondition()
{
	int rule = ui->lwRules->currentRow();
	int condition = ui->twConditions->currentRow();
	
	_rules[rule].conditions.removeAt(condition);
	ui->twConditions->removeRow(condition);

	updateConditionButtons();
}

void Options::upCondition()
{
	int rule = ui->lwRules->currentRow();
	int condition = ui->twConditions->currentRow();
	int conditionColumn = ui->twConditions->currentColumn();

	saveCondition(rule, condition - 1);
	saveCondition(rule, condition);

	_rules[rule].conditions.swap(condition, condition - 1);

	fillCondition(condition - 1);
	fillCondition(condition);

	ui->twConditions->setCurrentCell(condition - 1, conditionColumn);
}

void Options::downCondition()
{
	int rule = ui->lwRules->currentRow();
	int condition = ui->twConditions->currentRow();
	int conditionColumn = ui->twConditions->currentColumn();

	saveCondition(rule, condition);
	saveCondition(rule, condition + 1);

	_rules[rule].conditions.swap(condition, condition + 1);

	fillCondition(condition);
	fillCondition(condition + 1);

	ui->twConditions->setCurrentCell(condition + 1, conditionColumn);
}

void Options::updateConditionButtons()
{
	if (ui->twConditions->currentRow() < 0) {
		ui->btRemoveCondition->setEnabled(false);
		ui->btUpCondition->setEnabled(false);
		ui->btDownCondition->setEnabled(false);
	}
	else {
		ui->btRemoveCondition->setEnabled(true);
		if (ui->twConditions->currentRow() > 0)
			ui->btUpCondition->setEnabled(true);
		else
			ui->btUpCondition->setEnabled(false);
		
		if (ui->twConditions->currentRow() <  ui->twConditions->rowCount() - 1)
			ui->btDownCondition->setEnabled(true);
		else
			ui->btDownCondition->setEnabled(false);
	}
}

void Options::updateRuleName(const QString &name)
{
	ui->lwRules->currentItem()->setText(name);
	_rules[ui->lwRules->currentRow()].name = name;
}

void Options::setRulePane(int row)
{
	if (_currentRule >= 0) {
		_rules[_currentRule].name = ui->lneRuleName->text();
		_rules[_currentRule].showMessage = ui->chkShowMessage->isChecked();
		
		for (int i = 0; i < ui->twConditions->rowCount(); ++i)
			saveCondition(_currentRule, i);
	}

	qDebug() << "New current row" << row;

	_currentRule = row;
	clearConditionsTable();
	if (row >= 0 && row < _rules.size()) {
		ui->wRulePane->setEnabled(true);
		Rule rule = _rules[row];
		ui->lneRuleName->setText(rule.name);
		ui->chkShowMessage->setChecked(rule.showMessage);
		QList<Condition> conditions = rule.conditions;
		for (int i = 0; i < conditions.size(); ++i) {
			ui->twConditions->insertRow(i);
			QComboBox *comboBox = new QComboBox();
			comboBox->addItem("From jid");
			comboBox->addItem("To jid");
			comboBox->addItem("From full jid");
			comboBox->addItem("To full jid");
			comboBox->addItem("Message");
			comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			ui->twConditions->setCellWidget(i, 0, comboBox);
			connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(hack()));

			comboBox = new QComboBox();
			comboBox->addItem("equal");
			comboBox->addItem("not equal");
			comboBox->addItem("contains");
			comboBox->addItem("not contains");
			comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			ui->twConditions->setCellWidget(i, 1, comboBox);
			connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(hack()));

			QLineEdit *lineEdit = new QLineEdit();
			lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			ui->twConditions->setCellWidget(i, 2, lineEdit);
			connect(lineEdit, SIGNAL(textEdited(QString)), SLOT(hack()));
			
			fillCondition(i);
		}

#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
		ui->twConditions->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
		ui->twConditions->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
	}
	else {
		ui->wRulePane->setEnabled(false);
		ui->lneRuleName->setText("");
		ui->chkShowMessage->setChecked(false);
	}
	updateRuleButtons();
	updateConditionButtons();
}

// Hack to enable Apply button
void Options::hack()
{
	ui->chkShowMessage->toggle();
	ui->chkShowMessage->toggle();
}

void Options::clearConditionsTable()
{
	while (ui->twConditions->rowCount())
		ui->twConditions->removeRow(0);
}

void Options::fillCondition(int row)
{
	int rule = ui->lwRules->currentRow();
	qobject_cast<QComboBox*>(ui->twConditions->cellWidget(row, 0))->setCurrentIndex(_rules.at(rule).conditions.at(row).type);
	qobject_cast<QComboBox*>(ui->twConditions->cellWidget(row, 1))->setCurrentIndex(_rules.at(rule).conditions.at(row).comparsion);
	qobject_cast<QLineEdit*>(ui->twConditions->cellWidget(row, 2))->setText(_rules.at(rule).conditions.at(row).text);
}

void Options::saveCondition(int rule, int row)
{
	_rules[rule].conditions[row].type = static_cast<ConditionType>(qobject_cast<QComboBox*>(ui->twConditions->cellWidget(row, 0))->currentIndex());
	_rules[rule].conditions[row].comparsion = static_cast<Comparsion>(qobject_cast<QComboBox*>(ui->twConditions->cellWidget(row, 1))->currentIndex());
	_rules[rule].conditions[row].text = qobject_cast<QLineEdit*>(ui->twConditions->cellWidget(row, 2))->text();
}
