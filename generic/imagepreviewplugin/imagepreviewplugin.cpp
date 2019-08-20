/*
 * imagepreviewplugin.cpp - plugin
 * Copyright (C) 2016 rkfg
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

#include "ScrollKeeper.h"
#include "applicationinfoaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "chattabaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiplugin.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDomElement>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSpinBox>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QVBoxLayout>
#include <stdexcept>

#undef AUTOREDIRECTS
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#define AUTOREDIRECTS
#endif

//#define IMGPREVIEW_DEBUG

#define constVersion "0.1.2"
#define sizeLimitName "imgpreview-size-limit"
#define previewSizeName "imgpreview-preview-size"
#define allowUpscaleName "imgpreview-allow-upscale"

#define MAX_REDIRECTS 2

class Origin: public QObject {
    Q_OBJECT
public:
    Origin(QWidget* chat) :
    QObject(chat), originalUrl_(""), chat_(chat)
#ifndef AUTOREDIRECTS
    , redirectsLeft_(0)
#endif
    {}
    QString originalUrl_;
    QWidget* chat_;
#ifndef AUTOREDIRECTS
    int redirectsLeft_;
#endif
};

class ImagePreviewPlugin:
        public ApplicationInfoAccessor,
        public ChatTabAccessor,
        public OptionAccessor,
        public PluginInfoProvider,
        public PsiPlugin,
        public QObject {
Q_OBJECT
Q_PLUGIN_METADATA(IID "com.psi-plus.ImagePreviewPlugin")
Q_INTERFACES(PsiPlugin PluginInfoProvider OptionAccessor ChatTabAccessor ApplicationInfoAccessor)
public:
    ImagePreviewPlugin();
    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget* options();
    virtual bool enable();
    virtual bool disable();

    virtual void applyOptions();
    virtual void restoreOptions();
    virtual void setOptionAccessingHost(OptionAccessingHost *host);
    virtual void optionChanged(const QString &) {
    }
    virtual QString pluginInfo();
    virtual QPixmap icon() const;
    virtual void setupChatTab(QWidget* widget, int, const QString&) {
        connect(widget, SIGNAL(messageAppended(const QString &, QWidget*)),
                SLOT(messageAppended(const QString &, QWidget*)));
    }
    virtual void setupGCTab(QWidget* widget, int, const QString&) {
        connect(widget, SIGNAL(messageAppended(const QString &, QWidget*)),
                SLOT(messageAppended(const QString &, QWidget*)));
    }

    virtual bool appendingChatMessage(int, const QString&, QString&, QDomElement&, bool) {
        return false;
    }
    virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
    void updateProxy();
    ~ImagePreviewPlugin() {
        manager->deleteLater();
    }
private slots:
    void messageAppended(const QString &, QWidget*);
    void imageReply(QNetworkReply* reply);
private:
    OptionAccessingHost *psiOptions;
    bool enabled;
    QNetworkAccessManager* manager;
    QSet<QString> pending, failed;
    int previewSize;
    QPointer<QSpinBox> sb_previewSize;
    int sizeLimit;
    QPointer<QComboBox> cb_sizeLimit;
    bool allowUpscale;
    QPointer<QCheckBox> cb_allowUpscale;
    ApplicationInfoAccessingHost* appInfoHost;
    void queueUrl(const QString& url, Origin* origin);
};

ImagePreviewPlugin::ImagePreviewPlugin() :
        psiOptions(nullptr), enabled(false), manager(new QNetworkAccessManager(this)), previewSize(0), sizeLimit(0), allowUpscale(
                false), appInfoHost(nullptr) {
    connect(manager, SIGNAL(finished(QNetworkReply *)), SLOT(imageReply(QNetworkReply *)));
}

QString ImagePreviewPlugin::name() const {
    return "Image Preview Plugin";
}

QString ImagePreviewPlugin::shortName() const {
    return "imgpreview";
}
QString ImagePreviewPlugin::version() const {
    return constVersion;
}

bool ImagePreviewPlugin::enable() {
    enabled = true;
    sizeLimit = psiOptions->getPluginOption(sizeLimitName, 1024 * 1024).toInt();
    previewSize = psiOptions->getPluginOption(previewSizeName, 150).toInt();
    allowUpscale = psiOptions->getPluginOption(allowUpscaleName, true).toBool();
    updateProxy();
    return enabled;
}

bool ImagePreviewPlugin::disable() {
    enabled = false;
    return true;
}

QWidget* ImagePreviewPlugin::options() {
    if (!enabled) {
        return nullptr;
    }
    QWidget *optionsWid = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(optionsWid);
    cb_sizeLimit = new QComboBox(optionsWid);
    cb_sizeLimit->addItem(tr("512 Kb"), 512 * 1024);
    cb_sizeLimit->addItem(tr("1 Mb"), 1024 * 1024);
    cb_sizeLimit->addItem(tr("2 Mb"), 2 * 1024 * 1024);
    cb_sizeLimit->addItem(tr("5 Mb"), 5 * 1024 * 1024);
    cb_sizeLimit->addItem(tr("10 Mb"), 10 * 1024 * 1024);
    vbox->addWidget(new QLabel(tr("Maximum image size")));
    vbox->addWidget(cb_sizeLimit);
    sb_previewSize = new QSpinBox(optionsWid);
    sb_previewSize->setMinimum(1);
    sb_previewSize->setMaximum(65535);
    vbox->addWidget(new QLabel(tr("Image preview size in pixels")));
    vbox->addWidget(sb_previewSize);
    cb_allowUpscale = new QCheckBox(tr("Allow upscale"));
    vbox->addWidget(cb_allowUpscale);
    vbox->addStretch();
    updateProxy();
    return optionsWid;
}

void ImagePreviewPlugin::setOptionAccessingHost(OptionAccessingHost *host) {
    psiOptions = host;
}

QString ImagePreviewPlugin::pluginInfo() {
    return tr("Author: ") + "rkfg\n\n" + tr("This plugin shows images URLs' previews in chats for non-webkit Psi version.\n");
}

QPixmap ImagePreviewPlugin::icon() const {
    return QPixmap(":/imagepreviewplugin/imagepreviewplugin.png");
}

void ImagePreviewPlugin::queueUrl(const QString& url, Origin* origin) {
    if (!pending.contains(url) && !failed.contains(url)) {
        pending.insert(url);
        QNetworkRequest req;
        origin->originalUrl_ = url;
#ifndef AUTOREDIRECTS
        origin->redirectsLeft_ = MAX_REDIRECTS;
#endif
        req.setUrl(QUrl::fromUserInput(url));
        req.setOriginatingObject(origin);
        req.setRawHeader("User-Agent",
                "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");
#ifdef AUTOREDIRECTS
        req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        req.setMaximumRedirectsAllowed(MAX_REDIRECTS);
#endif
        manager->head(req);
    }
}

void ImagePreviewPlugin::messageAppended(const QString &, QWidget* logWidget) {
    if (!enabled) {
        return;
    }
    ScrollKeeper sk(logWidget);
    QTextEdit* te_log = qobject_cast<QTextEdit*>(logWidget);
    if (te_log) {
        QTextCursor cur = te_log->textCursor();
        te_log->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        te_log->moveCursor(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        QTextCursor found = te_log->textCursor();
        while (!(found = te_log->document()->find(QRegExp("https?://\\S*"), found)).isNull()) {
            QString url = found.selectedText();
#ifdef IMGPREVIEW_DEBUG
            qDebug() << "URL FOUND:" << url;
#endif
            queueUrl(url, new Origin(te_log));
        };
        te_log->setTextCursor(cur);
    }
}

void ImagePreviewPlugin::imageReply(QNetworkReply* reply) {
    bool ok;
    int size = 0;
    QStringList contentTypes;
    QStringList allowedTypes;
    allowedTypes.append("image/jpeg");
    allowedTypes.append("image/png");
    allowedTypes.append("image/gif");
    Origin* origin = qobject_cast<Origin*>(reply->request().originatingObject());
    QString urlStr = origin->originalUrl_;
    QString urlStrEscaped = reply->url().toString();
#ifdef IMGPREVIEW_DEBUG
    qDebug() << "Original URL " << urlStr << " / Escaped: " << urlStrEscaped;
#endif
    switch (reply->operation()) {
    case QNetworkAccessManager::HeadOperation: {
#ifndef AUTOREDIRECTS
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(&ok);
        if (ok && (status == 301 || status == 302 || status == 303)) {
            if (origin->redirectsLeft_ == 0) {
                qWarning() << "Redirect count exceeded for" << urlStr;
                origin->deleteLater();
                pending.remove(urlStr);
                break;
            }
            origin->redirectsLeft_--;
            QUrl location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (location.isRelative()) {
                location = location.resolved(reply->request().url());
            }
#ifdef IMGPREVIEW_DEBUG
            qDebug() << "Redirected to" << location;
#endif
            QNetworkRequest req = reply->request();
            req.setUrl(location);
            manager->head(req);
        } else
#endif
        {
            size = reply->header(QNetworkRequest::ContentLengthHeader).toInt(&ok);
            if (reply->error() == QNetworkReply::NoError) {
                ok = true;
            }
            contentTypes = reply->header(QNetworkRequest::ContentTypeHeader).toString().split(",");
            QString lastType = contentTypes.last().trimmed();
#ifdef IMGPREVIEW_DEBUG
            qDebug() << "URL:" << urlStr << "RESULT:" << reply->error() << "SIZE:" << size << "Content-type:"
                    << contentTypes << " Last type: " << lastType;
#endif
            if (ok && allowedTypes.contains(lastType, Qt::CaseInsensitive) && size < sizeLimit) {
                manager->get(reply->request());
            } else {
#ifdef IMGPREVIEW_DEBUG
                qDebug() << "Failed url " << origin->originalUrl_;
#endif
                failed.insert(origin->originalUrl_);
                origin->deleteLater();
                pending.remove(urlStr);
            }
        }
        break;
    }
    case QNetworkAccessManager::GetOperation:
        try {
            QImageReader imageReader(reply);
            QImage image = imageReader.read();
            if (imageReader.error() != QImageReader::UnknownError) {
                throw std::runtime_error(imageReader.errorString().toStdString());
            }
            if (image.width() > previewSize || image.height() > previewSize || allowUpscale) {
                image = image.scaled(previewSize, previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
#ifdef IMGPREVIEW_DEBUG
            qDebug() << "Image size:" << image.size();
#endif
            ScrollKeeper sk(origin->chat_);
            QTextEdit* te_log = qobject_cast<QTextEdit*>(origin->chat_);
            if (te_log) {
                te_log->document()->addResource(QTextDocument::ImageResource, urlStrEscaped, image);
                QTextCursor saved = te_log->textCursor();
                te_log->moveCursor(QTextCursor::End);
                QRegExp rawUrlRE = QRegExp("(<a href=\"[^\"]*\">)(.*)(</a>)");
                while (te_log->find(urlStr, QTextDocument::FindBackward)) {
                    QTextCursor cur = te_log->textCursor();
                    QString html = cur.selection().toHtml();
                    if (html.contains(rawUrlRE)) {
                        html.replace(rawUrlRE, QString("\\1<img src='%1'/>\\3").arg(urlStrEscaped));
                        cur.insertHtml(html);
                    }
                }
                te_log->setTextCursor(saved);
            }
        } catch (std::exception& e) {
            failed.insert(origin->originalUrl_);
            qWarning() << "ERROR: " << e.what();
        }
        origin->deleteLater();
        pending.remove(urlStr);
        break;
    default:
        break;
    }
    reply->deleteLater();
}

void ImagePreviewPlugin::applyOptions() {
    psiOptions->setPluginOption(previewSizeName, previewSize = sb_previewSize->value());
    psiOptions->setPluginOption(sizeLimitName, sizeLimit =
            cb_sizeLimit->itemData(cb_sizeLimit->currentIndex()).toInt());
    psiOptions->setPluginOption(allowUpscaleName, allowUpscale = cb_allowUpscale->checkState() == Qt::Checked);
}

void ImagePreviewPlugin::restoreOptions() {
    sb_previewSize->setValue(previewSize);
    cb_sizeLimit->setCurrentIndex(cb_sizeLimit->findData(sizeLimit));
    cb_allowUpscale->setCheckState(allowUpscale ? Qt::Checked : Qt::Unchecked);
}

void ImagePreviewPlugin::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host) {
    appInfoHost = host;
}

void ImagePreviewPlugin::updateProxy() {
    Proxy proxy = appInfoHost->getProxyFor(name());
#ifdef IMGPREVIEW_DEBUG
    qDebug() << "Proxy:" << "T:" << proxy.type << "H:" << proxy.host << "Pt:" << proxy.port << "U:" << proxy.user
            << "Ps:" << proxy.pass;
#endif
    if (proxy.type.isEmpty()) {
        manager->setProxy(QNetworkProxy());
        return;
    }
    QNetworkProxy netProxy(proxy.type == "socks" ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy, proxy.host,
            proxy.port, proxy.user, proxy.pass);
    manager->setProxy(netProxy);
}

#include "imagepreviewplugin.moc"
