/*
 * messagefilter.cpp - plugin main class
 *
 * Copyright (C) 2015  Ivan Romanov <drizt@land.ru>
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

#include "messagefilter.h"

#include "options.h"

#include <accountinfoaccessinghost.h>
#include <activetabaccessinghost.h>
#include <iconfactoryaccessinghost.h>
#include <optionaccessinghost.h>
#include <psiaccountcontrollinghost.h>
#include <stanzasendinghost.h>

#include <QCursor>
#include <QDomElement>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

MessageFilter::MessageFilter() :
    _enabled(false), _optionsForm(nullptr), _accountHost(nullptr), _optionHost(nullptr), _stanzaSending(nullptr),
    _accountInfo(nullptr), _rules(QList<Rule>())
{
}

MessageFilter::~MessageFilter() { }

QWidget *MessageFilter::options()
{
    if (!_enabled) {
        return nullptr;
    }

    loadRules();
    _optionsForm = new Options(_rules);
    _optionsForm->setOptionAccessingHost(_optionHost);
    return qobject_cast<QWidget *>(_optionsForm);
}

bool MessageFilter::enable()
{
    _enabled = true;
    loadRules();
    return true;
}

bool MessageFilter::disable()
{
    _enabled = false;
    return true;
}

void MessageFilter::applyOptions()
{
    _optionsForm->saveSettings();
    loadRules();
}

void MessageFilter::restoreOptions() { }

QString MessageFilter::pluginInfo()
{
    return tr("Can drop incoming stanzas according to various filters like source/destination address or specific "
              "message contents");
}

bool MessageFilter::incomingStanza(int account, const QDomElement &stanza)
{
    Q_UNUSED(account);

    if (!_enabled) {
        return false;
    }

    if (stanza.tagName() != "message") {
        return false;
    }

    QString message   = stanza.firstChildElement("body").text();
    QString from_full = stanza.attribute("from");
    QString from      = from_full.split("/").takeFirst();
    QString to_full   = stanza.attribute("to");
    QString to        = to_full.split("/").takeFirst();

    for (const Rule &rule : std::as_const(_rules)) {
        bool match = true;
        for (const Condition &condition : rule.conditions) {
            QString val;
            switch (condition.type) {
            case From:
                val = from;
                break;
            case To:
                val = to;
                break;
            case FromFull:
                val = from_full;
                break;
            case ToFull:
                val = to_full;
                break;
            case Message:
                val = message;
                break;
            }

            switch (condition.comparison) {
            case Equal:
                if (val != condition.text)
                    match = false;
                break;

            case NotEqual:
                if (val == condition.text)
                    match = false;
                break;

            case Contains:
                if (!val.contains(QRegularExpression(condition.text)))
                    match = false;
                break;

            case NotContains:
                if (val.contains(QRegularExpression(condition.text)))
                    match = false;
                break;
            }

            if (!match)
                break;
        }

        if (match)
            return !rule.showMessage;
    }

    return false;
}

void MessageFilter::loadRules()
{
    if (!_optionHost || !_enabled)
        return;

    _rules.clear();
    int rulesSize = _optionHost->getPluginOption("rules.size", 0).toInt();
    for (int i = 0; i < rulesSize; ++i) {
        QString optionName = QString("rules.l%1.").arg(i);
        Rule    rule;
        rule.name          = _optionHost->getPluginOption(optionName + "name").toString();
        rule.showMessage   = _optionHost->getPluginOption(optionName + "show-message").toBool();
        int conditionsSize = _optionHost->getPluginOption(optionName + "conditions.size").toInt();
        for (int j = 0; j < conditionsSize; ++j) {
            QString   optionName1 = QString("%1conditions.l%2.").arg(optionName).arg(j);
            Condition condition;
            condition.type = static_cast<ConditionType>(_optionHost->getPluginOption(optionName1 + "type").toInt());
            condition.comparison
                = static_cast<Comparison>(_optionHost->getPluginOption(optionName1 + "comparison").toInt());
            condition.text = _optionHost->getPluginOption(optionName1 + "text").toString();
            rule.conditions << condition;
        }
        _rules << rule;
    }
}
