#include "ui/middlecolumn.h"

#include "ui/usermenu.h"

#include <fstream>

namespace Ui {

MiddleColumn::MiddleColumn(Api::Requester *requesterp, const Api::Client *client, QWidget *parent)
    : QWidget(parent)
{
    // Set the requester
    requester = requesterp;

    // Create the layout
    layout = new QVBoxLayout(this);

    // Create and style the channel list
    channelList = new QScrollArea(this);
    channelList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Add the widget and style the main layout
    layout->addWidget(channelList);
    layout->addWidget(new UserMenu(requester, client, this));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QObject::connect(this, SIGNAL(privateChannelsRecieved(std::vector<Api::Channel *> *)), this, SLOT(setPrivateChannels(std::vector<Api::Channel *> *)));
    QObject::connect(this, SIGNAL(guildChannelsRecieved(std::vector<Api::Channel *> *)), this, SLOT(setGuildChannels(std::vector<Api::Channel *> *)));

    // Display private channels (we are on the home page)
    this->displayPrivateChannels();

    // Style this column
    this->setFixedWidth(240);
    this->setStyleSheet("background-color: #2F3136;"
                        "border: none;");
}

void MiddleColumn::setPrivateChannels(std::vector<Api::Channel *> *channels)
{
    // Set the private channels
    privateChannels = channels;

    // Create the widgets
    QWidget *privateChannelList = new QWidget(this);
    QVBoxLayout *privateChannelListLayout = new QVBoxLayout(privateChannelList);

    // Create and display the private channels
    for (unsigned int i = 0 ; i < privateChannels->size() ; i++) {
        PrivateChannel *privateChannel = new PrivateChannel(requester, *(*privateChannels)[i], i, privateChannelList);
        privateChannelWidgets.push_back(privateChannel);
        privateChannelListLayout->insertWidget(i, privateChannel);
        QObject::connect(privateChannel, SIGNAL(leftClicked(Api::Channel&, unsigned int)), this, SLOT(clicChannel(Api::Channel&, unsigned int)));
    }
    privateChannelListLayout->insertStretch(-1, 1);
    privateChannelListLayout->setSpacing(2);
    privateChannelListLayout->setContentsMargins(8, 8, 8, 0);

    // Set the channels to the column
    channelList->setWidget(privateChannelList);
}

void MiddleColumn::setGuildChannels(std::vector<Api::Channel *> *channelsp)
{
    // Clear the whannels
    guildChannelWidgets.clear();
    channelList->takeWidget();

    // Create widgets
    QWidget *guildChannelList = new QWidget(this);
    QVBoxLayout *guildChannelListLayout = new QVBoxLayout(guildChannelList);

    // Set the channels
    std::vector<Api::Channel *> channels = *channelsp;

    // Create the channels widgets

    size_t channelsLen = channels.size();
    unsigned int count = 0; // For the IDs of the channels
    // Loop to find channel that are not in a category
    for (size_t i = 0 ; i < channelsLen ; i++) {
        if ((*channels[i]).type != Api::GuildCategory && (*channels[i]).parentId == nullptr) {
            // Create and add the channel widget to the list
            GuildChannelWidget *channelWidget = new GuildChannelWidget(*channels[i], count, guildChannelList);
            guildChannelListLayout->addWidget(channelWidget);
            guildChannelWidgets.push_back(channelWidget);
            count++;
        }
    }
    // Loop through all channels to create widgets
    for (size_t i = 0 ; i < channelsLen ; i++) {
        if ((*channels[i]).type == Api::GuildCategory) {
            // Create the category channel channel widget
            GuildChannelWidget *channelWidget = new GuildChannelWidget(*channels[i], count, guildChannelList);
            guildChannelListLayout->addWidget(channelWidget);
            guildChannelWidgets.push_back(channelWidget);
            count++;
            // Loop another time to find channels belonging to this category
            for (size_t j = 0 ; j < channelsLen ; j++) {
                if ((*channels[j]).parentId == nullptr) continue;
                    // Category or 'orphan' channel
                if (*(*channels[j]).parentId == *(*channels[i]).id) {
                    // This channel belongs to the category
                    // Create and add the channel widget
                    GuildChannelWidget *channelWidget = new GuildChannelWidget(*channels[j], count, guildChannelList);
                    guildChannelWidgets.push_back(channelWidget);
                    guildChannelListLayout->addWidget(channelWidget);
                    count++;

                    // Connect the clicked signal to open the channel
                    QObject::connect(channelWidget, SIGNAL(leftClicked(Api::Channel&, unsigned int)), this, SLOT(clicChannel(Api::Channel&, unsigned int)));
                }
            }
        }
    }
    guildChannelListLayout->insertStretch(-1, 1);
    guildChannelListLayout->setSpacing(2);
    guildChannelListLayout->setContentsMargins(8, 8, 8, 0);

    // Style the channel list
    channelList->setWidget(guildChannelList);
    channelList->setStyleSheet("* {background-color: #2f3136; border: none;}"
                               "QScrollBar::handle:vertical {border: none; border-radius: 2px; background-color: #202225;}"
                               "QScrollBar:vertical {border: none; background-color: #2F3136; border-radius: 8px; width: 3px;}"
                               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {border:none; background: none; height: 0;}");
}

void MiddleColumn::clicChannel(Api::Channel& channel, unsigned int id)
{
    // Reset the stylesheet of the channels except the one that we just clicked
    if (channel.type == Api::DM || channel.type == Api::GroupDM) {
        for (size_t i = 0 ; i < privateChannelWidgets.size() ; i++) {
            if (i != id) {
                privateChannelWidgets[i]->unclicked();
            }
        }
    } else {
        for (size_t i = 0 ; i < guildChannelWidgets.size() ; i++) {
            if (i != id) {
                guildChannelWidgets[i]->unclicked();
            }
        }
    }

    // Emit the signal to open the channel
    emit channelClicked(channel);
}

void MiddleColumn::displayPrivateChannels()
{
    // Request private channels
    requester->getPrivateChannels([this](void *channels) {emit privateChannelsRecieved(static_cast<std::vector<Api::Channel *> *>(channels));});
}

void MiddleColumn::openGuild(Api::Guild& guild)
{
    // Request the channels of the guild
    requester->getGuildChannels([this](void *channels) {emit guildChannelsRecieved(static_cast<std::vector<Api::Channel *> *>(channels));}, *guild.id);
}

} // namespace Ui
