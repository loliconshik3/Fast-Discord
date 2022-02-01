#pragma once

#include "api/request.h"

#include <QtWebSockets/QtWebSockets>
#include <QJsonValue>

#include <string>
#include <map>

using json = QJsonValue;

namespace Api {

//https://discord.com/developers/docs/topics/opcodes-and-status-codes#gateway-opcodes
enum Opcodes {
    Dispatch,
    Heartbeat,
    Identify,
    PresenceUpdate,
    VoiceStateUpdate,
    Resume = 6,
    Reconnect,
    RequestGuildMembers,
    InvalidSession,
    Hello,
    HeartbeatACK,
    DMChannelOpened = 13,
    GuildChannelOpened
};

//https://discord.com/developers/docs/topics/gateway
class Gateway : public QObject
{
    Q_OBJECT
public:
    Gateway(Api::Requester *requester, const std::string& token);
    void onDispatch(std::function<void(std::string&, json&)> callback);
        //Sets the callback function called when the gateway recieve events

    void sendGuildChannelOpened(const std::map<std::string, std::vector<std::vector<int>>> channels, const std::string& guildId, bool activities, bool threads, bool typing);
    void sendDMChannelOpened(const std::string& channelId);

private slots:
    void closeHandler();
    void identify();
    void processBinaryMessage(const QByteArray& message);
                          // Process a text message that the gateway recieves
    void processTextMessage(const QString& message);
                          // Process a binary message that the gateway recieves

private:
    void start();
    void send(int op, const std::string& data);
                          //Send data through the gateway
    void resume();        //Resume connection
    void heartbeat();     //Send Heartbeat message to stay connected
    void dispatch(std::string eventName, json& data);
                          //Internal function used to process some messages

    QWebSocket client;    // websocket client
    std::function<void(std::string&, json&)> onDispatchHandler;
         //Function called when when the gateway recieve events
    std::string url;      //websocket URL
    std::string sessionId;
    std::string token;
    int heartbeatInterval;
    int seq;
    bool connected;
    bool resuming;
};

} // namespace Api
