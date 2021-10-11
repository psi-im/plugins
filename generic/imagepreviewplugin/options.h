#ifndef OPTIONS_H
#define OPTIONS_H

#include <QWidget>
#include <tuple>

namespace Ui {
class ImagePreviewOptions;
}

class OptionAccessingHost;
class ImagePreviewOptions : public QWidget {
    Q_OBJECT

public:
    explicit ImagePreviewOptions(OptionAccessingHost *optHost, QWidget *parent = nullptr);
    ~ImagePreviewOptions();
    std::tuple<int, int, bool, QString> applyOptions();
    void                                restoreOptions();

private:
    Ui::ImagePreviewOptions *ui;
    OptionAccessingHost *    optHost;
};

#endif // OPTIONS_H
