#include "api/ressourcemanager.h"

#include <QFile>

#include <fstream>

namespace Api {

RessourceManager::RessourceManager(const std::string& token)
{
    requester = new Requester(token);
    gw = new Gateway(requester, token);

    messages = new std::map<std::string, std::vector<Message *>>();
    guildsChannels = new std::map<std::string, std::vector<Channel *>>();

    // Set the gateway event callback
    gw->onDispatch([&](std::string& eventName, json& data){gatewayDispatchHandler(eventName, data);});
}

void RessourceManager::gatewayDispatchHandler(std::string& eventName, json& data)
{
    // Process gateway events
    if (eventName == "READY") {
        Api::unmarshalMultiple<Api::Guild>(data["guilds"].toArray(), &guilds);
        emit guildsReceived(*guilds);

        Api::unmarshalMultiple<Api::PrivateChannel>(data["private_channels"].toArray(), &privateChannels);
        Api::unmarshalMultiple<Api::User>(data["users"].toArray(), &users);
        emit privateChannelsReceived(*privateChannels);
    } else if (eventName == "READY_SUPPLEMENTAL") {
        Api::unmarshalMultiple<Api::Presence>(data["merged_presences"]["friends"].toArray(), &presences);
        emit presencesReceived(*presences);
    } else if (eventName == "CHANNEL_CREATE") {
        Api::Channel *channel;
        Api::unmarshal<Api::Channel>(data.toObject(), &channel);
        if (channel->type == DM || channel->type == GroupDM) {
            std::vector<std::string> *recipients = nullptr;
            if (channel->recipients != nullptr) {
                recipients = new std::vector<std::string>();
                for (unsigned int i = 0 ; i < channel->recipients->size() ; i++) {
                    users->push_back((*channel->recipients)[i]);
                    recipients->push_back(*(*channel->recipients)[i]->id);
                }
            }

            PrivateChannel *privateChannel = new PrivateChannel {
                recipients,
                channel->icon,
                channel->id,
                channel->lastMessageId,
                channel->name,
                channel->ownerId,
                channel->type
            };
            privateChannels->push_back(privateChannel);

            emit channelCreated(nullptr, privateChannel);
        } else {
            (*guildsChannels)[*channel->guildId].push_back(channel);

            emit channelCreated(channel, nullptr);
        }
    } else if (eventName == "CHANNEL_UPDATE") {
        Api::Channel *channel;
        Api::unmarshal<Api::Channel>(data.toObject(), &channel);
        if (channel->type == DM || channel->type == GroupDM) {
            std::vector<std::string> *recipients = nullptr;
            if (channel->recipients != nullptr) {
                recipients = new std::vector<std::string>();
                for (unsigned int i = 0 ; i < channel->recipients->size() ; i++) {
                    users->push_back((*channel->recipients)[i]);
                    recipients->push_back(*(*channel->recipients)[i]->id);
                }
            }

            for (auto it = privateChannels->begin() ; it != privateChannels->end() ; it++) {
                if (*(*it)->id == *channel->id) {
                    privateChannels->erase(it);
                    break;
                }
            }

            PrivateChannel *privateChannel = new PrivateChannel {
                recipients,
                channel->icon,
                channel->id,
                channel->lastMessageId,
                channel->name,
                channel->ownerId,
                channel->type
            };
            privateChannels->push_back(privateChannel);

            emit channelUpdated(nullptr, privateChannel);
        } else {
            for (auto it = (*guildsChannels)[*channel->guildId].begin() ; it != (*guildsChannels)[*channel->guildId].end() ; it++) {
                if (*(*it)->id == *channel->id) {
                    (*guildsChannels)[*channel->guildId].erase(it);
                    break;
                }
            }
            (*guildsChannels)[*channel->guildId].push_back(channel);

            emit channelUpdated(channel, nullptr);
        }
    } else if (eventName == "CHANNEL_DELETE") {
        Api::Channel *channel;
        Api::unmarshal<Api::Channel>(data.toObject(), &channel);
        if (channel->type == DM || channel->type == GroupDM) {
            for (auto it = privateChannels->begin() ; it != privateChannels->end() ; it++) {
                if (*(*it)->id == *channel->id) {
                    privateChannels->erase(it);
                    break;
                }
            }

            emit channelDeleted(*channel->id, "", channel->type);
        } else {
            for (auto it = (*guildsChannels)[*channel->guildId].begin() ; it != (*guildsChannels)[*channel->guildId].end() ; it++) {
                if (*(*it)->id == *channel->id) {
                    (*guildsChannels)[*channel->guildId].erase(it);
                    break;
                }
            }

            emit channelDeleted(*channel->id, *channel->guildId, channel->type);
        }
    } else if (eventName == "CHANNEL_UNREAD_UPDATE") {
        emit unreadUpdateReceived(data["guild_id"].toString().toUtf8().constData());
    } else if (eventName == "MESSAGE_CREATE") {
        // We received a message
        Api::Message *message;
        Api::unmarshal<Api::Message>(data.toObject(), &message);
        if (*message->author->id != *client->id) emit messageReceived(*message);
    } else if (eventName == "PRESENCE_UPDATE") {
        Api::Presence *presence;
        Api::unmarshal<Api::Presence>(data.toObject(), &presence);
        emit presenceReceived(*presence);
    } else if (eventName == "TYPING_START") {
        // Someone is typing
        /*gatewayData = data;
        QThread *typingThread = QThread::create([this](){rightColumn->userTyping(gatewayData);});
        typingThread->start();*/
    }
}

void RessourceManager::getGuilds(std::function<void(void *)> callback)
{
    if (guilds == nullptr || guilds->empty())
        requester->getGuilds([&, callback](void *guildsPtr) {
            guilds = reinterpret_cast<std::vector<Guild *> *>(guildsPtr);
            callback(guildsPtr);
        });
    else
        callback(reinterpret_cast<void *>(guilds));
}

void RessourceManager::getGuildChannels(std::function<void(void *)> callback, const std::string& id)
{
    openedGuildsChannels[id];
    if (guildsChannels->find(id) == guildsChannels->end()) {
        requester->getGuildChannels([&, callback](void *guildChannelsPtr) {
            (*guildsChannels)[id] = *reinterpret_cast<std::vector<Channel *> *>(guildChannelsPtr);
            callback(guildChannelsPtr);
        }, id);
    } else {
        callback(reinterpret_cast<void *>(&(*guildsChannels)[id]));
    }
}

void RessourceManager::getGuildChannel(std::function<void(void *)> callback, const std::string& guildId, const std::string& id)
{
    openedGuildsChannels[guildId][id];

    if (guildsChannels->find(guildId) == guildsChannels->end()) {
        requester->getGuildChannels([&, callback](void *guildChannelsPtr) {
            (*guildsChannels)[guildId] = *reinterpret_cast<std::vector<Channel *> *>(guildChannelsPtr);
            callback(guildChannelsPtr);
        }, guildId);
    } else {
        for (unsigned int i = 0 ; i < (*guildsChannels)[guildId].size() ; i++) {
            if (*(*guildsChannels)[guildId][i]->id == id) callback(reinterpret_cast<void *>((*guildsChannels)[guildId][i]));
        }
    }
}

void RessourceManager::getPrivateChannel(std::function<void(void *)> callback, const std::string& id)
{
    if (privateChannels == nullptr || privateChannels->empty()) {
        requester->getPrivateChannels([&, callback](void *privateChannelsPtr) {
            privateChannels = reinterpret_cast<std::vector<PrivateChannel *> *>(privateChannelsPtr);
            for (unsigned int i = 0 ; i < privateChannels->size() ; i++) {
                if (*(*privateChannels)[i]->id == id) callback(reinterpret_cast<void *>((*privateChannels)[i]));
            }
        });
        return;
    }
    for (unsigned int i = 0 ; i < privateChannels->size() ; i++) {
        if (*(*privateChannels)[i]->id == id) callback(reinterpret_cast<void *>((*privateChannels)[i]));
    }
}

void RessourceManager::getPrivateChannels(std::function<void(void *)> callback)
{
    if (privateChannels == nullptr || privateChannels->empty())
        requester->getPrivateChannels([&, callback](void *privateChannelsPtr) {
            privateChannels = reinterpret_cast<std::vector<PrivateChannel *> *>(privateChannelsPtr);
            callback(privateChannelsPtr);
        });
    else
        callback(reinterpret_cast<void *>(privateChannels));
}

void RessourceManager::getMessages(std::function<void(void *)> callback, const std::string& channelId, unsigned int limit, bool newMessages)
{
    bool found = false;
    for (auto it = openedGuildsChannels.begin() ; it != openedGuildsChannels.end() ; it++) {
        std::map<std::string, std::vector<std::vector<int>>> channels = it->second;
        if (channels.find(channelId) != channels.end()) {
            found = true;
            unsigned int size = (*messages)[channelId].size();

            std::vector<int> indexes;
            indexes.push_back(size);
            indexes.push_back(size + limit);
            channels[channelId].push_back(indexes);
            gw->sendGuildChannelOpened(channels, it->first, true, true, true);
            break;
        }
    }
    if (!found) {
        gw->sendDMChannelOpened(channelId);
    }

    if (newMessages) {
        if ((*messages)[channelId].size() >= 50)
            requester->getMessages([&, callback](void *messagesPtr) {
                std::vector<Message *> messagesVector = *reinterpret_cast<std::vector<Message *> *>(messagesPtr);
                if (messagesVector.size() > 0) {
                    for (unsigned int i = 0 ; i < messagesVector.size() ; i++)
                        (*messages)[channelId].push_back(messagesVector[i]);
                    callback(messagesPtr);
                }
            }, channelId, *(*messages)[channelId].back()->id, limit);
    } else if (messages->find(channelId) == messages->end() || (*messages)[channelId].size() == 0) {
        requester->getMessages([&, callback](void *messagesPtr) {
            (*messages)[channelId] = *reinterpret_cast<std::vector<Message *> *>(messagesPtr);
            callback(messagesPtr);
        }, channelId, "", limit);
    } else if ((*messages)[channelId].size() < 50) {
        callback(reinterpret_cast<void *>(&(*messages)[channelId]));
    } else if ((*messages)[channelId].size() < limit) {
        requester->getMessages([&, callback](void *messagesPtr) {
            std::vector<Message *> messagesVector = *reinterpret_cast<std::vector<Message *> *>(messagesPtr);
            for (unsigned int i = 0 ; i < messagesVector.size() ; i++)
                (*messages)[channelId].push_back(messagesVector[i]);
            callback(reinterpret_cast<void *>(&(*messages)[channelId]));
        }, channelId, *(*messages)[channelId].back()->id, limit - (*messages)[channelId].size());
    } else {
        callback(reinterpret_cast<void *>(&(*messages)[channelId]));
    }
}

void RessourceManager::getClient(std::function<void(void *)> callback)
{
    if (client == nullptr)
        requester->getClient([&, callback](void *clientPtr) {
            client = reinterpret_cast<Client *>(clientPtr);
            callback(clientPtr);
        });
    else
        callback(reinterpret_cast<void *>(client));
}

void RessourceManager::getClientSettings(std::function<void(void *)> callback)
{
    if (clientSettings == nullptr)
        requester->getClientSettings([&, callback](void *clientSettingsPtr) {
           clientSettings = reinterpret_cast<ClientSettings *>(clientSettingsPtr);
           callback(clientSettingsPtr);
       });
    else
        callback(reinterpret_cast<void *>(clientSettings));
}

void RessourceManager::getImage(std::function<void(void *)> callback, const std::string& url, const std::string& fileName)
{
    if (!std::ifstream(("cache/" + fileName).c_str()).good()) {
        requester->getImage(callback, url, fileName);}
    else {
        std::string path = "cache/" + fileName;
        callback(reinterpret_cast<void *>(const_cast<std::string *>(&path)));
    }
}

void RessourceManager::getUser(std::function<void(void *)> callback, const std::string& userId)
{
    for (unsigned int i = 0 ; i < users->size() ; i++) {
        if (*(*users)[i]->id == userId) {
            callback(reinterpret_cast<void *>((*users)[i]));
            return;
        }
    }
    requester->getUser([&, callback](void *userPtr) {
       users->push_back(reinterpret_cast<User *>(userPtr));
       callback(userPtr);
   }, userId);
}

void RessourceManager::getPresences(std::function<void(void *)> callback)
{
    callback(reinterpret_cast<void *>(presences));
}


std::vector<Api::Message *> RessourceManager::getAllMessages(std::string& channelId)
{
    return (*messages)[channelId];
}

bool RessourceManager::hasMessages(const std::string& channelId)
{
    return messages->find(channelId) == messages->end();
}

void RessourceManager::pushFrontMessage(const std::string& channelId, Api::Message *message)
{
    (*messages)[channelId].insert((*messages)[channelId].cbegin(), message);
}


RessourceManager::~RessourceManager()
{
    delete requester;
    delete gw;
}

} // namespace Api
