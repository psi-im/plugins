#ifndef PREVIEWFILEDIALOG_H
#define PREVIEWFILEDIALOG_H

#include <QFileDialog>
#include <QLabel>

// based on https://www.qtcentre.org/threads/33593-How-can-i-have-a-QFileDialog-with-a-preview-of-the-picture
class PreviewFileDialog: public QFileDialog {
Q_OBJECT
public:
    explicit PreviewFileDialog(QWidget* parent = nullptr, const QString & caption = QString(), const QString & directory =
            QString(), const QString & filter = QString(), int previewWidth = 150);

private slots:
    void onCurrentChanged(const QString & path);

protected:
    QLabel* mpPreview;

};

#endif // PREVIEWFILEDIALOG_H
