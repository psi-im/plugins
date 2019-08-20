/*
 * optionsparser.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef OPTIONSPARSER_H
#define OPTIONSPARSER_H

#include <QDomElement>
#include <QVariant>
#include <QByteArray>

class OptionsParser : public QObject {
    Q_OBJECT

    public:
        OptionsParser(QObject* parent = nullptr);
        static OptionsParser* instance();
        QVariant elementToVariant(const QDomElement& e);
        void variantToElement(const QVariant& var, QDomElement& e);

    private:
       static OptionsParser *instance_;

};

//stolen from iris
class Base64 {
        public:
            static QString encode(const QByteArray&);
            static QByteArray decode(const QString &s);
};

#endif // OPTIONSPARSER_H
