#include "options.h"
#include "optionaccessinghost.h"
#include "ui_options.h"

#define sizeLimitName "imgpreview-size-limit"
#define previewSizeName "imgpreview-preview-size"
#define allowUpscaleName "imgpreview-allow-upscale"
#define exceptionsName "imgpreview-exceptions"

ImagePreviewOptions::ImagePreviewOptions(OptionAccessingHost *optHost, QWidget *parent) :
    QWidget(parent), ui(new Ui::ImagePreviewOptions), optHost(optHost)
{
    ui->setupUi(this);

    ui->cb_sizeLimit->addItem(tr("512 Kb"), 512 * 1024);
    ui->cb_sizeLimit->addItem(tr("1 Mb"), 1024 * 1024);
    ui->cb_sizeLimit->addItem(tr("2 Mb"), 2 * 1024 * 1024);
    ui->cb_sizeLimit->addItem(tr("5 Mb"), 5 * 1024 * 1024);
    ui->cb_sizeLimit->addItem(tr("10 Mb"), 10 * 1024 * 1024);

    ui->cb_sizeLimit->setCurrentIndex(
        ui->cb_sizeLimit->findData(optHost->getPluginOption(sizeLimitName, 1024 * 1024).toInt(), Qt::UserRole));
    ui->sb_previewSize->setValue(optHost->getPluginOption(previewSizeName, 150).toInt());
    ui->cb_allowUpscale->setChecked(optHost->getPluginOption(allowUpscaleName, true).toBool());
}

ImagePreviewOptions::~ImagePreviewOptions() { delete ui; }

std::tuple<int, int, bool, QString> ImagePreviewOptions::applyOptions()
{
    int     previewSize;
    int     sizeLimit;
    bool    allowUpscale;
    QString exceptions;
    optHost->setPluginOption(previewSizeName, previewSize = ui->sb_previewSize->value());
    optHost->setPluginOption(sizeLimitName,
                             sizeLimit = ui->cb_sizeLimit->itemData(ui->cb_sizeLimit->currentIndex()).toInt());
    optHost->setPluginOption(allowUpscaleName, allowUpscale = ui->cb_allowUpscale->checkState() == Qt::Checked);
    optHost->setPluginOption(exceptionsName, exceptions = ui->te_exceptions->toPlainText());

    return std::tuple<int, int, bool, QString> { previewSize, sizeLimit, allowUpscale, exceptions };
}

void ImagePreviewOptions::restoreOptions()
{
    ui->sb_previewSize->setValue(optHost->getPluginOption(previewSizeName, 150).toInt());
    ui->cb_sizeLimit->setCurrentIndex(
        ui->cb_sizeLimit->findData(optHost->getPluginOption(sizeLimitName, 1024 * 1024).toInt()));
    ui->cb_allowUpscale->setChecked(optHost->getPluginOption(allowUpscaleName, true).toBool());
    ui->te_exceptions->setPlainText(optHost->getPluginOption(exceptionsName, QString()).toString());
}
