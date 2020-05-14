/*
 * imageplugin.cpp - plugin
 * Copyright (C) 2009-2010  VampiRus
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

#include "accountinfoaccessinghost.h"
#include "accountinfoaccessor.h"
#include "activetabaccessinghost.h"
#include "activetabaccessor.h"
#include "gctoolbariconaccessor.h"
#include "iconfactoryaccessinghost.h"
#include "iconfactoryaccessor.h"
#include "optionaccessinghost.h"
#include "optionaccessor.h"
#include "plugininfoprovider.h"
#include "psiaccountcontroller.h"
#include "psiaccountcontrollinghost.h"
#include "psiplugin.h"
#include "stanzasender.h"
#include "stanzasendinghost.h"
#include "toolbariconaccessor.h"
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

#define constVersion "0.1.2"

#define CONST_LAST_FOLDER "lastfolder"

const int MAX_SIZE = 400;

class ImagePlugin : public QObject,
                    public PsiPlugin,
                    public ToolbarIconAccessor,
                    public GCToolbarIconAccessor,
                    public StanzaSender,
                    public IconFactoryAccessor,
                    public ActiveTabAccessor,
                    public PluginInfoProvider,
                    public AccountInfoAccessor,
                    public PsiAccountController,
                    public OptionAccessor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.psi-plus.ImagePlugin" FILE "psiplugin.json")
    Q_INTERFACES(PsiPlugin ToolbarIconAccessor GCToolbarIconAccessor StanzaSender ActiveTabAccessor PsiAccountController
                     OptionAccessor IconFactoryAccessor AccountInfoAccessor PluginInfoProvider)
public:
    ImagePlugin();
    virtual QString  name() const;
    virtual QString  shortName() const;
    virtual QString  version() const;
    virtual QWidget *options();
    virtual bool     enable();
    virtual bool     disable();

    virtual void                applyOptions() { }
    virtual void                restoreOptions() { }
    virtual QList<QVariantHash> getButtonParam();
    virtual QAction *           getAction(QObject *, int, const QString &) { return nullptr; }
    virtual QList<QVariantHash> getGCButtonParam();
    virtual QAction *           getGCAction(QObject *, int, const QString &) { return nullptr; }
    virtual void                setIconFactoryAccessingHost(IconFactoryAccessingHost *host);
    virtual void                setStanzaSendingHost(StanzaSendingHost *host);
    virtual void                setActiveTabAccessingHost(ActiveTabAccessingHost *host);
    virtual void                setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    virtual void                setPsiAccountControllingHost(PsiAccountControllingHost *host);
    virtual void                setOptionAccessingHost(OptionAccessingHost *host);
    virtual void                optionChanged(const QString &) { }
    virtual QString             pluginInfo();
    virtual QPixmap             icon() const;

private slots:
    void actionActivated();

private:
    IconFactoryAccessingHost * iconHost;
    StanzaSendingHost *        stanzaSender;
    ActiveTabAccessingHost *   activeTab;
    AccountInfoAccessingHost * accInfo;
    PsiAccountControllingHost *psiController;
    OptionAccessingHost *      psiOptions;
    bool                       enabled;
    QHash<QString, int>        accounts_;
};

ImagePlugin::ImagePlugin() :
    iconHost(nullptr), stanzaSender(nullptr), activeTab(nullptr), accInfo(nullptr), psiController(nullptr),
    psiOptions(nullptr), enabled(false)
{
}

QString ImagePlugin::name() const { return "Image Plugin"; }

QString ImagePlugin::shortName() const { return "image"; }
QString ImagePlugin::version() const { return constVersion; }

bool ImagePlugin::enable()
{
    QFile file(":/imageplugin/imageplugin.gif");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray image = file.readAll();
        iconHost->addIcon("imageplugin/icon", image);
        file.close();
        enabled = true;
    } else {
        enabled = false;
    }
    return enabled;
}

bool ImagePlugin::disable()
{
    enabled = false;
    return true;
}

QWidget *ImagePlugin::options()
{
    if (!enabled) {
        return nullptr;
    }
    QWidget *    optionsWid = new QWidget();
    QVBoxLayout *vbox       = new QVBoxLayout(optionsWid);
    QLabel *     wikiLink
        = new QLabel(tr("<a href=\"https://psi-plus.com/wiki/en:plugins#image_plugin\">Wiki (Online)</a>"), optionsWid);
    wikiLink->setOpenExternalLinks(true);
    vbox->addWidget(wikiLink);
    vbox->addStretch();
    return optionsWid;
}

QList<QVariantHash> ImagePlugin::getButtonParam()
{
    QVariantHash hash;
    hash["tooltip"] = QVariant(tr("Send Image"));
    hash["icon"]    = QVariant(QString("imageplugin/icon"));
    hash["reciver"] = QVariant::fromValue(qobject_cast<QObject *>(this));
    hash["slot"]    = QVariant(SLOT(actionActivated()));
    QList<QVariantHash> l;
    l.push_back(hash);
    return l;
}

QList<QVariantHash> ImagePlugin::getGCButtonParam() { return getButtonParam(); }

void ImagePlugin::setAccountInfoAccessingHost(AccountInfoAccessingHost *host) { accInfo = host; }

void ImagePlugin::setIconFactoryAccessingHost(IconFactoryAccessingHost *host) { iconHost = host; }

void ImagePlugin::setPsiAccountControllingHost(PsiAccountControllingHost *host) { psiController = host; }

void ImagePlugin::setOptionAccessingHost(OptionAccessingHost *host) { psiOptions = host; }

void ImagePlugin::setStanzaSendingHost(StanzaSendingHost *host) { stanzaSender = host; }

void ImagePlugin::setActiveTabAccessingHost(ActiveTabAccessingHost *host) { activeTab = host; }

void ImagePlugin::actionActivated()
{
    if (!enabled)
        return;

    QString fileName("");
    QString jid       = activeTab->getYourJid();
    QString jidToSend = activeTab->getJid();
    int     account   = 0;
    QString tmpJid("");
    while (jid != (tmpJid = accInfo->getJid(account))) {
        ++account;
        if (tmpJid == "-1")
            return;
    }

    QMenu            m;
    QList<QAction *> list;
    list << new QAction(tr("Open file"), &m) << new QAction(tr("From clipboard"), &m);
    QAction *act = m.exec(list, QCursor::pos());
    if (!act)
        return;

    if ("offline" == accInfo->getStatus(account)) {
        return;
    }

    QPixmap pix;
    QString imageName;
    bool    isClipboard = list.indexOf(act) == 1;
    if (isClipboard) {
        if (!QApplication::clipboard()->mimeData()->hasImage())
            return;

        pix       = QPixmap::fromImage(QApplication::clipboard()->image());
        imageName = QApplication::clipboard()->text();
    } else {
        const QString lastPath = psiOptions->getPluginOption(CONST_LAST_FOLDER, QDir::homePath()).toString();
        fileName               = QFileDialog::getOpenFileName(nullptr, tr("Open Image"), lastPath,
                                                tr("Images (*.png *.gif *.jpg *.jpeg *.ico)"));
        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            pix       = QPixmap::fromImage(QImage::fromData(file.readAll()));
            imageName = QFileInfo(file).fileName();

        } else {
            return;
        }
        psiOptions->setPluginOption(CONST_LAST_FOLDER, QFileInfo(file).path());
    }

    QByteArray image;
    QString    mimeType("jpeg");
    if (pix.height() > MAX_SIZE || pix.width() > MAX_SIZE) {
        pix = pix.scaled(MAX_SIZE, MAX_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QBuffer b(&image);
    pix.save(&b, mimeType.toLatin1().constData());
    QString imageBase64(image.toBase64());
    int     length = image.length();
    if (length > 61440) {
        QMessageBox::information(nullptr, tr("The image size is too large."), tr("Image size must be less than 60 kb"));
    }
    QString mType = QLatin1String(sender()->parent()->metaObject()->className()) == "PsiChatDlg" ? "chat" : "groupchat";
    QString body  = tr("Image %1 bytes received.").arg(QString::number(length));
    QString msgHtml = QString("<message type=\"%1\" to=\"%2\" id=\"%3\" >"
                              "<body>%4</body>"
                              "<html xmlns=\"http://jabber.org/protocol/xhtml-im\">"
                              "<body xmlns=\"http://www.w3.org/1999/xhtml\">"
                              "<br/><img src=\"data:image/%5;base64,%6\" alt=\"img\"/> "
                              "</body></html></message>")
                          .arg(mType)
                          .arg(jidToSend)
                          .arg(stanzaSender->uniqueId(account))
                          .arg(body)
                          .arg(mimeType)
                          .arg(imageBase64);

    stanzaSender->sendStanza(account, msgHtml);
    if (isClipboard) {
        psiController->appendSysMsg(account, jidToSend, tr("An image from the clipboard is sent"));
        // TODO for private chat this has to be added as a regular message, otherwise it won't be visible.
    } else {
        psiController->appendSysMsg(account, jidToSend, tr("Image %1 is sent").arg(imageName));
    }
}

QString ImagePlugin::pluginInfo()
{
    return name() + "\n\n" + tr("Authors: ") + "VampiRUS, Dealer_WeARE\n\n"
        + tr("This plugin is designed to send images to roster contacts.\n"
             "Your contact's client must be support XEP-0071: XHTML-IM and support the data:URI scheme.\n"
             "Note: To work correctly, the option options.ui.chat.central-toolbar  must be set to true.");
}

QPixmap ImagePlugin::icon() const { return QPixmap(":/imageplugin/imageplugin.gif"); }

#include "imageplugin.moc"
