/*
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

#pragma once

#include <QLineEdit>
#include <QList>
#include <QToolButton>

class QFrame;
class QHBoxLayout;

class LineEditWidget : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(int optimalLength READ optimalLenth WRITE setOptimalLength)
    Q_PROPERTY(QString rxValidator READ rxValidator WRITE setRxValidator)
public:
    explicit LineEditWidget(QWidget *parent = nullptr);
    ~LineEditWidget();

    // reimplemented
    QSize sizeHint() const;
    void  showEvent(QShowEvent *e);
    bool  eventFilter(QObject *o, QEvent *e);

    // Properties
    int  optimalLenth() const { return m_optimalLength; }
    void setOptimalLength(int optimalLength) { m_optimalLength = optimalLength; }

    QString rxValidator() const { return m_rxValidator; }
    void    setRxValidator(const QString &str);

protected:
    void    addWidget(QWidget *w);
    void    setPopup(QWidget *w);
    QFrame *popup() const { return m_popup; };

protected slots:
    virtual void showPopup();
    virtual void hidePopup();

private:
    QHBoxLayout     *m_layout;
    QList<QWidget *> m_toolbuttons;
    QFrame          *m_popup;

    // Properties
    int     m_optimalLength;
    QString m_rxValidator;
};
