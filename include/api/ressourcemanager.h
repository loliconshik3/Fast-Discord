#pragma once

#include "gateway.h"
#include "request.h"
#include "jsonutils.h"
#include "presence.h"
#include "client.h"

#include <QObject>

#include <map>
#include <vector>

namespace Api {

class RessourceManager : public QObject
{
    Q_OBJECT
public:
    RessourceManager(const std::string& token);
    ~RessourceManager();

    void getGuilds(std::function<void(void *)> callback);
    void getGuildChannel(std::function<void(void *)> callback, const std::string& guildId, const std::string& id);
    void getGuildChannels(std::function<void(void *)> callback, const std::string& id);
    void getPrivateChannel(std::function<void(void *)> callback, const std::string& id);
    void getPrivateChannels(std::function<void(void *)> callback);
    void getMessages(std::function<void(void *)> callback, const std::string& channelId, unsigned int limit);
    void getClient(std::function<void(void *)> callback);
    void getClientSettings(std::function<void(void *)> callback);
    void getImage(std::function<void(void *)> callback, const std::string& url, const std::string& fileName);
    void getUser(std::function<void(void *)> callback, const std::string& userId);
    void getPresences(std::function<void(void *)> callback);

    Requester *requester;

signals:
    void typingReceived();
    void guildsReceived(const std::vector<Api::Guild *>&);
    void presencesReceived(const std::vector<Api::Presence *>&);
    void privateChannelsReceived(const std::vector<Api::PrivateChannel *>&);
    void unreadUpdateReceived(const std::string&);
    void presenceReceived(const Api::Presence&);
    void messageReceived(const Api::Message&);

private:
    void gatewayDispatchHandler(std::string& eventName, json& data);
        // Event handler for the gateway

    Gateway *gw;

    std::map<std::string, std::vector<Message *>> *messages;
    std::map<std::string, std::vector<Channel *>> *guildsChannels;

    std::vector<User *>           *users;
    std::vector<Guild *>          *guilds;
    std::vector<PrivateChannel *> *privateChannels;
    std::vector<Presence *>       *presences;

    Client         *client = nullptr;
    ClientSettings *clientSettings = nullptr;
};

} // namespace Api