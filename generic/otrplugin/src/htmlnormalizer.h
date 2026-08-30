#pragma once

#include <QString>

class QDomDocument;
class QDomElement;

class HtmlNormalizer
{
public:
    explicit HtmlNormalizer(QString html);

    QString output() const;
    QDomElement output(QDomDocument &document) const;

private:
    QString input_;
};
