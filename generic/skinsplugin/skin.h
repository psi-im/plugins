#ifndef SKIN_H
#define SKIN_H

#include <QTreeWidgetItem>

#include "ui_getskinname.h"
#include "ui_previewer.h"

class Skin : public QTreeWidgetItem {
public:
    explicit Skin(QTreeWidget *parent = nullptr);
    explicit Skin(QTreeWidgetItem *parent);
    ~Skin() override = default;

    void    setFile(const QString &file);
    QString filePass() const;
    QString name() const;
    QString author() const;
    QString version() const;
    QString skinFolder() const;
    QPixmap previewPixmap() const;
    bool    loadMetadata();

private:
    QString filePass_;
    QString name_;
    QString author_;
    QString version_;
};

class Previewer : public QDialog {
    Q_OBJECT
public:
    Previewer(Skin *skin, QWidget *parent = nullptr);
    bool loadSkinInformation();

private:
    Ui::Previewer ui_;
    Skin         *skin_;

signals:
    void applySkin();
};

class GetSkinName : public QDialog {
    Q_OBJECT
public:
    GetSkinName(QString name, QString author, QString version, QWidget *parent = nullptr);

private slots:
    void okPressed();

signals:
    void ok(QString, QString, QString);

private:
    Ui::GetSkinName ui_;
};

#endif // SKIN_H
