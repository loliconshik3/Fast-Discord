#include "ui/settings/menus/userprofile.h"

#include "ui/settings/components/divider.h"
#include "ui/settings/components/settingsbutton.h"
#include "ui/settings/components/popup.h"
#include "ui/settings/components/selectimagebutton.h"

#include <QTextEdit>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>

namespace Ui {

UserProfile::UserProfile(Api::RessourceManager *rmp, QWidget *parent)
    : QScrollArea()
{
    rm = rmp;
    modified = false;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    Widget *container = new Widget(this);
    layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    container->setMaximumWidth(Settings::scale(752));
    container->setContentsMargins(Settings::scale(40), Settings::scale(60), Settings::scale(40), Settings::scale(80));
    container->setBackgroundColor(Settings::BackgroundPrimary);

    QFont font;
    font.setPixelSize(Settings::scale(20));
    font.setFamily("whitney");

    Label *title = new Label("User Profile", container);
    title->setFixedHeight(Settings::scale(24));
    title->setFont(font);
    title->setTextColor(Settings::HeaderPrimary);

    layout->addWidget(title);
    layout->addWidget(new Divider());

    Widget *profileEdit = new Widget(container);
    QHBoxLayout *profileLayout = new QHBoxLayout(profileEdit);
    profileLayout->setSpacing(Settings::scale(20));
    profileLayout->setContentsMargins(0, 0, 0, 0);

    font.setPixelSize(Settings::scale(12));
    Widget *customization = new Widget(profileEdit);
    QVBoxLayout *customizationLayout = new QVBoxLayout(customization);
    customizationLayout->setSpacing(0);
    customizationLayout->setContentsMargins(0, 0, 0, 0);


    Widget *avatarSection = new Widget(customization);
    QVBoxLayout *avatarLayout = new QVBoxLayout(avatarSection);
    avatarLayout->setSpacing(Settings::scale(8));
    avatarLayout->setContentsMargins(0, 0, 0, 0);

    Label *avatarTitle = new Label("AVATAR", avatarSection);
    avatarTitle->setFixedHeight(Settings::scale(16));
    avatarTitle->setFont(font);
    avatarTitle->setTextColor(Settings::HeaderSecondary);
    
    Widget *avatarButtons = new Widget(avatarSection);
    QHBoxLayout *avatarButtonsLayout = new QHBoxLayout(avatarButtons);
    avatarButtonsLayout->setSpacing(Settings::scale(4));
    avatarButtonsLayout->setContentsMargins(0, 0, 0, 0);
    SettingsButton *changeAvatar = new SettingsButton(SettingsButton::Type::Normal, "Change Avatar", avatarButtons);
    SettingsButton *removeAvatar = new SettingsButton(SettingsButton::Type::NoBackground, "Remove Avatar", avatarButtons);
    avatarButtonsLayout->addWidget(changeAvatar);
    avatarButtonsLayout->addWidget(removeAvatar);
    avatarButtonsLayout->addStretch(1);

    avatarLayout->addWidget(avatarTitle);
    avatarLayout->addWidget(avatarButtons);


    Widget *colorSection = new Widget(customization);
    QVBoxLayout *colorLayout = new QVBoxLayout(colorSection);
    colorLayout->setSpacing(Settings::scale(16));
    colorLayout->setContentsMargins(0, 0, 0, 0);

    Label *colorTitle = new Label("PROFILE COLOR", colorSection);
    colorTitle->setFixedHeight(Settings::scale(16));
    colorTitle->setFont(font);
    colorTitle->setTextColor(Settings::HeaderSecondary);

    Widget *colorButtons = new Widget(colorSection);
    QHBoxLayout *colorButtonsLayout = new QHBoxLayout(colorButtons);
    colorButtonsLayout->setSpacing(Settings::scale(16));
    colorButtonsLayout->setContentsMargins(0, 0, 0, 0);

    font.setPixelSize(Settings::scale(11));
    Widget *defaultColorContainer = new Widget(colorButtons);
    defaultColorContainer->setFixedSize(Settings::scale(69), Settings::scale(70));
    QVBoxLayout *defaultColorLayout = new QVBoxLayout(defaultColorContainer);
    defaultColorLayout->setSpacing(Settings::scale(4));
    defaultColorLayout->setContentsMargins(0, 0, 0, 0);
    Label *defaultColorTitle = new Label("Default", defaultColorContainer);
    defaultColorTitle->setFixedSize(Settings::scale(69), Settings::scale(16));
    defaultColorTitle->setFlags(Qt::AlignCenter);
    defaultColorTitle->setFont(font);
    defaultColorTitle->setTextColor(Settings::HeaderSecondary);
    defaultColor = new ColorLabel(defaultColorContainer);
    defaultColorLayout->addWidget(defaultColor);
    defaultColorLayout->addWidget(defaultColorTitle, 1, Qt::AlignHCenter);

    Widget *customColorContainer = new Widget(colorButtons);
    customColorContainer->setFixedSize(Settings::scale(69), Settings::scale(70));
    QVBoxLayout *customColorLayout = new QVBoxLayout(customColorContainer);
    customColorLayout->setSpacing(Settings::scale(4));
    customColorLayout->setContentsMargins(0, 0, 0, 0);
    Label *customColorTitle = new Label("Custom", customColorContainer);
    customColorTitle->setFixedSize(Settings::scale(69), Settings::scale(16));
    customColorTitle->setFlags(Qt::AlignCenter);
    customColorTitle->setFont(font);
    customColorTitle->setTextColor(Settings::HeaderSecondary);
    customColor = new ColorLabel(customColorContainer);
    Widget *colorIcon = new Widget(customColor);
    colorIcon->setPixmap(QPixmap(":grab-color-icon.svg").scaled(Settings::scale(14), Settings::scale(14)));
    colorIcon->move(Settings::scale(50), 0);
    customColorLayout->addWidget(customColor);
    customColorLayout->addWidget(customColorTitle, 0, Qt::AlignHCenter);
    
    rm->getClient([this](void *clientPtr){
        Api::Client *client = reinterpret_cast<Api::Client *>(clientPtr);

        bannerColor = "#" + QString::number(client->bannerColor, Settings::scale(16));
        avatarFile = client->avatar;
        bannerFile = client->banner;
        bio = client->bio;

        if (client->bannerColor != 0) {
            customColor->setClicked();
            customColor->setColor("#" + QString::number(client->bannerColor, Settings::scale(16)));
        } else {
            defaultColor->setClicked();
            customColor->setColor("#000");
        }
        
        if (client->avatar.isNull()) {
            defaultColor->setColor("#000");
        } else {
            QString channelIconFileName = client->id + (client->avatar.indexOf("a_") == 0 ? ".gif" : ".png");
            rm->getImage([this](void *imageFileName){
                QImage img(*reinterpret_cast<QString *>(imageFileName));
                int count = 0;
                int r = 0, g = 0, b = 0;
                for (int i = 0 ; i < img.width() ; i++) {
                    for (int j = 0 ; j < img.height() ; j++) {
                        count++;
                        QColor c = img.pixel(i, j);
                        r += c.red();
                        g += c.green();
                        b += c.blue();
                    }
                }

                r /= count;
                g /= count;
                b /= count;
                defaultBannerColor = QColor(r, g, b);
                defaultColor->setColor(defaultBannerColor.name());
            }, "https://cdn.discordapp.com/avatars/" + client->id + "/" + client->avatar, channelIconFileName);
        }
    });

    colorButtonsLayout->addWidget(defaultColorContainer);
    colorButtonsLayout->addWidget(customColorContainer);
    colorButtonsLayout->addStretch(1);

    colorLayout->addWidget(colorTitle);
    colorLayout->addWidget(colorButtons);
    colorLayout->addStretch(1);


    Widget *bannerSection = new Widget(customization);
    QVBoxLayout *bannerLayout = new QVBoxLayout(bannerSection);
    bannerLayout->setSpacing(Settings::scale(8));
    bannerLayout->setContentsMargins(0, 0, 0, 0);

    font.setPixelSize(Settings::scale(12));
    Widget *bannerTitle = new Widget(bannerSection);
    QHBoxLayout *bannerTitleLayout = new QHBoxLayout(bannerTitle);
    bannerTitleLayout->setSpacing(0);
    bannerTitleLayout->setContentsMargins(0, 0, 0, 0);
    Label *bannerTitleText = new Label("PROFILE BANNER", bannerTitle);
    bannerTitleText->setMinimumSize(Settings::scale(104), Settings::scale(16));
    bannerTitleText->setFont(font);
    bannerTitleText->setTextColor(Settings::HeaderSecondary);
    QLabel *bannerTitleIcon = new QLabel(bannerTitle);
    bannerTitleIcon->setPixmap(QPixmap(":nitro-icon.svg").scaled(Settings::scale(16), Settings::scale(16)));
    bannerTitleLayout->addWidget(bannerTitleText);
    bannerTitleLayout->addWidget(bannerTitleIcon);
    bannerTitleLayout->addStretch(1);

    Label *bannerDescription = new Label("We recommend an image of at least 600x240. You can\nupload a PNG, JPG, or an animated GIF under 10 MB.", bannerSection);
    bannerDescription->setMinimumHeight(Settings::scale(40));
    bannerDescription->setFont(font);
    bannerDescription->setTextColor(Settings::HeaderSecondary);

    Widget *bannerButtons = new Widget(bannerSection);
    QHBoxLayout *bannerButtonsLayout = new QHBoxLayout(bannerButtons);
    bannerButtonsLayout->setSpacing(Settings::scale(4));
    bannerButtonsLayout->setContentsMargins(0, 0, 0, 0);
    rm->getClient([bannerButtons, bannerButtonsLayout, this](void *clientPtr){
        Api::Client *client = reinterpret_cast<Api::Client *>(clientPtr);
        if (client->purchasedFlags) {
            SettingsButton *changeBanner = new SettingsButton(SettingsButton::Type::Normal, "Change Banner", bannerButtons);
            bannerButtonsLayout->addWidget(changeBanner);

            if (!client->banner.isNull()) {
                SettingsButton *removeBanner = new SettingsButton(SettingsButton::Type::NoBackground, "Remove Banner", bannerButtons);
                bannerButtonsLayout->addWidget(removeBanner);
                QObject::connect(removeBanner, &SettingsButton::clicked, [this](){
                    profile->setBanner(QString(), "#000");
                    bannerFile.clear();
                    profileChanged();
                });
            }
            QObject::connect(changeBanner, &SettingsButton::clicked, [this](){
                QWidget *parentWidget = this;
                while (parentWidget->parent()) parentWidget = (Widget *)parentWidget->parent();

                QFont font;
                font.setPixelSize(Settings::scale(12));
                font.setFamily("whitney");
                Widget *container = new Widget(nullptr);
                container->setFixedSize(Settings::scale(200), Settings::scale(212));
                QHBoxLayout *mainLayout = new QHBoxLayout(container);
                mainLayout->setContentsMargins(Settings::scale(16), 0, Settings::scale(16), Settings::scale(16));
                Label *content = new Label(container);
                content->setFixedSize(Settings::scale(160), Settings::scale(196));
                content->setTextColor(Settings::BackgroundSecondaryAlt);
                QVBoxLayout *layout = new QVBoxLayout(content);
                layout->setSpacing(Settings::scale(32));
                layout->setContentsMargins(Settings::scale(16), Settings::scale(16), Settings::scale(16), Settings::scale(16));
                SelectImageButton *button = new SelectImageButton(content);
                QLabel *desc = new QLabel("Upload Image", content);
                desc->setFixedHeight(Settings::scale(16));
                desc->setFont(font);
                layout->addWidget(button);
                layout->addWidget(desc, 0, Qt::AlignHCenter);
                mainLayout->addWidget(content, 0, Qt::AlignCenter);

                PopUp *popUp = new PopUp(container, Settings::scale(240), Settings::scale(280), QString(), "Select Image", false, true, QString(), QString(), QString(), false, false, parentWidget->size(), parentWidget);
                QObject::connect(popUp, &PopUp::cancelled, [popUp](){popUp->deleteLater();});
                QObject::connect(button, &SelectImageButton::clicked, [this, parentWidget, popUp](){
                    QString defaultFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/";
                    if (defaultFolder.isEmpty()) {
                        defaultFolder = ".";
                    }
                    QString imagePath = QFileDialog::getOpenFileName(nullptr, "Select Image", defaultFolder);
                    popUp->deleteLater();
                    if (!imagePath.isNull()) {
                        profile->setBanner(imagePath, QString());
                        bannerFile = imagePath;
                        profileChanged();
                    }
                });
            });
            
            bannerButtonsLayout->addStretch(1);
        } else {
            SettingsButton *getNitroButton = new SettingsButton(SettingsButton::Type::Normal, "Get Nitro", bannerButtons);
            QObject::connect(getNitroButton, &SettingsButton::clicked, [this](){
                QWidget *parentWidget = this;
                while (parentWidget->parent()) parentWidget = (Widget *)parentWidget->parent();
                PopUp *popUp = new PopUp(new Widget(nullptr), Settings::scale(440), Settings::scale(120), QString(), "Get Nitro", true, false, "<div style=\"text-align: center\">You can get nitro at <a style=\"color: #00AFF4; text-decoration: none;\" href=\"https://discord.com/nitro\">https://discord.com/nitro</a></div>", QString(), QString(), false, false, parentWidget->size(), parentWidget);
                QObject::connect(popUp, &PopUp::cancelled, [popUp](){popUp->deleteLater();});
            });
            bannerButtonsLayout->addWidget(getNitroButton);
            bannerButtonsLayout->addStretch(1);
        }
    });

    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(bannerDescription);
    bannerLayout->addSpacing(Settings::scale(4));
    bannerLayout->addWidget(bannerButtons);


    Widget *aboutSection = new Widget(customization);
    QVBoxLayout *aboutLayout = new QVBoxLayout(aboutSection);
    aboutLayout->setSpacing(Settings::scale(8));
    aboutLayout->setContentsMargins(0, 0, 0, 0);

    Label *aboutTitle = new Label("ABOUT ME", aboutSection);
    aboutTitle->setFixedHeight(Settings::scale(16));
    aboutTitle->setFont(font);
    aboutTitle->setTextColor(Settings::HeaderSecondary);

    Label *aboutDescription = new Label("You can use markdown and links if you'd like.", aboutSection);
    aboutDescription->setFixedHeight(Settings::scale(16));
    aboutDescription->setFont(font);
    aboutDescription->setTextColor(Settings::HeaderSecondary);

    font.setPixelSize(Settings::scale(14));
    Widget *aboutEdit = new Widget(aboutSection);
    aboutEdit->setFixedSize(Settings::scale(340), Settings::scale(136));
    aboutEdit->setBackgroundColor(Settings::BackgroundTertiary);
    aboutEdit->setBorderRadius(Settings::scale(3));
    QHBoxLayout *aboutEditLayout = new QHBoxLayout(aboutEdit);
    aboutEditLayout->setSpacing(Settings::scale(10));
    aboutEditLayout->setContentsMargins(Settings::scale(16), Settings::scale(11), Settings::scale(16), Settings::scale(11));
    aboutTextEdit = new QTextEdit(aboutEdit);
    aboutTextEdit->setFont(font);
    aboutTextEdit->setTextColor(Settings::colors[Settings::TextNormal]);
    aboutTextEdit->setStyleSheet(QString("background-color: ") + Settings::colors[Settings::BackgroundTertiary].name());
    Label *charCount = new Label(aboutEdit);
    charCount->setFixedSize(Settings::scale(25), Settings::scale(16));
    charCount->setTextColor(Settings::TextNormal);
    charCount->setBackgroundColor(Settings::BackgroundTertiary);
    rm->getClient([charCount, this](void *clientPtr){
        aboutTextEdit->setText(reinterpret_cast<Api::Client *>(clientPtr)->bio);
        charCount->setText(QString::number(190 - reinterpret_cast<Api::Client *>(clientPtr)->bio.length()));
    });
    charCount->setFont(font);
    charCount->setTextColor(Settings::TextNormal);
    QObject::connect(aboutTextEdit, &QTextEdit::textChanged, [charCount, this](){
        QString text = aboutTextEdit->toPlainText();
        if (text.length() > 190) {
            QTextCursor endCursor = aboutTextEdit->textCursor();
            int position = endCursor.position();
            aboutTextEdit->setPlainText(text.mid(0, 190));
            endCursor.setPosition((position == 191 ? position-1 : position));
            aboutTextEdit->setTextCursor(endCursor);
            bio = text.mid(0, 190);
            profileChanged();
        } else {
            charCount->setText(QString::number(190 - text.length()));
            profile->setBio(text);
            bio = text;
            profileChanged();
        }
    });
    aboutEditLayout->addWidget(aboutTextEdit);
    aboutEditLayout->addWidget(charCount, 0, Qt::AlignBottom);

    aboutLayout->addWidget(aboutTitle);
    aboutLayout->addWidget(aboutDescription);
    aboutLayout->addWidget(aboutEdit);


    Widget *line1 = new Widget(this);
    line1->setFixedSize(Settings::scale(660), 1);
    line1->setBackgroundColor(Settings::BackgroundModifierActive);
    Widget *line2 = new Widget(this);
    line2->setFixedSize(Settings::scale(660), 1);
    line2->setBackgroundColor(Settings::BackgroundModifierActive);

    customizationLayout->addWidget(avatarSection);
    customizationLayout->addSpacing(Settings::scale(24));
    customizationLayout->addWidget(line1);
    customizationLayout->addSpacing(Settings::scale(24));
    customizationLayout->addWidget(colorSection);
    customizationLayout->addSpacing(Settings::scale(24));
    customizationLayout->addWidget(bannerSection);
    customizationLayout->addSpacing(Settings::scale(24));
    customizationLayout->addWidget(line2);
    customizationLayout->addSpacing(Settings::scale(24));
    customizationLayout->addWidget(aboutSection);
    customizationLayout->addStretch(1);


    Widget *preview = new Widget(profileEdit);
    QVBoxLayout *previewLayout = new QVBoxLayout(preview);
    previewLayout->setSpacing(Settings::scale(8));
    previewLayout->setContentsMargins(0, 0, 0, 0);

    font.setPixelSize(Settings::scale(12));
    Label *previewTitle = new Label("PREVIEW", preview);
    previewTitle->setFixedHeight(Settings::scale(16));
    previewTitle->setFont(font);
    previewTitle->setTextColor(Settings::HeaderSecondary);
    previewLayout->addWidget(previewTitle);

    rm->getClient([rmp, preview, previewLayout, this](void *clientPtr){
        profile = new Profile(rmp, *reinterpret_cast<Api::Client *>(clientPtr), preview);
        QObject::connect(profile, &Profile::bannerChanged, [this](QString banner){
            bannerFile = banner;
            profileChanged();
        });
        QObject::connect(profile, &Profile::avatarChanged, [this](QString avatar){
            avatarFile = avatar;
            profileChanged();
        });
        previewLayout->addWidget(profile);
    });

    profileLayout->addWidget(customization);
    profileLayout->addWidget(preview);

    layout->addSpacing(Settings::scale(20));
    layout->addWidget(profileEdit);

    mainLayout->addWidget(container, Settings::scale(10));

    this->setWidget(container);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setStyleSheet("* {border: none; background-color: " + Settings::colors[Settings::BackgroundPrimary].name() + "}"
                        "QScrollBar::handle:vertical {border: none; border-radius: " + QString::number(Settings::scale(2)) + "px; background-color: " + Settings::colors[Settings::BackgroundTertiary].name() + ";}"
                        "QScrollBar:vertical {border: none; background-color: " + Settings::colors[Settings::BackgroundSecondary].name() + "; border-radius: " + QString::number(Settings::scale(8)) + "px; width: " + QString::number(Settings::scale(3)) + "px;}"
                        "QScrollBar::add-line, QScrollBar::sub-line {border:none; background: none; height: 0;}"
                        "QScrollBar:left-arrow:vertical, QScrollBar::right-arrow:vertical {background: none;}"
                        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {background: none;}");


    QObject::connect(defaultColor, &ColorLabel::clicked, [this](){
        customColor->setUnclicked();
        bannerColor = defaultColor->color;
        profile->setBanner(QString(), defaultColor->color);
        profileChanged();
    });
    QObject::connect(customColor, &ColorLabel::clicked, [this](){
        defaultColor->setUnclicked();
        if (customColor->pressed) {
            QString color = QColorDialog::getColor().name();
            bannerColor = color;
            customColor->setColor(color);
            profile->setBanner(QString(), color);
            profileChanged();
        } else {
            bannerColor = customColor->color;
            profile->setBanner(QString(), customColor->color);
            profileChanged();
        }
    });
    QObject::connect(removeAvatar, &SettingsButton::clicked, [this](){
        defaultColor->setColor("#000");
        profile->setAvatar(QString(), "#000");
        avatarFile.clear();
        profileChanged();
    });
    QObject::connect(changeAvatar, &SettingsButton::clicked, [this](){
        QWidget *parentWidget = this;
        while (parentWidget->parent()) parentWidget = (Widget *)parentWidget->parent();

        QFont font;
        font.setPixelSize(Settings::scale(12));
        font.setFamily("whitney");
        Widget *container = new Widget(nullptr);
        container->setFixedSize(Settings::scale(200), Settings::scale(212));
        QHBoxLayout *mainLayout = new QHBoxLayout(container);
        mainLayout->setContentsMargins(Settings::scale(16), 0, Settings::scale(16), Settings::scale(16));
        Label *content = new Label(container);
        content->setFixedSize(Settings::scale(160), Settings::scale(196));
        content->setBackgroundColor(Settings::BackgroundSecondaryAlt);
        QVBoxLayout *layout = new QVBoxLayout(content);
        layout->setSpacing(Settings::scale(32));
        layout->setContentsMargins(Settings::scale(16), Settings::scale(16), Settings::scale(16), Settings::scale(16));
        SelectImageButton *button = new SelectImageButton(content);
        QLabel *desc = new QLabel("Upload Image", content);
        desc->setFont(font);
        layout->addWidget(button);
        layout->addWidget(desc, 0, Qt::AlignHCenter);
        mainLayout->addWidget(content, 0, Qt::AlignCenter);

        PopUp *popUp = new PopUp(container, Settings::scale(240), Settings::scale(280), QString(), "Select Image", false, true, QString(), QString(), QString(), false, false, parentWidget->size(), parentWidget);
        QObject::connect(popUp, &PopUp::cancelled, [popUp](){popUp->deleteLater();});
        QObject::connect(button, &SelectImageButton::clicked, [this, parentWidget, popUp](){
            QString defaultFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/";
            if (defaultFolder.isEmpty()) {
                defaultFolder = ".";
            }
            QString imagePath = QFileDialog::getOpenFileName(nullptr, "Select Image", defaultFolder);
            popUp->deleteLater();
            if (!imagePath.isNull()) {
                avatarFile = imagePath;
                profile->setAvatar(imagePath, QString());
                profileChanged();
            }
        });
    });
}

void UserProfile::profileChanged()
{
    if (!modified) {
        modified = true;

        QFont font;
        font.setPixelSize(Settings::scale(13));
        font.setFamily("whitney");

        Widget *unsaved = new Widget(this);
        unsaved->setFixedHeight(Settings::scale(52));
        unsaved->setMaximumWidth(Settings::scale(700));
        unsaved->setBackgroundColor(Settings::UnsavedBackground);
        unsaved->setBorderRadius(Settings::scale(5));
        QHBoxLayout *layout = new QHBoxLayout(unsaved);
        layout->setSpacing(Settings::scale(10));
        layout->setContentsMargins(Settings::scale(16), Settings::scale(10), Settings::scale(10), Settings::scale(10));
        Label *careful = new Label("Careful - you have unsaved changes!", unsaved);
        careful->setFixedHeight(Settings::scale(20));
        careful->setFont(font);
        careful->setTextColor(Settings::White);
        SettingsButton *reset = new SettingsButton(SettingsButton::Type::NoBackground, "Reset", unsaved);
        SettingsButton *save = new SettingsButton(SettingsButton::Type::Normal, "Save Changes", unsaved);
        layout->addWidget(careful);
        layout->addWidget(reset);
        layout->addWidget(save);

        this->layout->addWidget(unsaved);

        QObject::connect(reset, &SettingsButton::clicked, [this, unsaved](){
            rm->getClient([this, unsaved](void *clientPtr){
                Api::Client client = *reinterpret_cast<Api::Client *>(clientPtr);

                if (client.bannerColor != 0) {
                    customColor->setClicked();
                    customColor->setColor("#" + QString::number(client.bannerColor, 16));
                } else {
                    defaultColor->setClicked();
                    customColor->setColor("#000");
                }
                
                if (client.avatar.isNull()) {
                    defaultColor->setColor("#000");
                } else {
                    defaultColor->setColor(defaultBannerColor.name());
                }

                aboutTextEdit->setPlainText(client.bio);

                profile->setAvatar(client.banner, (client.banner != 0 ? QString::number(client.bannerColor, 16) : defaultBannerColor.name()));
                profile->setAvatar((client.avatar.isNull() ? ":user-icon-asset0.png" : client.avatar + (client.avatar.indexOf("a_") == 0 ? ".gif" : ".png")), QString());

                this->modified = false;
                unsaved->deleteLater();
            });
        });

        QObject::connect(save, &SettingsButton::clicked, [this, unsaved](){
            rm->getClient([this, unsaved](void *clientPtr){
                Api::Client client = *reinterpret_cast<Api::Client *>(clientPtr);

                bool modified = false;
                QString data = "{";

                if (QColor((client.bannerColor & 0x00FF0000) >> 16, (client.bannerColor & 0x0000FF00) >> 8, client.bannerColor & 0x000000FF) != bannerColor) {
                    data += "\"accent_color\":" + QString::number(bannerColor.name().mid(1, 6).toLong(nullptr, 16));
                    modified = true;
                }

                if (client.bio != bio) {
                    data += (modified ? "," : "") + QString("\"bio\":\"") + bio + "\"";
                    modified = true;
                }

                if (client.banner != bannerFile) {
                    QFile image(avatarFile);
                    image.open(QIODevice::OpenModeFlag::ReadOnly);
                    QMimeDatabase db;
                    data += (modified ? "," : "") + QString("\"banner\":\"") + "data:" + db.mimeTypeForFile(avatarFile).name() + ";base64,"
                        + image.readAll().toBase64() + "\"";
                    modified = true;
                }

                if (client.avatar != avatarFile) {
                    QFile image(avatarFile);
                    image.open(QIODevice::OpenModeFlag::ReadOnly);
                    QMimeDatabase db;
                    data += (modified ? "," : "") + QString("\"avatar\":\"") + "data:" + db.mimeTypeForFile(avatarFile).name() + ";base64,"
                        + image.readAll().toBase64() + "\"";
                }

                data += "}";

                rm->requester->changeClient([this, unsaved](void *errorsPtr){
                    if (errorsPtr == nullptr) {
                        this->modified = false;
                        unsaved->deleteLater();
                    } else {
                        QVector<Api::Error *> errors = *reinterpret_cast<QVector<Api::Error *> *>(errorsPtr);

                        Widget *container = new Widget(nullptr);
                        QHBoxLayout *layout = new QHBoxLayout(container);
                        layout->setContentsMargins(Settings::scale(32), Settings::scale(32), Settings::scale(32), Settings::scale(32));
                        layout->setSpacing(Settings::scale(10));
                        
                        QFont font;
                        font.setPixelSize(Settings::scale(14));
                        font.setBold(true);
                        font.setFamily("whitney");

                        for (int i = 0 ; i < errors.size() ; i++) {
                            Label *label = new Label(errors[i]->message, container);
                            label->setFont(font);
                            label->setTextColor(Settings::HeaderSecondary);
                            layout->addWidget(label);
                        }
                        layout->addStretch(1);

                        QWidget *parentWidget = this;
                        while (parentWidget->parent()) parentWidget = (Widget *)parentWidget->parent();

                        PopUp *popUp = new PopUp(container, Settings::scale(400), Settings::scale(300), QString(), "Errors", false, true, QString(), QString(), QString(), false, false, parentWidget->size(), parentWidget);
                        QObject::connect(popUp, &PopUp::cancelled, [popUp](){popUp->deleteLater();});
                    }
                }, data);
            });
        });
    }
}

} // namespace Ui