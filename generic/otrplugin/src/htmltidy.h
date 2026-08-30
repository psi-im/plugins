/*
 * htmltidy.h - Normalize rich text with Qt
 */

#ifndef HTMLTIDY_H_
#define HTMLTIDY_H_

#include <QString>

class QDomDocument;
class QDomElement;

class HtmlTidy {
public:
    explicit HtmlTidy(QString html);

    QString output() const;
    QDomElement output(QDomDocument &document) const;

private:
    QString m_input;
};

#endif
