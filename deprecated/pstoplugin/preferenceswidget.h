#ifndef PREFERENCESWIDGET_H
#define PREFERENCESWIDGET_H

#include <QWidget>
#include <QColorDialog>
#include "ui_preferences.h"
#include "optionaccessor.h"

class PreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreferencesWidget(const QColor & username_color,
                               const QColor & post_id_color,
                               const QColor & tag_color,
                               const QColor & quote_color,
                               const QColor & message_color,
                               QWidget *parent = 0);
private:
    Ui::PreferencesWidget preferences_ui;
    QAbstractButton * now_changing_button;
    QColorDialog color_dialog;
    QColor curr_username_color;
    QColor curr_post_id_color;
    QColor curr_tag_color;
    QColor curr_quote_color;
    QColor curr_message_color;

private:
    void showChangeButtonColorDialog(QAbstractButton *);

signals:
    void usernameColorChanged(QColor);
    void postColorChanged(QColor);
    void tagColorChanged(QColor);
    void quoteColorChanged(QColor);
    void messageColorChanged(QColor);

private slots:
    void usernameColorClicked();
    void postColorClicked();
    void tagColorClicked();
    void quoteColorClicked();
    void messageColorClicked();
    void colorDialogOk();
    void colorDialogCancel();
};

#endif // PREFERENCESWIDGET_H
