/*
 * viewmaildlg.cpp - plugin
 * Copyright (C) 2011 Evgeny Khryukin
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

#include "viewmaildlg.h"
#include <QDesktopServices>
#include <QWheelEvent>

static const QString mailBoxUrl = "https://mail.google.com/mail/#inbox";

ViewMailDlg::ViewMailDlg(const QList<MailItem> &l, IconFactoryAccessingHost* host, QWidget *p)
    : QDialog(p, Qt::Window)
    , items_(l)
    , currentItem_(-1)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui_.setupUi(this);
    ui_.tb_next->setIcon(host->getIcon("psi/arrowRight"));
    ui_.tb_prev->setIcon(host->getIcon("psi/arrowLeft"));
    ui_.pb_close->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    ui_.pb_browse->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    connect(ui_.tb_next, SIGNAL(clicked()), SLOT(showNext()));
    connect(ui_.tb_prev, SIGNAL(clicked()), SLOT(showPrev()));
    connect(ui_.pb_browse, SIGNAL(clicked()), SLOT(browse()));
    connect(ui_.te_text, SIGNAL(anchorClicked(QUrl)), SLOT(anchorClicked(QUrl)));

    if(!items_.isEmpty()) {
        currentItem_ = 0;
        showItem(currentItem_);
    }

    updateCaption();
}

void ViewMailDlg::updateCaption()
{
    setWindowTitle( caption() );
}

QString ViewMailDlg::caption() const
{
    return tr("[%1/%2] E-Mail")
            .arg(QString::number(currentItem_+1))
            .arg(items_.size());
}

void ViewMailDlg::appendItems(QList<MailItem> l)
{
    items_.append(l);
    if(currentItem_ == -1) {
        currentItem_ = 0;
        showItem(currentItem_);
    }
    updateButtons();
    updateCaption();
}

void ViewMailDlg::updateButtons()
{
    if(items_.isEmpty()) {
        ui_.tb_next->setEnabled(false);
        ui_.tb_prev->setEnabled(false);
    }
    else {
        ui_.tb_prev->setEnabled(currentItem_ != 0);
        ui_.tb_next->setEnabled(items_.size()-1 > currentItem_);
    }
}

void ViewMailDlg::showNext()
{
    if(ui_.tb_next->isEnabled())
        showItem(++currentItem_);
}

void ViewMailDlg::showPrev()
{
    if(ui_.tb_prev->isEnabled())
        showItem(--currentItem_);
}

void ViewMailDlg::showItem(int num)
{
    ui_.le_account->clear();
    ui_.le_from->clear();
    ui_.le_subject->clear();
    ui_.te_text->clear();

    if(num != -1
       && !items_.isEmpty()
       && num < items_.size() )
    {
        MailItem me = items_.at(num);
        ui_.le_account->setText(me.account);
        ui_.le_account->setCursorPosition(0);
        ui_.le_from->setText(me.from);
        ui_.le_from->setCursorPosition(0);
        ui_.le_subject->setText(me.subject);
        ui_.le_subject->setCursorPosition(0);
        //FIXMI gmail отдает какой-то непонятный урл
        QRegExp re("th=([0-9]+)&");
        QString text = me.text;
        if(me.url.contains(re)) {
            QString url = mailBoxUrl + "/";
            QString tmp = re.cap(1);
            url += QString::number(tmp.toLongLong(), 16);
            text += QString("<br><br><a href=\"%1\">%2</a>").arg(url, tr("Open in browser"));
        }
        ui_.te_text->setHtml(text);
    }
    updateButtons();
    updateCaption();
}

void ViewMailDlg::browse()
{
    QDesktopServices::openUrl(QUrl(mailBoxUrl));
}

void ViewMailDlg::anchorClicked(const QUrl &url)
{
    if(!url.isEmpty()) {
        QDesktopServices::openUrl(url);
        if(items_.count() < 2) {
            close();
        }
    }
}

void ViewMailDlg::wheelEvent(QWheelEvent *e)
{
    if(e->delta() < 0) {
        showNext();
    }
    else {
        showPrev();
    }
    e->accept();
}

QString ViewMailDlg::mailItemToText(const MailItem &mi)
{
    QStringList lst = QStringList() << mi.from
              << mi.subject << mi.text;
    QString text = lst.join("\n") + "\n";

    return text;
}
