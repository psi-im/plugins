/*
 * extendedoptionsplugin.cpp - plugin
 * Copyright (C) 2010-2014  Evgeny Khryukin
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

#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTextList>
#include <QToolButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QButtonGroup>
#include <QColorDialog>

#include "psiplugin.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "applicationinfoaccessor.h"
#include "applicationinfoaccessinghost.h"
#include "plugininfoprovider.h"


#define constVersion "0.4.2"

class ExtToolButton : public QToolButton
{
	Q_OBJECT
public:
	ExtToolButton() : QToolButton()
	{
		setFixedSize(22,22);
	}

};

class ExtendedOptions : public QObject, public PsiPlugin, public OptionAccessor, public ApplicationInfoAccessor, public PluginInfoProvider
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.psi-plus.ExtendedOptions")
	Q_INTERFACES(PsiPlugin OptionAccessor ApplicationInfoAccessor PluginInfoProvider)

public:
	ExtendedOptions() = default;
	virtual QString name() const;
	virtual QString shortName() const;
	virtual QString version() const;
	virtual QWidget* options();
	virtual bool enable();
	virtual bool disable();
	virtual void optionChanged(const QString& option);
	virtual void applyOptions();
	virtual void restoreOptions();
	virtual void setOptionAccessingHost(OptionAccessingHost* host);
	virtual void setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host);
	virtual QString pluginInfo();
	virtual QPixmap icon() const;

private slots:
	void chooseColor(QAbstractButton*);
	void hack();

private:
	void setWhatThis();

private:
	QString readFile();
	void saveFile(const QString& text);
	QString profileDir();

	OptionAccessingHost *psiOptions = nullptr;
	ApplicationInfoAccessingHost *appInfo = nullptr;
	bool enabled = false;
	QPointer<QWidget> options_;

	// Chats-----
	//QCheckBox *htmlRender = nullptr;
	QCheckBox *confirmClearing = nullptr;
	QCheckBox *messageIcons = nullptr;
	//QCheckBox *altnSwitch = nullptr;
	QCheckBox *showAvatar = nullptr;
	QSpinBox *avatarSize = nullptr;
	QCheckBox *sayMode = nullptr;
	QCheckBox *disableSend = nullptr;
	QCheckBox *auto_capitalize = nullptr;
	QCheckBox *auto_scroll_to_bottom = nullptr;
	QLineEdit *chat_caption = nullptr;
	QComboBox *default_jid_mode = nullptr;
	QTextEdit *default_jid_mode_ignorelist = nullptr;
	QCheckBox *show_status_changes = nullptr;
	QCheckBox *chat_status_with_priority = nullptr;
	QCheckBox *scaledIcons = nullptr;


	// MUC-----
	QCheckBox *showJoins = nullptr;
	QCheckBox *showRole = nullptr;
	QCheckBox *showStatus = nullptr;
	QCheckBox *leftMucRoster = nullptr;
	QCheckBox *showGroups = nullptr;
	QCheckBox *showAffIcons = nullptr;
	QCheckBox *skipAutojoin = nullptr;
	QTextEdit *bookmarksListSkip = nullptr;
	QCheckBox *mucClientIcons = nullptr;
	//QCheckBox *rosterNickColors = nullptr;
	QCheckBox *mucHtml = nullptr;
	QCheckBox *hideAutoJoin = nullptr;
	QCheckBox *show_initial_joins = nullptr;
	QCheckBox *status_with_priority = nullptr;
	QCheckBox *show_status_icons = nullptr;
	QCheckBox *use_slim_group_headings = nullptr;
	QComboBox *userlist_contact_sort_style = nullptr;
	QCheckBox *avatars_at_left = nullptr;
	QCheckBox *avatars_show = nullptr;
	QSpinBox *userlist_avatars_size = nullptr;
	QSpinBox *userlist_avatars_radius = nullptr;
	QLineEdit *muc_leave_status_message = nullptr;
	QCheckBox *accept_defaults = nullptr;
	QCheckBox *auto_configure = nullptr;
	QCheckBox *allowMucEvents = nullptr;
	QCheckBox *storeMucPrivates = nullptr;

	// Roster
	QCheckBox *resolveNicks = nullptr;
	QCheckBox *lockRoster = nullptr;
	QCheckBox *leftRoster = nullptr;
	QCheckBox *singleLineStatus = nullptr;
	QCheckBox *avatarTip = nullptr;
	QCheckBox *statusTip = nullptr;
	QCheckBox *geoTip = nullptr;
	QCheckBox *pgpTip = nullptr;
	QCheckBox *clientTip = nullptr;
	QComboBox *sortContacts = nullptr;
	// QComboBox *sortGroups = nullptr;
	// QComboBox *sortAccs = nullptr;
	QCheckBox *leftAvatars = nullptr;
	QCheckBox *defaultAvatar = nullptr;
	QCheckBox *showStatusIcons = nullptr;
	QCheckBox *statusIconsOverAvatars = nullptr;
	QCheckBox *auto_delete_unlisted = nullptr;

	// Menu------
	QCheckBox *admin = nullptr;
	QCheckBox *activeChats = nullptr;
	QCheckBox *pgpKey = nullptr;
	QCheckBox *picture = nullptr;
	QCheckBox *changeProfile = nullptr;
	QCheckBox *chat = nullptr;
	QCheckBox *invis = nullptr;
	QCheckBox *xa = nullptr;
	QCheckBox *enableMessages = nullptr;


	// Look-----
	QToolButton *popupBorder = nullptr;
	QToolButton *linkColor = nullptr;
	QToolButton *mailtoColor = nullptr;
	QToolButton *moderColor = nullptr;
	QToolButton *visitorColor = nullptr;
	QToolButton *parcColor = nullptr;
	QToolButton *noroleColor = nullptr;
	QToolButton *tipText = nullptr;
	QToolButton *tipBase = nullptr;
	QToolButton *composingBut = nullptr;
	QToolButton *unreadBut = nullptr;
	QGroupBox *groupTip = nullptr;
	QGroupBox *groupMucRoster = nullptr;

	// CSS----------------
	QTextEdit *chatCss = nullptr;
	QTextEdit *rosterCss = nullptr;
	QTextEdit *popupCss = nullptr;
	QTextEdit *tooltipCss = nullptr;

	// Tabs----------------
	QCheckBox *disableScroll = nullptr;
	QCheckBox *bottomTabs = nullptr;
	QCheckBox *closeButton = nullptr;
	QComboBox *middleButton = nullptr;
	QCheckBox *showTabIcons = nullptr;
	QCheckBox *hideWhenClose = nullptr;
	QCheckBox *canCloseTab = nullptr;
	QComboBox *mouseDoubleclick = nullptr;
	QCheckBox *multiRow = nullptr;

	// Misc
	QCheckBox *flash_windows = nullptr;
	QCheckBox *account_single = nullptr;
	QCheckBox *xml_console_enable_at_login = nullptr;
	QCheckBox *lastActivity = nullptr;
	QCheckBox *sndMucNotify = nullptr;
	QCheckBox *popupsSuppressDnd = nullptr;
	QCheckBox *popupsSuppressAway = nullptr;
};

QString ExtendedOptions::name() const
{
	return "Extended Options Plugin";
}

QString ExtendedOptions::shortName() const
{
	return "extopt";
}

QString ExtendedOptions::version() const
{
	return constVersion;
}

bool ExtendedOptions::enable()
{
	if(psiOptions) {
		enabled = true;
	}
	return enabled;
}

bool ExtendedOptions::disable()
{
	enabled = false;
	return true;
}

QWidget* ExtendedOptions::options()
{
	if (!enabled) {
		return 0;
	}

	options_ = new QWidget;
	QVBoxLayout *mainLayout = new QVBoxLayout(options_);
	QTabWidget *tabs = new QTabWidget;
	QWidget *tab1 = new QWidget;
	QWidget *tab2 = new QWidget;
	QWidget *tab3 = new QWidget;
	QWidget *tab4 = new QWidget;
	QWidget *tab5 = new QWidget;
	QWidget *tab6 = new QWidget;
	QWidget *tab7 = new QWidget;
	QWidget *tab8 = new QWidget;
	QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);
	QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);
	QVBoxLayout *tab3Layout = new QVBoxLayout(tab3);
	QVBoxLayout *tab4Layout = new QVBoxLayout(tab4);
	QVBoxLayout *tab5Layout = new QVBoxLayout(tab5);
	QVBoxLayout *tab6Layout = new QVBoxLayout(tab6);
	QVBoxLayout *tab7Layout = new QVBoxLayout(tab7);
	QVBoxLayout *tab8Layout = new QVBoxLayout(tab8);
	tabs->addTab(tab1, tr("Chat"));
	tabs->addTab(tab2, tr("Groupchat"));
	tabs->addTab(tab5, tr("Tabs"));
	tabs->addTab(tab3, tr("Roster"));
	tabs->addTab(tab4, tr("Menu"));
	tabs->addTab(tab6, tr("Look"));
	tabs->addTab(tab7, tr("CSS"));
	tabs->addTab(tab8, tr("Misc"));


	//Chats-----
	//	htmlRender = new QCheckBox(tr("Enable HTML rendering in chat window"));
	confirmClearing = new QCheckBox(tr("Ask for confirmation before clearing chat window"));
	messageIcons = new QCheckBox(tr("Enable icons in chat"));
	scaledIcons = new QCheckBox(tr("Scaled message icons"));

	/* altnSwitch = new QCheckBox(tr("Switch tabs with \"ALT+(1-9)\""));
	   altnSwitch->setChecked(psiOptions->getGlobalOption("options.ui.tabs.alt-n-switch").toBool());*/
	showAvatar = new QCheckBox(tr("Show Avatar"));
	sayMode = new QCheckBox(tr("Enable \"Says style\""));
	disableSend = new QCheckBox(tr("Hide \"Send\" button"));

	avatarSize = new QSpinBox;
	QGridLayout *chatGridLayout = new QGridLayout;
	chatGridLayout->addWidget(new QLabel(tr("Avatar size:")), 0, 0);
	chatGridLayout->addWidget(avatarSize, 0, 1);

	default_jid_mode = new QComboBox;
	default_jid_mode->addItems(QStringList() << "auto" << "barejid");
	chatGridLayout->addWidget(new QLabel(tr("Default JID mode:")), 1, 0);
	chatGridLayout->addWidget(default_jid_mode);


	auto_capitalize = new QCheckBox(tr("Automatically capitalize the first letter in a sentence"));
	auto_scroll_to_bottom = new QCheckBox(tr("Automatically scroll down the log when a message was sent"));
	show_status_changes = new QCheckBox(tr("Show status changes"));
	chat_status_with_priority = new QCheckBox(tr("Show status priority"));
	default_jid_mode_ignorelist = new QTextEdit;
	default_jid_mode_ignorelist->setMaximumWidth(300);

	chat_caption = new QLineEdit;


	tab1Layout->addWidget(new QLabel(tr("Chat window caption:")));
	tab1Layout->addWidget(chat_caption);
	// tab1Layout->addWidget(htmlRender);
	tab1Layout->addWidget(confirmClearing);
	tab1Layout->addWidget(messageIcons);
	tab1Layout->addWidget(scaledIcons);
	//tab1Layout->addWidget(altnSwitch);
	tab1Layout->addWidget(disableSend);
	tab1Layout->addWidget(sayMode);
	tab1Layout->addWidget(showAvatar);
	tab1Layout->addLayout(chatGridLayout);
	tab1Layout->addWidget(new QLabel(tr("Default JID mode ignore list:")));
	tab1Layout->addWidget(default_jid_mode_ignorelist);
	tab1Layout->addWidget(auto_capitalize);
	tab1Layout->addWidget(auto_scroll_to_bottom);
	tab1Layout->addWidget(show_status_changes);
	tab1Layout->addWidget(chat_status_with_priority);
	tab1Layout->addStretch();




	//MUC-----
	QTabWidget *mucTabWidget = new QTabWidget;
	QWidget *mucGeneralWidget = new QWidget;
	QWidget *mucRosterWidget = new QWidget;
	QVBoxLayout *mucGeneralLayout = new QVBoxLayout(mucGeneralWidget);
	QVBoxLayout *mucRosterLayout = new QVBoxLayout(mucRosterWidget);
	mucTabWidget->addTab(mucGeneralWidget, tr("General"));
	mucTabWidget->addTab(mucRosterWidget, tr("Roster"));


	showJoins = new QCheckBox(tr("Show joins"));
	show_initial_joins = new QCheckBox(tr("Show initial joins"));
	status_with_priority = new QCheckBox(tr("Show status with priority"));
	showRole = new QCheckBox(tr("Show roles and affiliations changes"));
	showStatus = new QCheckBox(tr("Show status changes"));
	skipAutojoin = new QCheckBox(tr("Enable autojoin for bookmarked groupchats"));
	hideAutoJoin = new QCheckBox(tr("Hide groupchat on auto-join"));
	mucHtml = new QCheckBox(tr("Enable HTML rendering in groupchat chat window"));
	allowMucEvents = new QCheckBox(tr("Allow groupchat highlight events"));
	storeMucPrivates = new QCheckBox(tr("Store MUC private messages in history"));

	muc_leave_status_message = new QLineEdit;
	accept_defaults = new QCheckBox(tr("Automatically accept the default room configuration"));
	accept_defaults->setToolTip(tr("Automatically accept the default room configuration when a new room is created"));
	auto_configure = new QCheckBox(tr("Automatically open the configuration dialog when a new room is created"));
	auto_configure->setToolTip(tr("Automatically open the configuration dialog when a new room is created.\n"
								  "This option only has effect if accept-defaults is false."));

	bookmarksListSkip = new QTextEdit();
	bookmarksListSkip->setMaximumWidth(300);
	bookmarksListSkip->setPlainText(readFile());

	mucGeneralLayout->addWidget(accept_defaults);
	mucGeneralLayout->addWidget(auto_configure);
	mucGeneralLayout->addWidget(mucHtml);
	mucGeneralLayout->addWidget(showJoins);
	mucGeneralLayout->addWidget(show_initial_joins);
	mucGeneralLayout->addWidget(showRole);
	mucGeneralLayout->addWidget(showStatus);
	mucGeneralLayout->addWidget(status_with_priority);
	mucGeneralLayout->addWidget(skipAutojoin);
	mucGeneralLayout->addWidget(hideAutoJoin);
	mucGeneralLayout->addWidget(allowMucEvents);
	mucGeneralLayout->addWidget(storeMucPrivates);
	mucGeneralLayout->addWidget(new QLabel(tr("Disable autojoin to following groupchats:\n(specify JIDs)")));
	mucGeneralLayout->addWidget(bookmarksListSkip);
	mucGeneralLayout->addWidget(new QLabel(tr("Groupchat leave status message:")));
	mucGeneralLayout->addWidget(muc_leave_status_message);
	mucGeneralLayout->addStretch();


	leftMucRoster = new QCheckBox(tr("Place groupchat roster at left"));
	showGroups = new QCheckBox(tr("Show groups"));
	use_slim_group_headings = new QCheckBox(tr("Use slim group heading"));
	show_status_icons = new QCheckBox(tr("Show status icons"));
	showAffIcons = new QCheckBox(tr("Show affiliation icons"));
	mucClientIcons = new QCheckBox(tr("Show client icons"));
	//	rosterNickColors = new QCheckBox(tr("Enable nick coloring"));
	avatars_show = new QCheckBox(tr("Show avatars"));
	avatars_at_left = new QCheckBox(tr("Place avatars at left"));

	userlist_contact_sort_style = new QComboBox;
	userlist_contact_sort_style->addItems(QStringList() << "alpha" << "status");

	QGridLayout *mucRosterGrid = new QGridLayout;
	mucRosterGrid->addWidget(new QLabel(tr("Sort style for contacts:")), 0, 0);
	mucRosterGrid->addWidget(userlist_contact_sort_style, 0, 2);

	userlist_avatars_size = new QSpinBox;
	mucRosterGrid->addWidget(new QLabel(tr("Avatars size:")), 1, 0);
	mucRosterGrid->addWidget(userlist_avatars_size, 1, 2);

	userlist_avatars_radius = new QSpinBox;
	mucRosterGrid->addWidget(new QLabel(tr("Avatars radius:")), 2, 0);
	mucRosterGrid->addWidget(userlist_avatars_radius, 2, 2);


	mucRosterLayout->addWidget(leftMucRoster);
	mucRosterLayout->addWidget(showGroups);
	mucRosterLayout->addWidget(use_slim_group_headings);
	mucRosterLayout->addWidget(show_status_icons);
	mucRosterLayout->addWidget(showAffIcons);
	mucRosterLayout->addWidget(mucClientIcons);
	//	mucRosterLayout->addWidget(rosterNickColors);
	mucRosterLayout->addWidget(avatars_show);
	mucRosterLayout->addWidget(avatars_at_left);
	mucRosterLayout->addLayout(mucRosterGrid);
	mucRosterLayout->addStretch();

	tab2Layout->addWidget(mucTabWidget);




	//Roster
	resolveNicks = new QCheckBox(tr("Resolve nicks on contact add"));
	lockRoster = new QCheckBox(tr("Lockdown roster"));
	leftRoster = new QCheckBox(tr("Place roster at left in \"all-in-one-window\" mode"));
	singleLineStatus = new QCheckBox(tr("Contact name and status message in a row"));
	leftAvatars = new QCheckBox(tr("Place avatars at left"));
	defaultAvatar = new QCheckBox(tr("If contact does not have avatar, use default avatar"));
	showStatusIcons = new QCheckBox(tr("Show status icons"));
	statusIconsOverAvatars = new QCheckBox(tr("Place status icon over avatar"));

	auto_delete_unlisted = new QCheckBox(tr("Automatically remove temporary contacts"));
	auto_delete_unlisted->hide(); //FIXME!!!! Remove this when the option will be fixed

	QGroupBox *groupBox = new QGroupBox(tr("Tooltips:"));
	QVBoxLayout *boxLayout = new QVBoxLayout(groupBox);

	avatarTip = new QCheckBox(tr("Show avatar"));
	statusTip = new QCheckBox(tr("Show last status"));
	pgpTip = new QCheckBox(tr("Show PGP"));
	clientTip = new QCheckBox(tr("Show client version"));
	geoTip = new QCheckBox(tr("Show geolocation"));

	boxLayout->addWidget(avatarTip);
	boxLayout->addWidget(statusTip);
	boxLayout->addWidget(pgpTip);
	boxLayout->addWidget(clientTip);
	boxLayout->addWidget(geoTip);

	sortContacts = new QComboBox;
	sortContacts->addItem("alpha");
	sortContacts->addItem("status");

	QHBoxLayout *sortLayout = new QHBoxLayout();
	sortLayout->addWidget(new QLabel(tr("Sort style for contacts:")));
	sortLayout->addWidget(sortContacts);
	sortLayout->addStretch();

	tab3Layout->addWidget(resolveNicks);
	tab3Layout->addWidget(lockRoster);
	tab3Layout->addWidget(auto_delete_unlisted);
	tab3Layout->addWidget(leftRoster);
	tab3Layout->addWidget(singleLineStatus);
	tab3Layout->addWidget(showStatusIcons);
	tab3Layout->addWidget(statusIconsOverAvatars);
	tab3Layout->addWidget(leftAvatars);
	tab3Layout->addWidget(defaultAvatar);
	tab3Layout->addLayout(sortLayout);
	tab3Layout->addWidget(groupBox);
	tab3Layout->addStretch();


	//Menu------
	admin = new QCheckBox(tr("Show \"Admin\" option in account menu"));
	activeChats = new QCheckBox(tr("Show \"Active Chats\" option in contact menu"));
	pgpKey = new QCheckBox(tr("Show \"Assign OpenPGP Key\" option in contact menu"));
	picture = new QCheckBox(tr("Show \"Picture\" option in contact menu"));
	changeProfile = new QCheckBox(tr("Show \"Change Profile\" option in main menu"));
	chat = new QCheckBox(tr("Show \"Chat\" option in status menu"));
	invis = new QCheckBox(tr("Show \"Invisible\" option in status menu"));
	xa = new QCheckBox(tr("Show \"XA\" option in status menu"));
	enableMessages = new QCheckBox(tr("Enable single messages"));

	tab4Layout->addWidget(enableMessages);
	tab4Layout->addWidget(admin);
	tab4Layout->addWidget(activeChats);
	tab4Layout->addWidget(pgpKey);
	tab4Layout->addWidget(picture);
	tab4Layout->addWidget(changeProfile);
	tab4Layout->addWidget(chat);
	tab4Layout->addWidget(invis);
	tab4Layout->addWidget(xa);
	tab4Layout->addStretch();


	//Look----
	popupBorder = new ExtToolButton;
	QHBoxLayout *pbLayout = new QHBoxLayout;
	pbLayout->addWidget(new QLabel(tr("Popup border color:")));
	pbLayout->addStretch();
	pbLayout->addWidget(popupBorder);

	linkColor = new ExtToolButton;
	QHBoxLayout *lcLayout = new QHBoxLayout;
	lcLayout->addWidget(new QLabel(tr("Link color:")));
	lcLayout->addStretch();
	lcLayout->addWidget(linkColor);

	mailtoColor = new ExtToolButton;
	QHBoxLayout *mcLayout = new QHBoxLayout;
	mcLayout->addWidget(new QLabel(tr("Mailto color:")));
	mcLayout->addStretch();
	mcLayout->addWidget(mailtoColor);

	moderColor = new ExtToolButton;
	QHBoxLayout *modcLayout = new QHBoxLayout;
	modcLayout->addWidget(new QLabel(tr("Moderators color:")));
	modcLayout->addStretch();
	modcLayout->addWidget(moderColor);

	parcColor = new ExtToolButton;
	QHBoxLayout *parcLayout = new QHBoxLayout;
	parcLayout->addWidget(new QLabel(tr("Participants color:")));
	parcLayout->addStretch();
	parcLayout->addWidget(parcColor);

	visitorColor  = new ExtToolButton;
	QHBoxLayout *vscLayout = new QHBoxLayout;
	vscLayout->addWidget(new QLabel(tr("Visitors color:")));
	vscLayout->addStretch();
	vscLayout->addWidget(visitorColor);

	noroleColor	 = new ExtToolButton;
	QHBoxLayout *nrcLayout = new QHBoxLayout;
	nrcLayout->addWidget(new QLabel(tr("No Role color:")));
	nrcLayout->addStretch();
	nrcLayout->addWidget(noroleColor);

	groupMucRoster = new QGroupBox(tr("Groupchat roster coloring:"));
	groupMucRoster->setCheckable(true);
	QVBoxLayout *mucRosterLay = new QVBoxLayout(groupMucRoster);
	mucRosterLay->addLayout(modcLayout);
	mucRosterLay->addLayout(parcLayout);
	mucRosterLay->addLayout(vscLayout);
	mucRosterLay->addLayout(nrcLayout);
	connect(groupMucRoster, SIGNAL(toggled(bool)), SLOT(hack()));

	tipText = new ExtToolButton;
	QHBoxLayout *ttLayout = new QHBoxLayout;
	ttLayout->addWidget(new QLabel(tr("ToolTip text color:")));
	ttLayout->addStretch();
	ttLayout->addWidget(tipText);

	tipBase = new ExtToolButton;
	QHBoxLayout *tbLayout = new QHBoxLayout;
	tbLayout->addWidget(new QLabel(tr("ToolTip background color:")));
	tbLayout->addStretch();
	tbLayout->addWidget(tipBase);

	groupTip = new QGroupBox(tr("ToolTip coloring:"));
	groupTip->setCheckable(true);
	QVBoxLayout *tipLay = new QVBoxLayout(groupTip);
	tipLay->addLayout(ttLayout);
	tipLay->addLayout(tbLayout);
	connect(groupTip, SIGNAL(toggled(bool)), SLOT(hack()));

	composingBut = new ExtToolButton;
	QHBoxLayout *composingLayout = new QHBoxLayout;
	composingLayout->addWidget(new QLabel(tr("Text color for \"composing\" events on tabs:")));
	composingLayout->addStretch();
	composingLayout->addWidget(composingBut);

	unreadBut = new ExtToolButton;
	QHBoxLayout *unreadLayout = new QHBoxLayout;
	unreadLayout->addWidget(new QLabel(tr("Text color for \"unread\" events on tabs:")));
	unreadLayout->addStretch();
	unreadLayout->addWidget(unreadBut);

	QButtonGroup *b_color = new QButtonGroup;
	b_color->addButton(popupBorder);
	b_color->addButton(linkColor);
	b_color->addButton(mailtoColor);
	b_color->addButton(moderColor);
	b_color->addButton(parcColor);
	b_color->addButton(visitorColor);
	b_color->addButton(noroleColor);
	b_color->addButton(tipText);
	b_color->addButton(tipBase);
	b_color->addButton(composingBut);
	b_color->addButton(unreadBut);
	connect(b_color, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(chooseColor(QAbstractButton*)));

	QGroupBox *group3Box = new QGroupBox(tr("Colors:"));
	QVBoxLayout *box3Layout = new QVBoxLayout(group3Box);
	box3Layout->addWidget(groupMucRoster);
	box3Layout->addWidget(groupTip);
	box3Layout->addLayout(pbLayout);
	box3Layout->addLayout(lcLayout);
	box3Layout->addLayout(mcLayout);
	box3Layout->addLayout(composingLayout);
	box3Layout->addLayout(unreadLayout);
	box3Layout->addStretch();

	tab6Layout->addWidget(group3Box);

	//CSS----------------
	QTabWidget *cssTab = new QTabWidget;

	chatCss = new QTextEdit;
	chatCss->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	rosterCss = new QTextEdit;
	rosterCss->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	popupCss = new QTextEdit;
	popupCss->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	tooltipCss = new QTextEdit;
	tooltipCss->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	cssTab->addTab(chatCss, tr("Chat"));
	cssTab->addTab(rosterCss, tr("Roster"));
	cssTab->addTab(popupCss, tr("Popup"));
	cssTab->addTab(tooltipCss, tr("Tooltip"));

	QLabel *cssLabel = new QLabel(tr("<a href=\"https://psi-plus.com/wiki/skins_css\">CSS for Psi+</a>"));
	cssLabel->setOpenExternalLinks(true);

	tab7Layout->addWidget(cssTab);
	tab7Layout->addWidget(cssLabel);


	//Tabs--------------------
	disableScroll = new QCheckBox(tr("Disable wheel scroll"));
	bottomTabs = new QCheckBox(tr("Put tabs at bottom of chat window"));
	closeButton = new QCheckBox(tr("Show Close Button on tabs"));
	showTabIcons = new QCheckBox(tr("Show status icons on tabs"));
	hideWhenClose = new QCheckBox(tr("Hide tab when close chat window"));
	canCloseTab = new QCheckBox(tr("Allow closing inactive tabs"));
	multiRow = new QCheckBox(tr("Enable multirow tabs"));

	middleButton = new QComboBox;
	QHBoxLayout *mbLayout = new QHBoxLayout;
	middleButton->addItem("none");
	middleButton->addItem("hide");
	middleButton->addItem("close");
	middleButton->addItem("detach");
	mbLayout->addWidget(new QLabel(tr("Action for mouse middle click on tabs:")));
	mbLayout->addStretch();
	mbLayout->addWidget(middleButton);

	mouseDoubleclick = new QComboBox;
	QHBoxLayout *mdLayout = new QHBoxLayout;
	mouseDoubleclick->addItem("none");
	mouseDoubleclick->addItem("hide");
	mouseDoubleclick->addItem("close");
	mouseDoubleclick->addItem("detach");
	mdLayout->addWidget(new QLabel(tr("Action for mouse double click on tabs:")));
	mdLayout->addStretch();
	mdLayout->addWidget(mouseDoubleclick);

	tab5Layout->addWidget(hideWhenClose);
	tab5Layout->addWidget(disableScroll);
	tab5Layout->addWidget(bottomTabs);
	tab5Layout->addWidget(canCloseTab);
	tab5Layout->addWidget(closeButton);
	tab5Layout->addWidget(showTabIcons);
	tab5Layout->addWidget(multiRow);
	tab5Layout->addLayout(mbLayout);
	tab5Layout->addLayout(mdLayout);
	tab5Layout->addStretch();


	//Misc--------------------------------
	flash_windows = new QCheckBox(tr("Enable windows flashing"));
	account_single = new QCheckBox(tr("Enable \"Single Account\" mode"));
	xml_console_enable_at_login = new QCheckBox(tr("Enable XML-console on login"));
	lastActivity = new QCheckBox(tr("Enable last activity server"));
	sndMucNotify = new QCheckBox(tr("Enable sound notifications for every groupchat message"));
	popupsSuppressDnd = new QCheckBox(tr("Disable popup notifications if status is DND"));
	popupsSuppressAway = new QCheckBox(tr("Disable popup notifications if status is Away"));

	QGroupBox *ngb = new QGroupBox(tr("Notifications"));
	QVBoxLayout *nvbl = new QVBoxLayout(ngb);
	nvbl->addWidget(flash_windows);
	nvbl->addWidget(sndMucNotify);
	nvbl->addWidget(popupsSuppressDnd);
	nvbl->addWidget(popupsSuppressAway);

	tab8Layout->addWidget(account_single);
	tab8Layout->addWidget(xml_console_enable_at_login);
	tab8Layout->addWidget(lastActivity);
	tab8Layout->addWidget(ngb);
	tab8Layout->addStretch();


	QLabel *wikiLink = new QLabel(tr("<a href=\"https://psi-plus.com/wiki/plugins#extended_options_plugin\">Wiki (Online)</a>"));
	wikiLink->setOpenExternalLinks(true);

	mainLayout->addWidget(tabs);
	mainLayout->addWidget(wikiLink);

	setWhatThis();

	restoreOptions();

	return options_;
}

void ExtendedOptions::applyOptions()
{
	if(!options_)
		return;

	//Chats-----
	//psiOptions->setGlobalOption("options.html.chat.render",QVariant(htmlRender->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.warn-before-clear",QVariant(confirmClearing->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.use-message-icons",QVariant(messageIcons->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.scaled-message-icons",QVariant(scaledIcons->isChecked()));
	//psiOptions->setGlobalOption("options.ui.tabs.alt-n-switch",QVariant(altnSwitch->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.avatars.show",QVariant(showAvatar->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.use-chat-says-style",QVariant(sayMode->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.avatars.size", QVariant(avatarSize->value()));
	psiOptions->setGlobalOption("options.ui.disable-send-button",QVariant(disableSend->isChecked()));

	psiOptions->setGlobalOption("options.ui.chat.auto-capitalize",QVariant(auto_capitalize->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.auto-scroll-to-bottom",QVariant(auto_scroll_to_bottom->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.caption",QVariant(chat_caption->text()));
	psiOptions->setGlobalOption("options.ui.chat.default-jid-mode",QVariant(default_jid_mode->currentText()));
	psiOptions->setGlobalOption("options.ui.chat.default-jid-mode-ignorelist",
								QVariant(default_jid_mode_ignorelist->toPlainText()
										 .split(QRegExp("\\s+"), QString::SkipEmptyParts)
										 .join(",")));
	psiOptions->setGlobalOption("options.ui.chat.show-status-changes",QVariant(show_status_changes->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.status-with-priority",QVariant(chat_status_with_priority->isChecked()));


	//MUC-----
	psiOptions->setGlobalOption("options.ui.muc.allow-highlight-events", QVariant(allowMucEvents->isChecked()));
	psiOptions->setGlobalOption("options.muc.show-joins",QVariant(showJoins->isChecked()));
	psiOptions->setGlobalOption("options.muc.show-role-affiliation",QVariant(showRole->isChecked()));
	psiOptions->setGlobalOption("options.muc.show-status-changes",QVariant(showStatus->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.roster-at-left",QVariant(leftMucRoster->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.show-groups",QVariant(showGroups->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.show-affiliation-icons",QVariant(showAffIcons->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.show-client-icons",QVariant(mucClientIcons->isChecked()));
	psiOptions->setGlobalOption("options.muc.bookmarks.auto-join",QVariant(skipAutojoin->isChecked()));
	//	psiOptions->setGlobalOption("options.ui.muc.userlist.nick-coloring",QVariant(rosterNickColors->isChecked()));
	psiOptions->setGlobalOption("options.html.muc.render",QVariant(mucHtml->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.hide-on-autojoin",QVariant(hideAutoJoin->isChecked()));
	saveFile(bookmarksListSkip->toPlainText());

	psiOptions->setGlobalOption("options.ui.muc.show-initial-joins",QVariant(show_initial_joins->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.status-with-priority",QVariant(status_with_priority->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.show-status-icons",QVariant(show_status_icons->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.use-slim-group-headings",QVariant(use_slim_group_headings->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.contact-sort-style",QVariant(userlist_contact_sort_style->currentText()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.avatars.avatars-at-left",QVariant(avatars_at_left->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.avatars.show",QVariant(avatars_show->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.avatars.size",QVariant(userlist_avatars_size->value()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.avatars.radius",QVariant(userlist_avatars_radius->value()));
	psiOptions->setGlobalOption("options.muc.leave-status-message",QVariant(muc_leave_status_message->text()));
	psiOptions->setGlobalOption("options.muc.accept-defaults",QVariant(accept_defaults->isChecked()));
	psiOptions->setGlobalOption("options.muc.auto-configure",QVariant(auto_configure->isChecked()));
	psiOptions->setGlobalOption("options.history.store-muc-private", QVariant(storeMucPrivates->isChecked()));

	//Tabs--------------------
	psiOptions->setGlobalOption("options.ui.tabs.disable-wheel-scroll",QVariant(disableScroll->isChecked()));
	psiOptions->setGlobalOption("options.ui.tabs.put-tabs-at-bottom",QVariant(bottomTabs->isChecked()));
	psiOptions->setGlobalOption("options.ui.tabs.show-tab-close-buttons",QVariant(closeButton->isChecked()));
	psiOptions->setGlobalOption("options.ui.tabs.mouse-middle-button",QVariant(middleButton->currentText()));
	psiOptions->setGlobalOption("options.ui.tabs.mouse-doubleclick-action",QVariant(mouseDoubleclick->currentText()));
	psiOptions->setGlobalOption("options.ui.tabs.show-tab-icons",QVariant(showTabIcons->isChecked()));
	psiOptions->setGlobalOption("options.ui.chat.hide-when-closing",QVariant(hideWhenClose->isChecked()));
	psiOptions->setGlobalOption("options.ui.tabs.can-close-inactive-tab",QVariant(canCloseTab->isChecked()));
	psiOptions->setGlobalOption("options.ui.tabs.multi-rows",QVariant(multiRow->isChecked()));

	//Roster-----
	psiOptions->setGlobalOption("options.contactlist.resolve-nicks-on-contact-add",QVariant(resolveNicks->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.lockdown-roster",QVariant(lockRoster->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.aio-left-roster",QVariant(leftRoster->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.status-messages.single-line",QVariant(singleLineStatus->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.avatar",QVariant(avatarTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.last-status",QVariant(statusTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.pgp",QVariant(pgpTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.geolocation",QVariant(geoTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.client-version",QVariant(clientTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.contact-sort-style",QVariant(sortContacts->currentText()));
	// psiOptions->setGlobalOption("options.ui.contactlist.group-sort-style",QVariant(sortGroups->currentText()));
	// psiOptions->setGlobalOption("options.ui.contactlist.account-sort-style",QVariant(sortAccs->currentText()));
	psiOptions->setGlobalOption("options.ui.contactlist.avatars.avatars-at-left",QVariant(leftAvatars->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.avatars.use-default-avatar",QVariant(defaultAvatar->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.show-status-icons",QVariant(showStatusIcons->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.status-icon-over-avatar",QVariant(statusIconsOverAvatars->isChecked()));
	psiOptions->setGlobalOption("options.ui.contactlist.auto-delete-unlisted",QVariant(auto_delete_unlisted->isChecked()));


	//Menu------
	psiOptions->setGlobalOption("options.ui.menu.account.admin",QVariant(admin->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.contact.active-chats",QVariant(activeChats->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.contact.custom-pgp-key",QVariant(pgpKey->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.contact.custom-picture",QVariant(picture->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.main.change-profile",QVariant(changeProfile->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.status.chat",QVariant(chat->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.status.invisible",QVariant(invis->isChecked()));
	psiOptions->setGlobalOption("options.ui.menu.status.xa",QVariant(xa->isChecked()));
	psiOptions->setGlobalOption("options.ui.message.enabled", QVariant(enableMessages->isChecked()));

	//Look----
	psiOptions->setGlobalOption("options.ui.look.colors.passive-popup.border", QVariant(popupBorder->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.chat.link-color", QVariant(linkColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.chat.mailto-color", QVariant(mailtoColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.muc.role-moderator", QVariant(moderColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.muc.role-participant", QVariant(parcColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.muc.role-visitor", QVariant(visitorColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.muc.role-norole", QVariant(noroleColor->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.tooltip.text", QVariant(tipText->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.tooltip.background", QVariant(tipBase->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.chat.unread-message-color", QVariant(unreadBut->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.chat.composing-color", QVariant(composingBut->property("psi_color").value<QColor>()));
	psiOptions->setGlobalOption("options.ui.look.colors.tooltip.enable",QVariant(groupTip->isChecked()));
	psiOptions->setGlobalOption("options.ui.muc.userlist.nick-coloring",QVariant(groupMucRoster->isChecked()));

	//CSS----------------
	psiOptions->setGlobalOption("options.ui.chat.css", QVariant(chatCss->toPlainText()));
	psiOptions->setGlobalOption("options.ui.contactlist.css", QVariant(rosterCss->toPlainText()));
	psiOptions->setGlobalOption("options.ui.notifications.passive-popups.css", QVariant(popupCss->toPlainText()));
	psiOptions->setGlobalOption("options.ui.contactlist.tooltip.css", QVariant(tooltipCss->toPlainText()));


	//Misc--------------------
	psiOptions->setGlobalOption("options.ui.flash-windows", QVariant(flash_windows->isChecked()));
	psiOptions->setGlobalOption("options.ui.account.single", QVariant(account_single->isChecked()));
	psiOptions->setGlobalOption("options.xml-console.enable-at-login", QVariant(xml_console_enable_at_login->isChecked()));
	psiOptions->setGlobalOption("options.service-discovery.last-activity", QVariant(lastActivity->isChecked()));
	psiOptions->setGlobalOption("options.ui.notifications.sounds.notify-every-muc-message", QVariant(sndMucNotify->isChecked()));
	psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd", QVariant(popupsSuppressDnd->isChecked()));
	psiOptions->setGlobalOption("options.ui.notifications.passive-popups.suppress-while-away", QVariant(popupsSuppressAway->isChecked()));
}

void ExtendedOptions::restoreOptions()
{
	if(!options_)
		return;

	//Chats-----
	// htmlRender->setChecked(psiOptions->getGlobalOption("options.html.chat.render").toBool());
	confirmClearing->setChecked(psiOptions->getGlobalOption("options.ui.chat.warn-before-clear").toBool());
	messageIcons->setChecked(psiOptions->getGlobalOption("options.ui.chat.use-message-icons").toBool());
	scaledIcons->setChecked(psiOptions->getGlobalOption("options.ui.chat.scaled-message-icons").toBool());
	// altnSwitch->setChecked(psiOptions->getGlobalOption("options.ui.tabs.alt-n-switch").toBool());
	showAvatar->setChecked(psiOptions->getGlobalOption("options.ui.chat.avatars.show").toBool());
	avatarSize->setValue(psiOptions->getGlobalOption("options.ui.chat.avatars.size").toInt());
	sayMode->setChecked(psiOptions->getGlobalOption("options.ui.chat.use-chat-says-style").toBool());
	disableSend->setChecked(psiOptions->getGlobalOption("options.ui.disable-send-button").toBool());

	auto_capitalize->setChecked(psiOptions->getGlobalOption("options.ui.chat.auto-capitalize").toBool());
	auto_scroll_to_bottom->setChecked(psiOptions->getGlobalOption("options.ui.chat.auto-scroll-to-bottom").toBool());
	chat_caption->setText(psiOptions->getGlobalOption("options.ui.chat.caption").toString());
	default_jid_mode->setCurrentIndex(default_jid_mode->findText(psiOptions->getGlobalOption("options.ui.chat.default-jid-mode").toString()));
	default_jid_mode_ignorelist->setPlainText(psiOptions->getGlobalOption("options.ui.chat.default-jid-mode-ignorelist").toString().split(",").join("\n"));
	show_status_changes->setChecked(psiOptions->getGlobalOption("options.ui.chat.show-status-changes").toBool());
	chat_status_with_priority->setChecked(psiOptions->getGlobalOption("options.ui.chat.status-with-priority").toBool());

	//MUC-----
	allowMucEvents->setChecked(psiOptions->getGlobalOption("options.ui.muc.allow-highlight-events").toBool());
	showJoins->setChecked(psiOptions->getGlobalOption("options.muc.show-joins").toBool());
	showRole->setChecked(psiOptions->getGlobalOption("options.muc.show-role-affiliation").toBool());
	showStatus->setChecked(psiOptions->getGlobalOption("options.muc.show-status-changes").toBool());
	leftMucRoster->setChecked(psiOptions->getGlobalOption("options.ui.muc.roster-at-left").toBool());
	showGroups->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.show-groups").toBool());
	showAffIcons->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.show-affiliation-icons").toBool());
	skipAutojoin->setChecked(psiOptions->getGlobalOption("options.muc.bookmarks.auto-join").toBool());
	bookmarksListSkip->setPlainText(readFile());
	mucClientIcons->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.show-client-icons").toBool());
	//	rosterNickColors->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.nick-coloring").toBool());
	mucHtml->setChecked(psiOptions->getGlobalOption("options.html.muc.render").toBool());
	hideAutoJoin->setChecked(psiOptions->getGlobalOption("options.ui.muc.hide-on-autojoin").toBool());

	show_initial_joins->setChecked(psiOptions->getGlobalOption("options.ui.muc.show-initial-joins").toBool());
	status_with_priority->setChecked(psiOptions->getGlobalOption("options.ui.muc.status-with-priority").toBool());
	show_status_icons->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.show-status-icons").toBool());
	use_slim_group_headings->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.use-slim-group-headings").toBool());
	userlist_contact_sort_style->setCurrentIndex(userlist_contact_sort_style->findText(psiOptions->getGlobalOption("options.ui.muc.userlist.contact-sort-style").toString()));
	avatars_at_left->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.avatars.avatars-at-left").toBool());
	avatars_show->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.avatars.show").toBool());
	userlist_avatars_size->setValue(psiOptions->getGlobalOption("options.ui.muc.userlist.avatars.size").toInt());
	userlist_avatars_radius->setValue(psiOptions->getGlobalOption("options.ui.muc.userlist.avatars.radius").toInt());
	muc_leave_status_message->setText(psiOptions->getGlobalOption("options.muc.leave-status-message").toString());
	accept_defaults->setChecked(psiOptions->getGlobalOption("options.muc.accept-defaults").toBool());
	auto_configure->setChecked(psiOptions->getGlobalOption("options.muc.auto-configure").toBool());
	storeMucPrivates->setChecked(psiOptions->getGlobalOption("options.history.store-muc-private").toBool());

	//Tabs----------------------
	disableScroll->setChecked(psiOptions->getGlobalOption("options.ui.tabs.disable-wheel-scroll").toBool());
	bottomTabs->setChecked(psiOptions->getGlobalOption("options.ui.tabs.put-tabs-at-bottom").toBool());
	closeButton->setChecked(psiOptions->getGlobalOption("options.ui.tabs.show-tab-close-buttons").toBool());
	middleButton->setCurrentIndex(middleButton->findText(psiOptions->getGlobalOption("options.ui.tabs.mouse-middle-button").toString()));
	int index = mouseDoubleclick->findText(psiOptions->getGlobalOption("options.ui.tabs.mouse-doubleclick-action").toString());
	if(index == -1)
		index = 0;
	mouseDoubleclick->setCurrentIndex(index);
	showTabIcons->setChecked(psiOptions->getGlobalOption("options.ui.tabs.show-tab-icons").toBool());
	hideWhenClose->setChecked(psiOptions->getGlobalOption("options.ui.chat.hide-when-closing").toBool());
	canCloseTab->setChecked(psiOptions->getGlobalOption("options.ui.tabs.can-close-inactive-tab").toBool());
	multiRow->setChecked(psiOptions->getGlobalOption("options.ui.tabs.multi-rows").toBool());

	//Roster
	resolveNicks->setChecked(psiOptions->getGlobalOption("options.contactlist.resolve-nicks-on-contact-add").toBool());
	lockRoster->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.lockdown-roster").toBool());
	leftRoster->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.aio-left-roster").toBool());
	singleLineStatus->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.status-messages.single-line").toBool());
	avatarTip->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.avatar").toBool());
	statusTip->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.last-status").toBool());
	geoTip->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.geolocation").toBool());
	pgpTip->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.pgp").toBool());
	clientTip->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.client-version").toBool());
	sortContacts->setCurrentIndex(sortContacts->findText(psiOptions->getGlobalOption("options.ui.contactlist.contact-sort-style").toString()));
	leftAvatars->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.avatars.avatars-at-left").toBool());
	defaultAvatar->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.avatars.use-default-avatar").toBool());
	showStatusIcons->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.show-status-icons").toBool());
	statusIconsOverAvatars->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.status-icon-over-avatar").toBool());
	auto_delete_unlisted->setChecked(psiOptions->getGlobalOption("options.ui.contactlist.auto-delete-unlisted").toBool());

	//Menu------
	admin->setChecked(psiOptions->getGlobalOption("options.ui.menu.account.admin").toBool());
	activeChats->setChecked(psiOptions->getGlobalOption("options.ui.menu.contact.active-chats").toBool());
	pgpKey->setChecked(psiOptions->getGlobalOption("options.ui.menu.contact.custom-pgp-key").toBool());
	picture->setChecked(psiOptions->getGlobalOption("options.ui.menu.contact.custom-picture").toBool());
	changeProfile->setChecked(psiOptions->getGlobalOption("options.ui.menu.main.change-profile").toBool());
	chat->setChecked(psiOptions->getGlobalOption("options.ui.menu.status.chat").toBool());
	invis->setChecked(psiOptions->getGlobalOption("options.ui.menu.status.invisible").toBool());
	xa->setChecked(psiOptions->getGlobalOption("options.ui.menu.status.xa").toBool());
	enableMessages->setChecked(psiOptions->getGlobalOption("options.ui.message.enabled").toBool());

	//Look----
	QColor color;
	color = psiOptions->getGlobalOption("options.ui.look.colors.passive-popup.border").toString();
	popupBorder->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	popupBorder->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.chat.link-color").toString();
	linkColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	linkColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.chat.mailto-color").toString();
	mailtoColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	mailtoColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.muc.role-moderator").toString();
	moderColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	moderColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.muc.role-participant").toString();
	parcColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	parcColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.muc.role-visitor").toString();
	visitorColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	visitorColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.muc.role-norole").toString();
	noroleColor->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	noroleColor->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.tooltip.text").toString();
	tipText->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	tipText->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.tooltip.background").toString();
	tipBase->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	tipBase->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.chat.unread-message-color").toString();
	unreadBut->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	unreadBut->setProperty("psi_color", color);
	color = psiOptions->getGlobalOption("options.ui.look.colors.chat.composing-color").toString();
	composingBut->setStyleSheet(QString("background-color: %1;").arg(color.name()));
	composingBut->setProperty("psi_color", color);
	groupTip->setChecked(psiOptions->getGlobalOption("options.ui.look.colors.tooltip.enable").toBool());
	groupMucRoster->setChecked(psiOptions->getGlobalOption("options.ui.muc.userlist.nick-coloring").toBool());

	//CSS----------------
	chatCss->setText(psiOptions->getGlobalOption("options.ui.chat.css").toString());
	rosterCss->setText(psiOptions->getGlobalOption("options.ui.contactlist.css").toString());
	popupCss->setText(psiOptions->getGlobalOption("options.ui.notifications.passive-popups.css").toString());
	tooltipCss->setText(psiOptions->getGlobalOption("options.ui.contactlist.tooltip.css").toString());

	//Misc--------------------
	flash_windows->setChecked(psiOptions->getGlobalOption("options.ui.flash-windows").toBool());
	account_single->setChecked(psiOptions->getGlobalOption("options.ui.account.single").toBool());
	xml_console_enable_at_login->setChecked(psiOptions->getGlobalOption("options.xml-console.enable-at-login").toBool());
	lastActivity->setChecked(psiOptions->getGlobalOption("options.service-discovery.last-activity").toBool());
	sndMucNotify->setChecked(psiOptions->getGlobalOption("options.ui.notifications.sounds.notify-every-muc-message").toBool());
	popupsSuppressDnd->setChecked(psiOptions->getGlobalOption("options.ui.notifications.passive-popups.suppress-while-dnd").toBool());
	popupsSuppressAway->setChecked(psiOptions->getGlobalOption("options.ui.notifications.passive-popups.suppress-while-away").toBool());
}


void ExtendedOptions::setOptionAccessingHost(OptionAccessingHost *host)
{
	psiOptions = host;
}

void ExtendedOptions::optionChanged(const QString &option)
{
	Q_UNUSED(option);
}

void ExtendedOptions::chooseColor(QAbstractButton* button)
{
	QColor c;
	c = button->property("psi_color").value<QColor>();
	c = QColorDialog::getColor(c, new QWidget());
	if(c.isValid()) {
		button->setProperty("psi_color", c);
		button->setStyleSheet(QString("background-color: %1").arg(c.name()));
	}

	hack();
}

void ExtendedOptions::setApplicationInfoAccessingHost(ApplicationInfoAccessingHost* host)
{
	appInfo = host;
}

QString ExtendedOptions::readFile()
{
	QFile file(profileDir() + QDir::separator() + QString("mucskipautojoin.txt"));
	if(file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		in.setCodec("UTF-8");
		return in.readAll();
	}
	return QString();
}

void ExtendedOptions::saveFile(const QString& text)
{
	QFile file(profileDir() + QDir::separator() + QString("mucskipautojoin.txt"));
	if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		if(text.isEmpty())
			return;

		QTextStream out(&file);
		out.setCodec("UTF-8");
		out.setGenerateByteOrderMark(false);
		out << text << endl;
	}
}

void ExtendedOptions::hack()
{
	//Enable "Apply" button
	confirmClearing->toggle();
	confirmClearing->toggle();
}

void ExtendedOptions::setWhatThis()
{
	//Chats-----
	confirmClearing->setWhatsThis("options.ui.chat.warn-before-clear");
	messageIcons->setWhatsThis("options.ui.chat.use-message-icons");
	scaledIcons->setWhatsThis("options.ui.chat.scaled-message-icons");
	showAvatar->setWhatsThis("options.ui.chat.avatars.show");
	avatarSize->setWhatsThis("options.ui.chat.avatars.size");
	sayMode->setWhatsThis("options.ui.chat.use-chat-says-style");
	disableSend->setWhatsThis("options.ui.disable-send-button");

	auto_capitalize->setWhatsThis("options.ui.chat.auto-capitalize");
	auto_scroll_to_bottom->setWhatsThis("options.ui.chat.auto-scroll-to-bottom");
	chat_caption->setWhatsThis("options.ui.chat.caption");
	default_jid_mode->setWhatsThis("options.ui.chat.default-jid-mode");
	default_jid_mode_ignorelist->setWhatsThis("options.ui.chat.default-jid-mode-ignorelist");
	show_status_changes->setWhatsThis("options.ui.chat.show-status-changes");
	chat_status_with_priority->setWhatsThis("options.ui.chat.status-with-priority");

	//MUC-----
	allowMucEvents->setWhatsThis("options.ui.muc.allow-highlight-events");
	showJoins->setWhatsThis("options.muc.show-joins");
	showRole->setWhatsThis("options.muc.show-role-affiliation");
	showStatus->setWhatsThis("options.muc.show-status-changes");
	leftMucRoster->setWhatsThis("options.ui.muc.roster-at-left");
	showGroups->setWhatsThis("options.ui.muc.userlist.show-groups");
	showAffIcons->setWhatsThis("options.ui.muc.userlist.show-affiliation-icons");
	skipAutojoin->setWhatsThis("options.muc.bookmarks.auto-join");
	mucClientIcons->setWhatsThis("options.ui.muc.userlist.show-client-icons");
	mucHtml->setWhatsThis("options.html.muc.render");
	hideAutoJoin->setWhatsThis("options.ui.muc.hide-on-autojoin");

	show_initial_joins->setWhatsThis("options.ui.muc.show-initial-joins");
	status_with_priority->setWhatsThis("options.ui.muc.status-with-priority");
	show_status_icons->setWhatsThis("options.ui.muc.userlist.show-status-icons");
	use_slim_group_headings->setWhatsThis("options.ui.muc.userlist.use-slim-group-headings");
	userlist_contact_sort_style->setWhatsThis("options.ui.muc.userlist.contact-sort-style");
	avatars_at_left->setWhatsThis("options.ui.muc.userlist.avatars.avatars-at-left");
	avatars_show->setWhatsThis("options.ui.muc.userlist.avatars.show");
	userlist_avatars_size->setWhatsThis("options.ui.muc.userlist.avatars.size");
	userlist_avatars_radius->setWhatsThis("options.ui.muc.userlist.avatars.radius");
	muc_leave_status_message->setWhatsThis("options.muc.leave-status-message");
	accept_defaults->setWhatsThis("options.muc.accept-defaults");
	auto_configure->setWhatsThis("options.muc.auto-configure");
	storeMucPrivates->setWhatsThis("options.history.store-muc-private");

	//Tabs----------------------
	disableScroll->setWhatsThis("options.ui.tabs.disable-wheel-scroll");
	bottomTabs->setWhatsThis("options.ui.tabs.put-tabs-at-bottom");
	closeButton->setWhatsThis("options.ui.tabs.show-tab-close-buttons");
	middleButton->setWhatsThis("options.ui.tabs.mouse-middle-button");
	mouseDoubleclick->setWhatsThis("options.ui.tabs.mouse-doubleclick-action");
	showTabIcons->setWhatsThis("options.ui.tabs.show-tab-icons");
	hideWhenClose->setWhatsThis("options.ui.chat.hide-when-closing");
	canCloseTab->setWhatsThis("options.ui.tabs.can-close-inactive-tab");
	multiRow->setWhatsThis("options.ui.tabs.multi-rows");

	//Roster
	resolveNicks->setWhatsThis("options.contactlist.resolve-nicks-on-contact-add");
	lockRoster->setWhatsThis("options.ui.contactlist.lockdown-roster");
	leftRoster->setWhatsThis("options.ui.contactlist.aio-left-roster");
	singleLineStatus->setWhatsThis("options.ui.contactlist.status-messages.single-line");
	avatarTip->setWhatsThis("options.ui.contactlist.tooltip.avatar");
	statusTip->setWhatsThis("options.ui.contactlist.tooltip.last-status");
	geoTip->setWhatsThis("options.ui.contactlist.tooltip.geolocation");
	pgpTip->setWhatsThis("options.ui.contactlist.tooltip.pgp");
	clientTip->setWhatsThis("options.ui.contactlist.tooltip.client-version");
	sortContacts->setWhatsThis("options.ui.contactlist.contact-sort-style");
	leftAvatars->setWhatsThis("options.ui.contactlist.avatars.avatars-at-left");
	defaultAvatar->setWhatsThis("options.ui.contactlist.avatars.use-default-avatar");
	showStatusIcons->setWhatsThis("options.ui.contactlist.show-status-icons");
	statusIconsOverAvatars->setWhatsThis("options.ui.contactlist.status-icon-over-avatar");
	auto_delete_unlisted->setWhatsThis("options.ui.contactlist.auto-delete-unlisted");

	//Menu------
	admin->setWhatsThis("options.ui.menu.account.admin");
	activeChats->setWhatsThis("options.ui.menu.contact.active-chats");
	pgpKey->setWhatsThis("options.ui.menu.contact.custom-pgp-key");
	picture->setWhatsThis("options.ui.menu.contact.custom-picture");
	changeProfile->setWhatsThis("options.ui.menu.main.change-profile");
	chat->setWhatsThis("options.ui.menu.status.chat");
	invis->setWhatsThis("options.ui.menu.status.invisible");
	xa->setWhatsThis("options.ui.menu.status.xa");
	enableMessages->setWhatsThis("options.ui.message.enabled");

	//Look----
	popupBorder->setWhatsThis("options.ui.look.colors.passive-popup.border");
	linkColor->setWhatsThis("options.ui.look.colors.chat.link-color");
	mailtoColor->setWhatsThis("options.ui.look.colors.chat.mailto-color");
	moderColor->setWhatsThis("options.ui.look.colors.muc.role-moderator");
	parcColor->setWhatsThis("options.ui.look.colors.muc.role-participant");
	visitorColor->setWhatsThis("options.ui.look.colors.muc.role-visitor");
	noroleColor->setWhatsThis("options.ui.look.colors.muc.role-norole");
	tipText->setWhatsThis("options.ui.look.colors.tooltip.text");
	tipBase->setWhatsThis("options.ui.look.colors.tooltip.background");
	unreadBut->setWhatsThis("options.ui.look.colors.chat.unread-message-color");
	composingBut->setWhatsThis("options.ui.look.colors.chat.composing-color");
	groupTip->setWhatsThis("options.ui.look.colors.tooltip.enable");
	groupMucRoster->setWhatsThis("options.ui.muc.userlist.nick-coloring");

	//CSS----------------
	chatCss->setWhatsThis("options.ui.chat.css");
	rosterCss->setWhatsThis("options.ui.contactlist.css");
	popupCss->setWhatsThis("options.ui.notifications.passive-popups.css");
	tooltipCss->setWhatsThis("options.ui.contactlist.tooltip.css");

	//Misc--------------------
	flash_windows->setWhatsThis("options.ui.flash-windows");
	account_single->setWhatsThis("options.ui.account.single");
	xml_console_enable_at_login->setWhatsThis("options.xml-console.enable-at-login");
	lastActivity->setWhatsThis("options.service-discovery.last-activity");
	sndMucNotify->setWhatsThis("options.ui.notifications.sounds.notify-every-muc-message");
	popupsSuppressDnd->setWhatsThis("options.ui.notifications.passive-popups.suppress-while-dnd");
	popupsSuppressAway->setWhatsThis("options.ui.notifications.passive-popups.suppress-while-away");
}

QString ExtendedOptions::profileDir()
{
	QString profileDir = appInfo->appHistoryDir();
	int index = profileDir.size() - profileDir.lastIndexOf("/");
	profileDir.chop(index);
	return profileDir;
}

QString ExtendedOptions::pluginInfo()
{
	return tr("Author: ") +	 "Dealer_WeARE\n"
		 + tr("Email: ") + "wadealer@gmail.com\n\n"
		 + trUtf8("This plugin is designed to allow easy configuration of some advanced options in Psi+.\n"
				  "This plugin gives you access to advanced application options, which do not have a graphical user interface.\n\n"
				  "Importantly: a large part of the options are important system settings. These require extra attention and proper"
				  "understanding of the results when changing the option.");
}

QPixmap ExtendedOptions::icon() const
{
	return QPixmap(":/icons/extendedoptions.png");
}

#include "extendedoptionsplugin.moc"
