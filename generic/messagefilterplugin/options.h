/*
 * options.h - plugin widget
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QList>
#include <QMetaType>
#include <QWidget>

class OptionAccessingHost;

namespace Ui {
class Options;
}

enum ConditionType { From, To, FromFull, ToFull, Message };
enum Comparison { Equal, NotEqual, Contains, NotContains };

struct Condition {
    ConditionType type;
    Comparison    comparison;
    QString       text;
};

struct Rule {
    QString          name;
    bool             showMessage;
    QList<Condition> conditions;
};

class Options : public QWidget {
    Q_OBJECT

public:
    explicit Options(const QList<Rule> &rules, QWidget *parent = nullptr);
    ~Options();

    void update();
    void setOptionAccessingHost(OptionAccessingHost *host) { _optionHost = host; }

    void saveSettings();

public slots:
    void addRule();
    void removeRule();
    void upRule();
    void downRule();
    void updateRuleButtons();

    void addCondition();
    void removeCondition();
    void upCondition();
    void downCondition();
    void updateConditionButtons();

    void updateRuleName(const QString &name);
    void setRulePane(int row);

    void hack();

private:
    void clearConditionsTable();
    void fillCondition(int row);
    void saveCondition(int rule, int row);

    Ui::Options *        ui;
    OptionAccessingHost *_optionHost;
    QList<Rule>          _rules;
    int                  _currentRule;
};

#endif // OPTIONS_H
