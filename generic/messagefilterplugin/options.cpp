/*
 * options.cpp - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "options.h"

#include "optionaccessinghost.h"
#include "ui_options.h"

#include <QComboBox>
#include <QDebug>
#include <QLineEdit>

Options::Options(const QList<Rule> &rules, QWidget *parent) :
    QWidget(parent), m_ui(new Ui::Options), m_optionHost(nullptr), m_rules(rules), m_currentRule(-1)
{
    m_ui->setupUi(this);

    m_ui->btAddRule->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    m_ui->btRemoveRule->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
    m_ui->btUpRule->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    m_ui->btDownRule->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));

    m_ui->btAddCondition->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/list-add.png")));
    m_ui->btRemoveCondition->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/list-remove.png")));
    m_ui->btUpCondition->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    m_ui->btDownCondition->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));

    m_ui->wRulePane->setEnabled(false);

    updateRuleButtons();
    updateConditionButtons();

    for (const Rule &rule : m_rules) {
        m_ui->lwRules->addItem(rule.name);
    }
}

Options::~Options() { delete m_ui; }

void Options::update() { }

void Options::saveSettings()
{
    if (m_currentRule >= 0) {
        m_rules[m_currentRule].name        = m_ui->lneRuleName->text();
        m_rules[m_currentRule].showMessage = m_ui->chkShowMessage->isChecked();

        for (int i = 0; i < m_ui->twConditions->rowCount(); ++i)
            saveCondition(m_currentRule, i);
    }

    m_optionHost->setPluginOption("rules.size", m_rules.size());
    for (int i = 0; i < m_rules.size(); ++i) {
        QString optionName = QString("rules.l%1.").arg(i);
        Rule    rule       = m_rules.at(i);
        m_optionHost->setPluginOption(optionName + "name", rule.name);
        m_optionHost->setPluginOption(optionName + "show-message", rule.showMessage);
        m_optionHost->setPluginOption(optionName + "conditions.size", rule.conditions.size());
        for (int j = 0; j < rule.conditions.size(); ++j) {
            Condition condition   = rule.conditions.at(j);
            QString   optionName1 = QString("%1conditions.l%2.").arg(optionName).arg(j);
            m_optionHost->setPluginOption(optionName1 + "type", static_cast<int>(condition.type));
            m_optionHost->setPluginOption(optionName1 + "comparison", static_cast<int>(condition.comparison));
            m_optionHost->setPluginOption(optionName1 + "text", condition.text);
        }
    }
}

void Options::addRule()
{
    m_ui->lwRules->addItem("New rule");

    Rule rule;
    rule.name        = "New rule";
    rule.showMessage = false;

    m_rules << rule;
    m_ui->lwRules->setCurrentRow(m_ui->lwRules->count() - 1);
    addCondition();
}

void Options::removeRule()
{
    int rule = m_ui->lwRules->currentRow();
    m_ui->lwRules->setCurrentRow(-1);
    m_rules.removeAt(rule);
    delete m_ui->lwRules->takeItem(rule);

    m_ui->lwRules->setCurrentRow(rule == m_rules.size() ? -1 : rule);
}

void Options::upRule()
{
    int rule = m_ui->lwRules->currentRow();
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    m_rules.swap(rule, rule - 1);
#else
    m_rules.swapItemsAt(rule, rule - 1);
#endif

    QListWidgetItem *item = m_ui->lwRules->takeItem(rule);
    m_ui->lwRules->insertItem(rule - 1, item);

    m_ui->lwRules->setCurrentRow(rule - 1);
}

void Options::downRule()
{
    int rule = m_ui->lwRules->currentRow();
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    m_rules.swap(rule, rule + 1);
#else
    m_rules.swapItemsAt(rule, rule + 1);
#endif

    QListWidgetItem *item = m_ui->lwRules->takeItem(rule);
    m_ui->lwRules->insertItem(rule + 1, item);

    m_ui->lwRules->setCurrentRow(rule + 1);
}

void Options::updateRuleButtons()
{
    if (m_ui->lwRules->currentRow() < 0) {
        m_ui->btRemoveRule->setEnabled(false);
        m_ui->btUpRule->setEnabled(false);
        m_ui->btDownRule->setEnabled(false);
    } else {
        m_ui->btRemoveRule->setEnabled(true);
        if (m_ui->lwRules->currentRow() > 0)
            m_ui->btUpRule->setEnabled(true);
        else
            m_ui->btUpRule->setEnabled(false);

        if (m_ui->lwRules->currentRow() < m_ui->lwRules->count() - 1)
            m_ui->btDownRule->setEnabled(true);
        else
            m_ui->btDownRule->setEnabled(false);
    }
}

void Options::addCondition()
{
    Condition condition;
    condition.type       = From;
    condition.comparison = Equal;
    condition.text       = "";

    m_rules[m_ui->lwRules->currentRow()].conditions << condition;
    setRulePane(m_ui->lwRules->currentRow());
}

void Options::removeCondition()
{
    int rule      = m_ui->lwRules->currentRow();
    int condition = m_ui->twConditions->currentRow();

    m_rules[rule].conditions.removeAt(condition);
    m_ui->twConditions->removeRow(condition);

    updateConditionButtons();
}

void Options::upCondition()
{
    int rule            = m_ui->lwRules->currentRow();
    int condition       = m_ui->twConditions->currentRow();
    int conditionColumn = m_ui->twConditions->currentColumn();

    saveCondition(rule, condition - 1);
    saveCondition(rule, condition);
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    m_rules[rule].conditions.swap(condition, condition - 1);
#else
    m_rules[rule].conditions.swapItemsAt(condition, condition - 1);
#endif

    fillCondition(condition - 1);
    fillCondition(condition);

    m_ui->twConditions->setCurrentCell(condition - 1, conditionColumn);
}

void Options::downCondition()
{
    int rule            = m_ui->lwRules->currentRow();
    int condition       = m_ui->twConditions->currentRow();
    int conditionColumn = m_ui->twConditions->currentColumn();

    saveCondition(rule, condition);
    saveCondition(rule, condition + 1);
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    m_rules[rule].conditions.swap(condition, condition + 1);
#else
    m_rules[rule].conditions.swapItemsAt(condition, condition + 1);
#endif

    fillCondition(condition);
    fillCondition(condition + 1);

    m_ui->twConditions->setCurrentCell(condition + 1, conditionColumn);
}

void Options::updateConditionButtons()
{
    if (m_ui->twConditions->currentRow() < 0) {
        m_ui->btRemoveCondition->setEnabled(false);
        m_ui->btUpCondition->setEnabled(false);
        m_ui->btDownCondition->setEnabled(false);
    } else {
        m_ui->btRemoveCondition->setEnabled(true);
        if (m_ui->twConditions->currentRow() > 0)
            m_ui->btUpCondition->setEnabled(true);
        else
            m_ui->btUpCondition->setEnabled(false);

        if (m_ui->twConditions->currentRow() < m_ui->twConditions->rowCount() - 1)
            m_ui->btDownCondition->setEnabled(true);
        else
            m_ui->btDownCondition->setEnabled(false);
    }
}

void Options::updateRuleName(const QString &name)
{
    m_ui->lwRules->currentItem()->setText(name);
    m_rules[m_ui->lwRules->currentRow()].name = name;
}

void Options::setRulePane(int row)
{
    if (m_currentRule >= 0) {
        m_rules[m_currentRule].name        = m_ui->lneRuleName->text();
        m_rules[m_currentRule].showMessage = m_ui->chkShowMessage->isChecked();

        for (int i = 0; i < m_ui->twConditions->rowCount(); ++i)
            saveCondition(m_currentRule, i);
    }

    qDebug() << "New current row" << row;

    m_currentRule = row;
    clearConditionsTable();
    if (row >= 0 && row < m_rules.size()) {
        m_ui->wRulePane->setEnabled(true);
        Rule rule = m_rules[row];
        m_ui->lneRuleName->setText(rule.name);
        m_ui->chkShowMessage->setChecked(rule.showMessage);
        QList<Condition> conditions = rule.conditions;
        for (int i = 0; i < conditions.size(); ++i) {
            m_ui->twConditions->insertRow(i);
            QComboBox *comboBox = new QComboBox();
            comboBox->addItem("From jid");
            comboBox->addItem("To jid");
            comboBox->addItem("From full jid");
            comboBox->addItem("To full jid");
            comboBox->addItem("Message");
            comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            m_ui->twConditions->setCellWidget(i, 0, comboBox);
            connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(hack()));

            comboBox = new QComboBox();
            comboBox->addItem("equal");
            comboBox->addItem("not equal");
            comboBox->addItem("contains");
            comboBox->addItem("not contains");
            comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            m_ui->twConditions->setCellWidget(i, 1, comboBox);
            connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(hack()));

            QLineEdit *lineEdit = new QLineEdit();
            lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            m_ui->twConditions->setCellWidget(i, 2, lineEdit);
            connect(lineEdit, &QLineEdit::textEdited, this, &Options::hack);

            fillCondition(i);
        }

#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
        m_ui->twConditions->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
        m_ui->twConditions->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    } else {
        m_ui->wRulePane->setEnabled(false);
        m_ui->lneRuleName->setText("");
        m_ui->chkShowMessage->setChecked(false);
    }
    updateRuleButtons();
    updateConditionButtons();
}

// Hack to enable Apply button
void Options::hack()
{
    m_ui->chkShowMessage->toggle();
    m_ui->chkShowMessage->toggle();
}

void Options::clearConditionsTable()
{
    while (m_ui->twConditions->rowCount())
        m_ui->twConditions->removeRow(0);
}

void Options::fillCondition(int row)
{
    int rule = m_ui->lwRules->currentRow();
    qobject_cast<QComboBox *>(m_ui->twConditions->cellWidget(row, 0))
        ->setCurrentIndex(m_rules.at(rule).conditions.at(row).type);
    qobject_cast<QComboBox *>(m_ui->twConditions->cellWidget(row, 1))
        ->setCurrentIndex(m_rules.at(rule).conditions.at(row).comparison);
    qobject_cast<QLineEdit *>(m_ui->twConditions->cellWidget(row, 2))->setText(m_rules.at(rule).conditions.at(row).text);
}

void Options::saveCondition(int rule, int row)
{
    m_rules[rule].conditions[row].type
        = static_cast<ConditionType>(qobject_cast<QComboBox *>(m_ui->twConditions->cellWidget(row, 0))->currentIndex());
    m_rules[rule].conditions[row].comparison
        = static_cast<Comparison>(qobject_cast<QComboBox *>(m_ui->twConditions->cellWidget(row, 1))->currentIndex());
    m_rules[rule].conditions[row].text = qobject_cast<QLineEdit *>(m_ui->twConditions->cellWidget(row, 2))->text();
}
