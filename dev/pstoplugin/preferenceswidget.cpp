#include "preferenceswidget.h"

PreferencesWidget::PreferencesWidget(const QColor &username_color,
                                     const QColor &post_id_color,
                                     const QColor &tag_color,
                                     const QColor &quote_color,
                                     const QColor &message_color,
                                     QWidget *parent)
    : QWidget(parent),
    now_changing_button(NULL),
    color_dialog(this),
    curr_username_color(username_color),
    curr_post_id_color(post_id_color),
    curr_tag_color(tag_color),
    curr_quote_color(quote_color),
    curr_message_color(message_color)
{
    connect(&color_dialog, SIGNAL(accepted()),
            this, SLOT(colorDialogOk()));
    connect(&color_dialog, SIGNAL(rejected()),
            this, SLOT(colorDialogCancel()));
    color_dialog.setModal(true);

    preferences_ui.setupUi(this);

    // @username
    preferences_ui.usernameColorButton->setStyleSheet(QString("background-color: %1;").arg(username_color.name()));
    connect(preferences_ui.usernameColorButton, SIGNAL(clicked()), this, SLOT(usernameColorClicked()));

    // #post/id
    preferences_ui.postColorButton->setStyleSheet(QString("background-color: %1;").arg(post_id_color.name()));
    connect(preferences_ui.postColorButton, SIGNAL(clicked()), this, SLOT(postColorClicked()));

    // *tag
    preferences_ui.tagColorButton->setStyleSheet(QString("background-color: %1;").arg(tag_color.name()));
    connect(preferences_ui.tagColorButton, SIGNAL(clicked()), this, SLOT(tagColorClicked()));

    // > quote
    preferences_ui.quoteColorButton->setStyleSheet(QString("background-color: %1;").arg(quote_color.name()));
    connect(preferences_ui.quoteColorButton, SIGNAL(clicked()), this, SLOT(quoteColorClicked()));

    // message
    preferences_ui.messageColorButton->setStyleSheet(QString("background-color: %1;").arg(message_color.name()));
    connect(preferences_ui.messageColorButton, SIGNAL(clicked()), this, SLOT(messageColorClicked()));
}

void PreferencesWidget::usernameColorClicked() {
    color_dialog.setCurrentColor(curr_username_color);
    showChangeButtonColorDialog(preferences_ui.usernameColorButton);
}

void PreferencesWidget::postColorClicked() {
    color_dialog.setCurrentColor(curr_post_id_color);
    showChangeButtonColorDialog(preferences_ui.postColorButton);
}

void PreferencesWidget::tagColorClicked() {
    color_dialog.setCurrentColor(curr_tag_color);
    showChangeButtonColorDialog(preferences_ui.tagColorButton);
}

void PreferencesWidget::quoteColorClicked() {
    color_dialog.setCurrentColor(curr_quote_color);
    showChangeButtonColorDialog(preferences_ui.quoteColorButton);
}

void PreferencesWidget::messageColorClicked() {
    color_dialog.setCurrentColor(curr_message_color);
    showChangeButtonColorDialog(preferences_ui.messageColorButton);
}


void PreferencesWidget::showChangeButtonColorDialog(QAbstractButton * button) {
    now_changing_button = button;
    color_dialog.show();
}

void PreferencesWidget::colorDialogOk() {
    if (now_changing_button) {
        QColor new_color = color_dialog.currentColor();
    now_changing_button->setStyleSheet(QString("background-color: %1;").arg(new_color.name()));

        if (now_changing_button == preferences_ui.usernameColorButton) {
            emit usernameColorChanged(new_color);
        } else if (now_changing_button == preferences_ui.postColorButton) {
            emit postColorChanged(new_color);
        } else if (now_changing_button == preferences_ui.tagColorButton) {
            emit tagColorChanged(new_color);
        } else if (now_changing_button == preferences_ui.quoteColorButton) {
            emit quoteColorChanged(new_color);
        } else if (now_changing_button == preferences_ui.messageColorButton) {
            emit messageColorChanged(new_color);
        }
    }

    now_changing_button = NULL;
}

void PreferencesWidget::colorDialogCancel() {
    now_changing_button = NULL;
}
