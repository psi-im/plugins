/*
 * cleaner.cpp - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cleaner.h"
#include "common.h"
#include <QDomDocument>
#include <QInputDialog>
#include <QMessageBox>


CleanerMainWindow::CleanerMainWindow(CleanerPlugin *cleaner)
        : QMainWindow(0)
        , cleaner_(cleaner)
{
    setAttribute(Qt::WA_DeleteOnClose);
    vCardDir_ = cleaner_->appInfo->appVCardDir();
    historyDir_ = cleaner_->appInfo->appHistoryDir();
    cacheDir_ = cleaner_->appInfo->appHomeDir(ApplicationInfoAccessingHost::CacheLocation);
    profilesConfigDir_ = cleaner_->appInfo->appProfilesDir(ApplicationInfoAccessingHost::ConfigLocation);
    profilesDataDir_ = cleaner_->appInfo->appProfilesDir(ApplicationInfoAccessingHost::DataLocation);
    profilesCacheDir_ = cleaner_->appInfo->appProfilesDir(ApplicationInfoAccessingHost::CacheLocation);

    ui_.setupUi(this);

    setWindowIcon(cleaner_->iconHost->getIcon("psi/psiplus_logo"));
    ui_.pb_close->setIcon(cleaner_->iconHost->getIcon("psi/quit"));
    ui_.pb_delete->setIcon(cleaner_->iconHost->getIcon("psi/remove"));
    ui_.tw_tab->setTabIcon(0, cleaner_->iconHost->getIcon("psi/history"));
    ui_.tw_tab->setTabIcon(1, cleaner_->iconHost->getIcon("psi/vCard"));
    ui_.tw_tab->setTabIcon(2, cleaner_->iconHost->getIcon("psi/default_avatar"));
    ui_.tw_tab->setTabIcon(3, cleaner_->iconHost->getIcon("psi/options"));
    ui_.pb_selectAll->setIcon(cleaner_->iconHost->getIcon("psi/ok"));
    ui_.pb_unselectAll->setIcon(cleaner_->iconHost->getIcon("psi/cancel"));

    createMainMenu();
    createStatusBar();
}

void CleanerMainWindow::showCleaner()
{
    setContent();
    show();
}

void CleanerMainWindow::createMainMenu()
{
    QMenuBar *mBar = ui_.menuBar;

    QAction *chooseProf = new QAction(cleaner_->iconHost->getIcon("psi/account"), tr("Choose &Profile"), mBar);
    QAction *quit = new QAction(cleaner_->iconHost->getIcon("psi/quit"), tr("&Quit"), mBar);
    QAction *rmJuick = new QAction(cleaner_->iconHost->getIcon("clients/juick"), tr("Clear &Juick Cache"), mBar);
    QAction *rmBirthday = new QAction(cleaner_->iconHost->getIcon("reminder/birthdayicon"), tr("Clear &Birthdays Cache"), mBar);

    QMenu *file = mBar->addMenu(tr("&File"));
    file->addAction(chooseProf);
    file->addSeparator();
    file->addAction(quit);

    QMenu *act = mBar->addMenu(tr("&Actions"));
    act->addAction(rmJuick);
    act->addAction(rmBirthday);

    connect(chooseProf, SIGNAL(triggered()), this, SLOT(chooseProfileAct()));
    connect(quit, SIGNAL(triggered()), this, SLOT(close()));
    connect(rmJuick, SIGNAL(triggered()), this, SLOT(clearJuick()));
    connect(rmBirthday, SIGNAL(triggered()), this, SLOT(clearBirhday()));
}

void CleanerMainWindow::createStatusBar()
{
    QStatusBar *sBar = ui_.statusBar;
    sb1 = new QLabel(sBar);
    sb2 = new QLabel(sBar);
    sb3 = new QLabel(sBar);
    sBar->addWidget(sb1, 1);
    sBar->addWidget(sb2, 1);
    sBar->addWidget(sb3, 1);
}

void CleanerMainWindow::updateStatusBar()
{
    sb1->setText(tr("History files: ") + QString::number(historyModel_->rowCount()));
    sb2->setText(tr("vCards: ") + QString::number(vcardsModel_->rowCount()));
    sb3->setText(tr("Avatars: ") + QString::number(avatarModel_->rowCount()));
}

void CleanerMainWindow::setContent()
{
    historyModel_ = new ClearingHistoryModel(historyDir_, this);
    proxyHistoryModel_ = new ClearingProxyModel(this);
    proxyHistoryModel_->setSourceModel(historyModel_);
    ui_.tab_history->tv_table->setModel(proxyHistoryModel_);
    ui_.tab_history->tv_table->init(cleaner_->iconHost);

    vcardsModel_ = new ClearingVcardModel(vCardDir_, this);
    proxyVcardsModel_ = new ClearingProxyModel(this);
    proxyVcardsModel_->setSourceModel(vcardsModel_);
    ui_.tab_vcard->tv_table->setModel(proxyVcardsModel_);
    ui_.tab_vcard->tv_table->init(cleaner_->iconHost);

    QStringList avatars;
    avatars.append(avatarsDir());
    avatars.append(picturesDir());

    avatarModel_ = new ClearingAvatarModel(avatars, this);
    proxyAvatarModel_ = new QSortFilterProxyModel(this);
    proxyAvatarModel_->setSourceModel(avatarModel_);
    ui_.tab_avatars->tv_table->verticalHeader()->setDefaultSectionSize(120);
    ui_.tab_avatars->tv_table->setItemDelegateForColumn(1, new AvatarDelegate(this));
    ui_.tab_avatars->tv_table->setModel(proxyAvatarModel_);
    ui_.tab_avatars->tv_table->init(cleaner_->iconHost);

    QString optionsFile = profilesConfigDir_ + "/" + currentProfileName() + "/options.xml";
    optionsModel_ = new ClearingOptionsModel(optionsFile, this);
    proxyOptionsModel_ = new QSortFilterProxyModel(this);
    proxyOptionsModel_->setSourceModel(optionsModel_);
    ui_.tab_options->tv_table->setModel(proxyOptionsModel_);
    ui_.tab_options->tv_table->init(cleaner_->iconHost);


    connect(ui_.tab_history->tv_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(viewHistory(QModelIndex)));
    connect(ui_.tab_vcard->tv_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(viewVcard(QModelIndex)));
    connect(ui_.tab_avatars->tv_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(viewAvatar(QModelIndex)));
    connect(ui_.le_filter, SIGNAL(textChanged(QString)), this, SLOT(filterEvent()));
    connect(ui_.pb_delete, SIGNAL(released()), this, SLOT(deleteButtonPressed()));
    connect(ui_.tw_tab, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    connect(historyModel_, SIGNAL(updateLabel(int)), this, SLOT(currentTabChanged(int)));
    connect(vcardsModel_, SIGNAL(updateLabel(int)), this, SLOT(currentTabChanged(int)));
    connect(avatarModel_, SIGNAL(updateLabel(int)), this, SLOT(currentTabChanged(int)));
    connect(optionsModel_, SIGNAL(updateLabel(int)), this, SLOT(currentTabChanged(int)));
    connect(ui_.pb_selectAll, SIGNAL(released()), this, SLOT(selectAll()));
    connect(ui_.pb_unselectAll, SIGNAL(released()), this, SLOT(unselectAll()));
    connect(ui_.pb_close, SIGNAL(released()), this, SLOT(close()));

    ui_.le_filter->installEventFilter(this);

    ui_.tw_tab->setCurrentIndex(0);

    updateStatusBar();
}

static QModelIndexList visibleIndexes(const QSortFilterProxyModel *const model)
{
    int count = model->rowCount();
    QModelIndexList l;
    for(int i = 0; i < count; i++) {
        QModelIndex index = model->index(i, 0);
        index = model->mapToSource(index);
        l.append(index);
    }

    return l;
}

void CleanerMainWindow::selectAll()
{
    int tab = ui_.tw_tab->currentIndex();
    switch(tab) {
        case(0):
        {
            QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel*>(ui_.tab_history->tv_table->model());
            historyModel_->selectAll(visibleIndexes(model));
            break;
        }
        case(1):
        {
            QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel*>(ui_.tab_vcard->tv_table->model());
            vcardsModel_->selectAll(visibleIndexes(model));
            break;
        }
        case(2):
        {
            QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel*>(ui_.tab_avatars->tv_table->model());
            avatarModel_->selectAll(visibleIndexes(model));
            break;
        }
        case(3):
        {
            QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel*>(ui_.tab_options->tv_table->model());
            optionsModel_->selectAll(visibleIndexes(model));
            break;
        }
    }
}

void CleanerMainWindow::unselectAll()
{
    int tab = ui_.tw_tab->currentIndex();
    switch(tab) {
        case(0):
        historyModel_->unselectAll();
        break;
        case(1):
        vcardsModel_->unselectAll();
        break;
        case(2):
        avatarModel_->unselectAll();
        break;
        case(3):
        optionsModel_->unselectAll();
        break;
    }
}

void CleanerMainWindow::filterEvent()
{
    QString text = ui_.le_filter->text();
    proxyHistoryModel_->setFilterFixedString(text);
    proxyVcardsModel_->setFilterFixedString(text);
}

void CleanerMainWindow::currentTabChanged(int tab)
{
    tab = ui_.tw_tab->currentIndex();
    switch(tab){
        case(0):
        ui_.lbl_selected->setText(QString::number(historyModel_->selectedCount()));
        break;
        case(1):
        ui_.lbl_selected->setText(QString::number(vcardsModel_->selectedCount()));
        break;
        case(2):
        ui_.lbl_selected->setText(QString::number(avatarModel_->selectedCount()));
        break;
        case(3):
        ui_.lbl_selected->setText(QString::number(optionsModel_->selectedCount()));
        break;
        }
    updateStatusBar();
}

void CleanerMainWindow::deleteButtonPressed()
{
    int tab = ui_.tw_tab->currentIndex();
    switch(tab) {
        case(0):
        deleteHistory();
        break;
        case(1):
        deleteVcards();
        break;
        case(2):
        deleteAvatars();
        break;
        case(3):
        deleteOptions();
        break;
    }
}

void CleanerMainWindow::deleteHistory()
{
    int ret = QMessageBox::warning(this, tr("Clear History"),
                       tr("Are You Sure?"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel) return;
    historyModel_->deleteSelected();
    updateStatusBar();
}

void CleanerMainWindow::deleteVcards()
{
    int ret = QMessageBox::warning(this, tr("Clear vCards"),
                       tr("Are You Sure?"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel) return;
    vcardsModel_->deleteSelected();
    updateStatusBar();
}

void CleanerMainWindow::deleteAvatars()
{
    int ret = QMessageBox::warning(this, tr("Clear Avatars"),
                       tr("Are You Sure?"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel) return;

    avatarModel_->deleteSelected();
    updateStatusBar();
}

void CleanerMainWindow::deleteOptions()
{
    int ret = QMessageBox::warning(this, tr("Clear Options"),
                       tr("Not supported yet!"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    Q_UNUSED(ret);
    updateStatusBar();
}

void CleanerMainWindow::viewVcard(const QModelIndex& index)
{
    QModelIndex modelIndex = proxyVcardsModel_->mapToSource(index);
    QString filename = vcardsModel_->filePass(modelIndex);
    new vCardView(filename, this);
}

void CleanerMainWindow::viewHistory(const QModelIndex& index)
{
    QModelIndex modelIndex = proxyHistoryModel_->mapToSource(index);
    QString filename = historyModel_->filePass(modelIndex);
    new HistoryView(filename, this);
}

void CleanerMainWindow::viewAvatar(const QModelIndex& index)
{
    if(index.column() != 1)
        return;

    AvatarView *avaView = new AvatarView(index.data(Qt::DisplayRole).value<QPixmap>(), this);
    avaView->setIcon(cleaner_->iconHost->getIcon("psi/save"));
    avaView->show();
}

void CleanerMainWindow::chooseProfileAct()
{
    QStringList prof;
    foreach(const QString& dir, QDir(profilesConfigDir_).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        prof.append(dir);
    }
    const QString profile = QInputDialog::getItem(this, tr("Choose profile"), tr("Profile:"), prof, prof.indexOf(currentProfileName()), false);
    if(!profile.isEmpty())
        changeProfile(profile);
}

void CleanerMainWindow::changeProfile(const QString& profDir)
{
    vCardDir_ = profilesCacheDir_ + QDir::separator() + profDir + QDir::separator() + "vcard";
    historyDir_ = profilesDataDir_ + QDir::separator() + profDir + QDir::separator() + "history";
    historyModel_->setDirs({historyDir_});
    vcardsModel_->setDirs({vCardDir_});
;
    QStringList avatars;
    avatars.append(avatarsDir());
    avatars.append(picturesDir());
    avatarModel_->setDirs(avatars);

    QString optionsFile = profilesConfigDir_ + "/" + currentProfileName() + "/options.xml";
    optionsModel_->setFile(optionsFile);

    updateStatusBar();
}

void CleanerMainWindow::clearJuick()
{
    int ret = QMessageBox::warning(this, tr("Clear Juick Cache"),
                       tr("Are You Sure?"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel) return;
    QDir dir(cacheDir_ + QDir::separator() + QString::fromUtf8("avatars") + QDir::separator() + QString::fromUtf8("juick"));
    if(dir.exists()) {
        bool b = clearDir(dir.absolutePath());
        if(b) {
            QMessageBox::information(this, tr("Clear Juick Cache"),
                         tr("Juick Cache Successfully Cleared"),
                         QMessageBox::Ok);
        } else {
            QMessageBox::critical(this, tr("Clear Juick Cache"),
                          tr("Something wrong!"),
                          QMessageBox::Ok);
        }
    } else {
        QMessageBox::critical(this, tr("Clear Juick Cache"),
                      tr("Cache Not Found!"),
                      QMessageBox::Ok);
    }
}

void CleanerMainWindow::clearBirhday()
{
    int ret = QMessageBox::warning(this, tr("Clear Birthdays Cache"),
                       tr("Are You Sure?"),
                       QMessageBox::Ok  | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel) return;
    QDir dir(vCardDir_ + QDir::separator() + QString::fromUtf8("Birthdays"));
    if(dir.exists()) {
        bool b = clearDir(dir.absolutePath());
        if(b) {
            QMessageBox::information(this, tr("Clear Birthdays Cache"),
                         tr("Birthdays Cache Successfully Cleared"),
                         QMessageBox::Ok);
        } else {
            QMessageBox::critical(this, tr("Clear Birthdays Cache"),
                          tr("Something wrong!"),
                          QMessageBox::Ok);
        }
    } else {
        QMessageBox::critical(this, tr("Clear Birthdays Cache"),
                      tr("Cache Not Found!"),
                      QMessageBox::Ok);
    }
}

bool CleanerMainWindow::clearDir(const QString& path)
{
    bool b = true;
    QDir dir(path);
    foreach (QString filename, dir.entryList(QDir::Files)) {
        QFile file(path + QDir::separator() + filename);
        if(file.open(QIODevice::ReadWrite)) {
            b = file.remove();
            if(!b) return b;
        }
    }
    foreach (QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        b = clearDir(path + QDir::separator() + subDir);
        if(!b) return b;
    }
    //dir.rmpath(path); //remove dir
    return b;
}

void CleanerMainWindow::closeEvent(QCloseEvent *e)
{
    e->ignore();
    cleaner_->deleteCln();
}

bool CleanerMainWindow::eventFilter(QObject *o, QEvent *e)
{
    if(o == ui_.le_filter && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        if(ke->key() == Qt::Key_Escape) {
            ui_.le_filter->clear();
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}

QString CleanerMainWindow::avatarsDir() const
{
    return cacheDir_ + QDir::separator() + QString::fromUtf8("avatars");
}

QString CleanerMainWindow::picturesDir() const
{    
    QString picturesDir = currentProfileDir() + QDir::separator() + QString::fromUtf8("pictures");
    return picturesDir;
}

QString CleanerMainWindow::currentProfileDir() const
{
    QString profileDir = historyDir_;
    int index = profileDir.size() - profileDir.lastIndexOf("/");
    profileDir.chop(index);
    return profileDir;
}

QString CleanerMainWindow::currentProfileName() const
{
    QString name = currentProfileDir();
    name = name.right(name.size() - name.lastIndexOf("/") - 1);
    return name;
}
