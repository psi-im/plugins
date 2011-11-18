#include "HtmlTidy.hpp"
#include <string>

//-----------------------------------------------------------------------------

HtmlTidy::HtmlTidy(const QString& html)
    : m_tidyDoc(tidyCreate()),
      m_errorOutput(),
      m_output(),
      m_input(html)
{
    tidyOptSetBool(m_tidyDoc, TidyXmlOut, yes);
    tidyOptSetValue(m_tidyDoc, TidyCharEncoding, "utf8");
    tidyOptSetInt(m_tidyDoc, TidyNewline, TidyLF);
    tidyOptSetBool(m_tidyDoc, TidyQuoteNbsp, no);
    tidyOptSetBool(m_tidyDoc, TidyForceOutput, yes);

    tidySetErrorBuffer(m_tidyDoc, &m_errorOutput);  
            
    tidyParseString(m_tidyDoc, m_input.toUtf8().data());
    tidyCleanAndRepair(m_tidyDoc);
}

//-----------------------------------------------------------------------------

HtmlTidy::~HtmlTidy()
{
    tidyRelease(m_tidyDoc);
    tidyBufFree(&m_errorOutput);
}

//-----------------------------------------------------------------------------

QString HtmlTidy::writeOutput()
{
    m_output.clear();
    TidyOutputSink sink;
#ifdef Q_WS_WIN
    void (*tmpF)(void*, byte) = putByte;
    sink.putByte = (void (TIDY_CALL *)(void*, byte))tmpF;
#else
    sink.putByte = putByte;
#endif
    sink.sinkData = this;
    tidySaveSink(m_tidyDoc, &sink);
    
    return QString::fromUtf8(m_output);
}

//-----------------------------------------------------------------------------

QString HtmlTidy::output()
{
    QDomDocument document;
    QDomElement body = output(document);
    QString s;
    QTextStream ts(&s) ;
    body.save(ts, 0);
    return s;
}

//-----------------------------------------------------------------------------

QDomElement HtmlTidy::output(QDomDocument& document)
{
    int errorLine = 0;
    int errorColumn = 0;
    QString errorText;
        
    QString html = writeOutput();
    if (!document.setContent(html, true, &errorText,
                            &errorLine, &errorColumn))
    {
        qWarning() << "---- parsing error:\n" << html << "\n----\n"
                   << errorText << " line:" << errorLine << " column:" << errorColumn;

        QDomElement domBody = document.createElement("body");
        domBody.appendChild(document.createTextNode(m_input));
        return domBody;
    }

    return document.documentElement().firstChildElement("body");
}

//-----------------------------------------------------------------------------

void HtmlTidy::putByte(void* sinkData, byte bt)
{
    static_cast<HtmlTidy*>(sinkData)->putByte(bt);
}

//-----------------------------------------------------------------------------
    
void HtmlTidy::putByte(byte bt)
{
    m_output.append(bt);
}

//-----------------------------------------------------------------------------
