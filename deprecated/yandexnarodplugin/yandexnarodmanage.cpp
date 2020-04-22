/*
    yandexnarodManage

    Copyright (c) 2009 by Alexander Kazarin <boiler@co.ru>
            2011 by Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>

#include "optionaccessinghost.h"
#include "options.h"
#include "ui_yandexnarodmanage.h"
#include "uploaddialog.h"
#include "yandexnarodmanage.h"
#include "yandexnarodsettings.h"

//------------------------------------------
//-------ListWidgetItem---------------------
//------------------------------------------
class ListWidgetItem : public QListWidgetItem {
public:
    ListWidgetItem(const QIcon &ico, const yandexnarodNetMan::FileItem &fileitem) :
        QListWidgetItem(ico, fileitem.filename), item_(fileitem)
    {
        QString toolTip = QObject::tr("Name: %1\nSize: %2\nDate prolongate: %3\nURL: %4\nPassword: %5")
                              .arg(fileitem.filename)
                              .arg(QString(fileitem.size).replace("&nbsp;", " "))
                              .arg(fileitem.date)
                              .arg(fileitem.fileurl)
                              .arg(fileitem.passset ? QObject::tr("Yes") : QObject::tr("No"));
        setToolTip(toolTip);
    }

    const yandexnarodNetMan::FileItem &fileItem() const { return item_; }

private:
    yandexnarodNetMan::FileItem item_;
};

//------------------------------------------
//-------ListWidget-------------------------
//------------------------------------------
ListWidget::ListWidget(QWidget *p) : QListWidget(p) { }

QStringList ListWidget::mimeTypes() const { return QStringList() << "text/plain"; }

QMimeData *ListWidget::mimeData(const QList<QListWidgetItem *> &items) const
{
    if (items.isEmpty())
        return 0;

    QMimeData *d = new QMimeData();
    QString    text;
    foreach (QListWidgetItem *i, items) {
        text += static_cast<ListWidgetItem *>(i)->fileItem().fileurl + "\n";
    }
    d->setText(text);

    return d;
}

void ListWidget::mousePressEvent(QMouseEvent *event)
{
    QListWidget::mousePressEvent(event);
    if (event->button() == Qt::RightButton) {
        QListWidgetItem *lwi = itemAt(event->pos());
        if (lwi) {
            ListWidgetItem *it = static_cast<ListWidgetItem *>(lwi);
            emit            menu(it->fileItem());
            event->accept();
        }
    }
}

static QStringList files(const QMimeData *md)
{
    QString     str = QFile::decodeName(QByteArray::fromPercentEncoding(md->data("text/uri-list")));
    QRegExp     re("file://(.+)");
    QStringList files;
    int         index = re.indexIn(str);
    while (index != -1) {
        files.append(re.cap(1).trimmed());
        index = re.indexIn(str, index + 1);
    }
    return files;
}

void ListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *md   = event->mimeData();
    QStringList      list = files(md);
    bool             ok   = false;
    if (list.size() == 1) {
        QFile file(list.takeFirst());
        if (file.exists()) {
            ok = true;
        }
    }
    if (ok) {
        event->acceptProposedAction();
    }
}

void ListWidget::dropEvent(QDropEvent *event)
{
    QStringList list = files(event->mimeData());
    if (list.size() == 1) {
        const QString path = list.takeFirst();
        QFile         file(path);
        if (file.exists())
            emit uploadFile(path);
    }

    event->setDropAction(Qt::IgnoreAction);
    event->accept();
}

//------------------------------------------
//-------yandexnarodManage------------------
//------------------------------------------
yandexnarodManage::yandexnarodManage(QWidget *p) : QDialog(p, Qt::Window), ui_(new Ui::yandexnarodManageClass)
{
    ui_->setupUi(this);
    setWindowTitle(tr("Yandex.Narod file manager"));
    setWindowIcon(QIcon(":/icons/yandexnarodplugin.png"));
    ui_->frameProgress->hide();
    ui_->frameFileActions->hide();
    ui_->listWidget->clear();

    ui_->btnOpenBrowser->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));

    newNetMan();

    QPixmap iconimage(":/icons/yandexnarod-icons-files.png");
    for (int i = 0; i < (iconimage.width() / 16); i++) {
        QIcon icon(iconimage.copy((i * 16), 0, 16, 16));
        fileicons.append(icon);
    }

    fileiconstyles["b-icon-music"]   = 0;
    fileiconstyles["b-icon-video"]   = 1;
    fileiconstyles["b-icon-arc"]     = 2;
    fileiconstyles["b-icon-doc"]     = 3;
    fileiconstyles["b-icon-soft"]    = 4;
    fileiconstyles["b-icon-unknown"] = 5;
    fileiconstyles["b-icon-picture"] = 14;

    Options *o = Options::instance();
    int      h = o->getOption(CONST_HEIGHT, 200).toInt();
    int      w = o->getOption(CONST_WIDTH, 300).toInt();

    resize(w, h);

    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui_->listWidget, SIGNAL(menu(yandexnarodNetMan::FileItem)), SLOT(doMenu(yandexnarodNetMan::FileItem)));
    connect(ui_->listWidget, SIGNAL(uploadFile(QString)), this, SLOT(uploadFile(QString)), Qt::QueuedConnection);
}

yandexnarodManage::~yandexnarodManage()
{
    Options *o = Options::instance();
    o->setOption(CONST_HEIGHT, height());
    o->setOption(CONST_WIDTH, width());

    delete ui_;
}

void yandexnarodManage::newNetMan()
{
    netman = new yandexnarodNetMan(this);
    connect(netman, SIGNAL(statusText(QString)), ui_->labelStatus, SLOT(setText(QString)));
    connect(netman, SIGNAL(newFileItem(yandexnarodNetMan::FileItem)), this,
            SLOT(newFileItem(yandexnarodNetMan::FileItem)));
    connect(netman, SIGNAL(finished()), this, SLOT(netmanFinished()));
}

void yandexnarodManage::newFileItem(yandexnarodNetMan::FileItem fileitem)
{
    int     iconnum      = 5;
    QString fileiconname = fileitem.fileicon.replace("-old", "");
    if (fileiconstyles.contains(fileiconname))
        iconnum = fileiconstyles[fileiconname];

    QListWidgetItem *listitem = new ListWidgetItem(fileicons[iconnum], fileitem);
    ui_->listWidget->addItem(listitem);
}

void yandexnarodManage::netmanPrepare()
{
    ui_->frameProgress->show();
    ui_->labelStatus->clear();
    ui_->frameFileActions->hide();
    ui_->btnReload->setEnabled(false);
}

void yandexnarodManage::netmanFinished() { ui_->btnReload->setEnabled(true); }

void yandexnarodManage::on_btnReload_clicked()
{
    ui_->listWidget->clear();
    netmanPrepare();
    netman->startGetFilelist();
}

void yandexnarodManage::on_btnDelete_clicked()
{
    QList<yandexnarodNetMan::FileItem> out;
    foreach (QListWidgetItem *i, ui_->listWidget->selectedItems()) {
        ListWidgetItem *             lwi = static_cast<ListWidgetItem *>(i);
        yandexnarodNetMan::FileItem &f   = const_cast<yandexnarodNetMan::FileItem &>(lwi->fileItem());
        if (!f.deleted) {
            out.append(f);
            f.deleted = true;
        }
    }
    if (out.isEmpty())
        return;

    int rez
        = QMessageBox::question(this, tr("Delete file(s)"), tr("Are you sure?"), QMessageBox::Ok | QMessageBox::Cancel);
    if (rez == QMessageBox::Cancel)
        return;

    foreach (QListWidgetItem *i, ui_->listWidget->selectedItems()) {
        i->setIcon(fileicons[15]);
    }
    netmanPrepare();
    netman->startDelFiles(out);
}

void yandexnarodManage::on_btnProlong_clicked()
{
    netmanPrepare();
    QList<yandexnarodNetMan::FileItem> out;
    foreach (QListWidgetItem *i, ui_->listWidget->selectedItems()) {
        ListWidgetItem *            lwi = static_cast<ListWidgetItem *>(i);
        yandexnarodNetMan::FileItem f   = lwi->fileItem();
        if (f.prolong() < 45) {
            out.append(f);
        }
    }
    netman->startProlongFiles(out);
}

void yandexnarodManage::on_btnClearCookies_clicked()
{
    netman->disconnect();
    netman->deleteLater();

    Options::instance()->saveCookies(QList<QNetworkCookie>());

    newNetMan();
    ui_->frameProgress->show();
    ui_->labelStatus->setText(O_M(MRemoveCookie));
}

void yandexnarodManage::on_btnOpenBrowser_clicked()
{
    QDesktopServices::openUrl(QUrl("http://narod.yandex.ru/disk/all/"));
}

void yandexnarodManage::on_listWidget_pressed(QModelIndex)
{
    if (ui_->frameFileActions->isHidden())
        ui_->frameFileActions->show();

    bool prolong = false;
    foreach (QListWidgetItem *i, ui_->listWidget->selectedItems()) {
        ListWidgetItem *lwi = static_cast<ListWidgetItem *>(i);
        if (lwi->fileItem().prolong() < 45) {
            prolong = true;
            break;
        }
    }
    ui_->btnProlong->setEnabled(prolong);
}

void yandexnarodManage::on_btnClipboard_clicked()
{
    QStringList text;
    foreach (QListWidgetItem *i, ui_->listWidget->selectedItems()) {
        text << static_cast<ListWidgetItem *>(i)->fileItem().fileurl;
    }
    copyToClipboard(text.join("\n"));
}

void yandexnarodManage::copyToClipboard(const QString &text)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void yandexnarodManage::on_btnUpload_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, O_M(MChooseFile),
                                                    Options::instance()->getOption(CONST_LAST_FOLDER).toString());

    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        Options::instance()->setOption(CONST_LAST_FOLDER, fi.dir().path());

        uploadFile(filePath);
    }
}

void yandexnarodManage::uploadFile(const QString &path)
{
    uploadDialog *uploadwidget = new uploadDialog(this);
    connect(uploadwidget, SIGNAL(canceled()), this, SLOT(netmanFinished()));
    connect(uploadwidget, SIGNAL(finished()), this, SLOT(netmanFinished()));
    uploadwidget->show();
    uploadwidget->start(path);
}

void yandexnarodManage::doMenu(const yandexnarodNetMan::FileItem &it)
{
    QMenu            m;
    QList<QAction *> actions;
    QAction *        act = new QAction(tr("Set password"), &m);
    act->setVisible(!it.passset);
    act->setData(1);
    actions << act;
    act = new QAction(tr("Remove password"), &m);
    act->setVisible(it.passset);
    act->setData(2);
    actions << act;
    act = new QAction(tr("Copy URL"), &m);
    act->setData(3);
    actions << act;
    act = new QAction(tr("Prolongate"), &m);
    act->setData(4);
    act->setEnabled(it.prolong() < 45);
    actions << act;
    m.addActions(actions);
    QAction *ret = m.exec(QCursor::pos());
    if (ret) {
        switch (ret->data().toInt()) {
        case 1:
            netman->startSetPass(it);
            break;
        case 2:
            netman->startRemovePass(it);
            break;
        case 3:
            copyToClipboard(it.fileurl);
            break;
        case 4:
            netman->startProlongFiles(QList<yandexnarodNetMan::FileItem>() << it);
        default:
            break;
        }
    }
}
