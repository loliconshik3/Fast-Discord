#include "api/gateway.h"

#include "api/jsonutils.h"

#include <QThread>

namespace Api {

Gateway::Gateway(Api::Requester *requester, const std::string& tokenp)
    : QObject()
{
    // Member initialization
    seq = 0;
    connected = false;
    resuming = false;
    token = tokenp;

    // Getting the websocket URL
    requester->requestApi({
        [this](void *urlp) {
            url = *static_cast<std::string *>(urlp);
            this->start();
        },
        "https://discord.com/api/v9/gateway",
        "",
        "",
        "",
        "",
        GetWsUrl,
        false
    });
}

// Set the handlers and connect
void Gateway::start()
{
    QObject::connect(&client, SIGNAL(aboutToClose()), this, SLOT(closeHandler()));
    QObject::connect(&client, SIGNAL(connected()), this, SLOT(identify()));
    QObject::connect(&client, SIGNAL(binaryMessageReceived(const QByteArray&)), this, SLOT(processBinaryMessage(const QByteArray&)));
    QObject::connect(&client, SIGNAL(textMessageReceived(const QString&)), this, SLOT(processTextMessage(const QString&)));

    client.open(QUrl(QString(url.c_str()) + "/" + QString("?v=9&encoding=json")));
}

void Gateway::closeHandler()
{
    // Reconnect when the connection is closed
    if (connected) {
        connected = false;
        client.open(QUrl(QString(url.c_str()) + QString("?v=9&encoding=json")));
    }
}

// Set the callback function called when the gateway recieve events
void Gateway::onDispatch(std::function<void(std::string&, json&)> callback)
{
    onDispatchHandler = callback;
}

void Gateway::sendGuildChannelOpened(const std::map<std::string, std::vector<std::vector<int>>> channels, const std::string& guildId, bool activities, bool threads, bool typing)
{
    std::string data = "{\"guild_id\":\"" + guildId + "\"" + (typing ? ",\"typing\":true" : "") + (activities ? ",\"activities\":true" : "") + (threads ? ",\"threads\":true" : "") + ",\"channels\":{";

    unsigned int counter = 0;
    for (auto it = channels.begin() ; it != channels.end() ; it++, counter++) {
        data += std::string(counter > 0 ? "," : "") + "\"" + it->first + "\":[";
        std::vector<std::vector<int>> messagesNumbers = it->second;
        for (unsigned int i = 0 ; i < messagesNumbers.size() ; i++) {
            data += "[";
            for (unsigned int j = 0 ; j < messagesNumbers[i].size() ; j++) {
                data += std::to_string(messagesNumbers[i][j]) + (j + 1 < messagesNumbers[i].size() ? "," : "");
            }
            data += "]" + std::string(i + 1 < messagesNumbers.size() ? "," : "");
        }
        data += "]";
    }

    send(GuildChannelOpened, data + "}}");
}

void Gateway::sendDMChannelOpened(const std::string& channelId)
{
    send(DMChannelOpened, "{\"channel_id\":\"" + channelId + "\"}");
}

// Send data through the gateway
void Gateway::send(int op, const std::string& data)
{
    //qDebug() << "⇧" << data.substr(0, 198).c_str();
    // Build the payload string
    std::string payload = "{\"op\":" + std::to_string(op) + ",\"d\":" + data + "}";
    // Send the message
    client.sendTextMessage(QString(payload.c_str()));
}

// Process a binary message that the gateway recieves
void Gateway::processBinaryMessage(const QByteArray& message)
{
    //qDebug() << "⇩" << qUncompress(message).mid(0, 198);
    QJsonDocument payload = QJsonDocument::fromJson(qUncompress(message));
    QJsonValue data = payload["d"];

    switch (payload["op"].toInt(-1)) {
        case Dispatch: //Event recieved
            seq = payload["s"].toInt();
            dispatch(payload["t"].toString().toUtf8().constData(), data);
            break;
        case Reconnect:
            resume();
            break;
        case InvalidSession:
            identify();
            break;
        case Hello:
            connected = true;

            // Start Heartbeating
            heartbeatInterval = data["heartbeat_interval"].toInt(45000);
            QThread *heartbeatThread = QThread::create([this](){
                while (connected) {
                    send(1, std::to_string(seq));

                    QThread::msleep(heartbeatInterval);
                }
            });
            heartbeatThread->start();

            if (resuming) {
                resume();
            } else {
                identify();
                resuming = true;
            }
            break;
    }
}

// Process a text message that the gateway recieves
void Gateway::processTextMessage(const QString& message)
{
    //qDebug() << "⇩" << message.mid(0, 198);
    QJsonDocument payload = QJsonDocument::fromJson(message.toUtf8());
    QJsonValue data = payload["d"];

    switch (payload["op"].toInt(-1)) {
        case Dispatch: //Event recieved
            seq = payload["s"].toInt();
            dispatch(payload["t"].toString().toUtf8().constData(), data);
            break;
        case Reconnect:
            resume();
            break;
        case InvalidSession:
            identify();
            break;
        case Hello:
            connected = true;

            // Start Heartbeating
            heartbeatInterval = data["heartbeat_interval"].toInt(45000);
            QThread *heartbeatThread = QThread::create([this](){
                while (connected) {
                    send(1, std::to_string(seq));

                    QThread::msleep(heartbeatInterval);
                }
            });
            heartbeatThread->start();

            if (resuming) {
                resume();
            } else {
                //identify();
                resuming = true;
            }
            break;
    }
}

// Identifying to the gateway
void Gateway::identify()
{
    send(Identify, "{\"token\":\"" + token +"\","
        "\"capabilities\":125,"
        "\"properties\":{"
            "\"os\":\"Linux\","
            "\"browser\":\"Chrome\","
            "\"device\":\"\","
            "\"system_locale\":\"fr\","
            "\"browser_user_agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.4577.63 \"," // Random generated User-Agent
            "\"browser_version\":\"93.0\","
            "\"os_version\":\"\","
            "\"referrer\":\"\","
            "\"referring_domain\":\"\","
            "\"referrer_current\":\"\","
            "\"referring_domain_current\":\"\","
            "\"release_channel\":\"stable\","
            "\"client_event_source\":null"
        "},"
        "\"presence\":{"
            "\"status\":\"online\","
            "\"since\":0,"
            "\"activities\":[],"
            "\"afk\":false"
        "},"
        "\"compress\":false,"
        "\"client_state\":{"
            "\"guild_hashes\":{},"
            "\"highest_last_message_id\":\"0\","
            "\"read_state_version\":0,"
            "\"user_guild_settings_version\":-1"
        "}}");
}

// Resume connection
void Gateway::resume()
{
    send(Resume, "{\"token\":\""+ token +"\","
                 "\"session_id\":,"
                 "\"seq\":" + std::to_string(seq) + "}");
}

// Internal function used to process some messages
void Gateway::dispatch(std::string eventName, json& data)
{
    if (eventName == "READY") {
        sessionId = data["session_id"].toString().toUtf8().constData();
    }
    onDispatchHandler(eventName, data);
}

} // namespace Api
