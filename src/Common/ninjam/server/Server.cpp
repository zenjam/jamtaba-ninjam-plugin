#include "Server.h"

#include <QDebug>
#include <QDataStream>
#include <QRegularExpression>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTcpServer>

#include "ninjam/Ninjam.h"
#include "ninjam/common/CommonMessages.h"
#include "ninjam/client/ServerMessages.h"
#include "ninjam/client/ClientMessages.h"
#include "ninjam/client/User.h"
#include "ninjam/client/UserChannel.h"

using ninjam::common::KeepAliveMessage;
using ninjam::server::Server;
using ninjam::server::Voting;
using ninjam::client::AuthChallengeMessage;     // TODO message used both in server and client
using ninjam::client::ClientAuthUserMessage;    // todo message used both in server and client
using ninjam::client::ClientSetChannel;         // used in both
using ninjam::client::AuthReplyMessage;         // used in both client and server
using ninjam::client::ConfigChangeNotifyMessage;// used in both
using ninjam::client::UserInfoChangeNotifyMessage; // used in both
using ninjam::client::ServerToClientChatMessage;        // used in both
using ninjam::client::DownloadIntervalBegin;    // BOTH
using ninjam::client::DownloadIntervalWrite;    //BOTH
using ninjam::client::UploadIntervalBegin;      // BOTH
using ninjam::client::UploadIntervalWrite;      //BOTH
using ninjam::client::ClientToServerChatMessage;
using ninjam::client::ClientSetUserMask;
using ninjam::client::UserChannel;              // used in both
using ninjam::client::User;                     // both
using ninjam::server::RemoteUser;
using ninjam::MessageHeader;
using ninjam::MessageType;

enum AdminCommand
{
    Invalid,
    Topic,
    Bpi,
    Bpm,
    kick
};

AdminCommand getAdminCommand(const QString &cmd)
{
    QString command = cmd.split(" ").first();

    if (command == "topic")
        return AdminCommand::Topic;

    if (command == "bpi")
        return AdminCommand::Bpi;

    if (command == "bpm")
        return AdminCommand::Bpm;

    if (command == "kick")
        return AdminCommand::kick;

    return AdminCommand::Invalid;
}

RemoteUser::RemoteUser() :
    currentHeader(MessageHeader()),
    lastKeepAliveReceived(QDateTime::currentMSecsSinceEpoch()),
    receivedServerInfos(false)
{

}

void RemoteUser::updateChannels(const QList<UserChannel> &newChannels, quint8 maxChannels)
{
    QSet<quint8> updatedIndexes;

    for (const UserChannel &channel : newChannels) {
        quint8 index = channel.getIndex();
        channels.insert(index, channel);
        updatedIndexes.insert(index);
    }

    // deactivate non updated channels
    for (auto& channel : channels) {
        quint8 index = channel.getIndex();
        if (!updatedIndexes.contains(index)) {
            channel.setActive(false);
            channel.setName(QString());
        }
    }

    // keep just 'maxChannels'
    while (channels.size() > maxChannels) {
        channels.pop_back();
    }
}

void RemoteUser::setFullName(const QString &fullName)
{
    this->fullName = fullName;
    this->name = ninjam::client::extractUserName(fullName);
    this->ip = ninjam::client::extractUserIP(fullName);
}

void RemoteUser::setLastKeepAliveToNow()
{
    lastKeepAliveReceived = QDateTime::currentMSecsSinceEpoch();
}

// -------------------------------------------------------------

Voting::Voting(QObject *parent) :
    QObject(parent),
    value(0),
    timer(nullptr),
    requiredVotes(0)
{

}

Voting::~Voting()
{
    //qDebug() << "Destructing voting " << value;
}

bool Voting::isRunning() const
{
    return timer && timer->isActive();
}

void Voting::start(quint16 newValue, quint8 requiredVotes, quint64 expiration)
{
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setTimerType(Qt::PreciseTimer);

        connect(timer, &QTimer::timeout, this, [&](){
            emit expired(value);
            reset();
        });
    }

    if (isRunning())
        return;

    reset();

    this->requiredVotes = requiredVotes;
    this->value = newValue;
    timer->setInterval(expiration);
    timer->start();
}

void Voting::registerVote(const QString &userFullName, quint16 value)
{
    if (!isRunning())
        return;

    if (voters.contains(userFullName))
        return; // discard duplicated vote

    if (value != this->value)
        return; // user voting using wrong bpm/bpi value

    voters.insert(userFullName);

    auto voteCount = voters.count();

    if (voteCount >= requiredVotes) {
        emit accepted(value);
        timer->stop();
        reset();
    }
    else {
        auto expirationTime = timer->interval();
        emit incremented(value, voteCount, requiredVotes, expirationTime);
    }
}

void Voting::reset()
{
    voters.clear();
    value = 0;
    requiredVotes = 0;
}

// -------------------------------------------------------------


Server::Server() :
    bpm(120),
    bpi(16),
    topic("No topic!"),
    licence("No licence at moment!"),
    maxChannels(2),
    maxUsers(4),
    keepAlivePeriod(30),
    votingSettings({0.6, 10000}) // 60% for threshold, 60 seconds to vote expiration
{
    connect(&tcpServer, &QTcpServer::newConnection, this, &Server::handleNewConnection);
    connect(&tcpServer, &QTcpServer::acceptError, this, &Server::handleAcceptError);
}

Server::~Server()
{
    shutdown();
}

void Server::bpiVotingIncremented(quint16 votingValue, quint16 currentVotes, quint16 requiredVotes, quint64 expirationTime)
{
    //[voting system] leading candidate: 1/3 votes for 8 BPI [each vote expires in 20s]

    auto msg = QString("leading candidate: %1/%2 votes for %3 BPI [each vote expires in %4s]")
            .arg(currentVotes)
            .arg(requiredVotes)
            .arg(votingValue)
            .arg(expirationTime/1000); // expiration time in seconds

    broadcastVotingSystemMessage(msg);
}

void Server::bpiVotingAccepted(quint16 acceptedValue)
{
    if (acceptedValue != bpi) {// maybe admin changed the value before voting is finished?
        setBpi(acceptedValue);
        broadcastVotingSystemMessage(QString("setting BPM to %1").arg(acceptedValue));
    }
}

void Server::bpiVotingExpired(quint16 bpiValue)
{
    if (bpiVotings.contains(bpiValue)) {
        bpiVotings[bpiValue]->deleteLater();
        bpiVotings.remove(bpiValue);
    }
}

void Server::bpmVotingIncremented(quint16 votingValue, quint16 currentVotes, quint16 requiredVotes, quint64 expirationTime)
{
    //[voting system] leading candidate: 1/3 votes for 8 BPM [each vote expires in 20s]

    auto msg = QString("leading candidate: %1/%2 votes for %3 BPM [each vote expires in %4s]")
            .arg(currentVotes)
            .arg(requiredVotes)
            .arg(votingValue)
            .arg(expirationTime/1000); // expiration time in seconds

    broadcastVotingSystemMessage(msg);
}

void Server::bpmVotingAccepted(quint16 acceptedValue)
{
    if (acceptedValue != bpm) {  // maybe admin changed the value before voting is finished?
        setBpm(acceptedValue);
        broadcastVotingSystemMessage(QString("setting BPI to %1").arg(acceptedValue));
    }
}

void Server::bpmVotingExpired(quint16 bpmValue)
{
    if (bpmVotings.contains(bpmValue)) {
        bpmVotings[bpmValue]->deleteLater();
        bpmVotings.remove(bpmValue);
    }
}

bool Server::isStarted() const
{
    return tcpServer.isListening();
}

QHostAddress Server::getBestHostAddress()
{
    return QHostAddress::AnyIPv4;
}

void Server::start(quint16 port)
{
    shutdown();

    QHostAddress address = Server::getBestHostAddress();
    bool listening = tcpServer.listen(address, port);
    if (listening)
        emit serverStarted();
    else
        emit errorStartingServer(tcpServer.errorString());
}

void Server::handleNewConnection()
{
    QTcpSocket *socket = tcpServer.nextPendingConnection();
    if (!socket)
        return;

    if (remoteUsers.size() >= maxUsers) {
        socket->disconnectFromHost(); // reject the connection
        return;
    }

    emit incommingConnection(socket->peerAddress().toString());

    connect(socket, &QTcpSocket::disconnected, this, &Server::handleDisconnection);
    connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &Server::handleClientSocketError);
    connect(socket, &QIODevice::readyRead, this, &Server::processReceivedBytes);

    connect(socket, &QTcpSocket::bytesWritten, [&](qint64 bytes){
        totalUploadMeasurer.addTransferedBytes(bytes);
    });

    remoteUsers.insert(socket, RemoteUser());

    sendAuthChallenge(socket);
}

void Server::sendAuthChallenge(QTcpSocket *device)
{
    QByteArray challenge("abcdabcd");
    quint32 protocolVersion = 0x00020000; // fixed value
    quint32 serverCapabilities = keepAlivePeriod << 8; // keep alive period value is stored in bytes 8-15
    if(!licence.isEmpty())
        serverCapabilities |= 1; // when server has licence the first bit is set.

    auto msg = AuthChallengeMessage(challenge, licence, serverCapabilities, protocolVersion);
    msg.serializeToDevice(device);
}

void Server::processClientAuthUserMessage(QTcpSocket *socket, const ClientAuthUserMessage& msg)
{
    // ignoring challenge and password for while

    quint8 flag = 1; // authentication suceeded
    QString newUserName(generateUniqueUserName(msg.getUserName())); // updated user name or error message;
    newUserName += "@" + socket->peerAddress().toString();

    remoteUsers[socket].setFullName(newUserName);

    AuthReplyMessage authReply(flag, newUserName, maxChannels);
    authReply.serializeToDevice(socket);

    if (authReply.userIsAuthenticated()) {
        broadcastNetworkMessage(ServerToClientChatMessage::buildUserJoinMessage(newUserName), socket);

        emit userEntered(newUserName);
    } else {
        disconnectClient(socket);
    }
}

QString Server::generateUniqueUserName(const QString &userName) const
{
    static const QString ANON_PREFIX("anonymous:");
    static const QString SPECIAL_SYMBOLS("[^a-zA-Z0-9 _-]"); // allowing lower and upper case letters, numbers, _ and - symbols, blank space, at least 1 character and max 16 characters.
    static const quint8 MAX_NAME_SIZE = 16;

    // replace special symbols
    QString newName(QString(userName).replace(QRegularExpression(SPECIAL_SYMBOLS), QString("_")));

    if (userName.startsWith(ANON_PREFIX)) // remove the anonymous: prefix
        newName = newName.right(newName.size() - ANON_PREFIX.size());

    if (newName.size() > MAX_NAME_SIZE) // truncate to 16 symbols
        newName = newName.left(MAX_NAME_SIZE);

    if (newName.isEmpty())
        newName = "anon";

    // check if is unique name
    for (const auto &user : remoteUsers.values()) {
        if (user.getName() == newName) {
            if (newName.size() < MAX_NAME_SIZE)
                newName = newName + "_";
            else
                newName = newName.left(MAX_NAME_SIZE - 1) + "_";
            break;
        }
    }

    return newName;
}

void Server::sendServerInitialInfosTo(QTcpSocket *socket)
{
    // send server config change
    auto configChange = ConfigChangeNotifyMessage(bpm, bpi);
    configChange.serializeToDevice(socket);

    auto topicMessage = ServerToClientChatMessage::buildTopicMessage(topic);
    topicMessage.serializeToDevice(socket);
}

void Server::processClientSetChannel(QTcpSocket *socket, const ClientSetChannel& msg)
{
    /**
      ClientSetChannel is received after server/client handshake, it's the end of the initialization process. But this message is
      received while jamming too, when channels are added, removed or the channel name is changed.
    */

    if (!remoteUsers.contains(socket))
        return;

    RemoteUser &user = remoteUsers[socket];

    // update remote user channels list
    user.updateChannels(msg.getChannels(), maxChannels);

    // broadcast the updated remote user channels to everybody
    broadcastUserChanges(user.getFullName(), user.getChannels());
    if (!user.receivedInitialServerInfos()) {
        // send everybody to connected remote user
        sendConnectedUsersTo(socket);

        // send bpm, bpi and server topic to connected user
        sendServerInitialInfosTo(socket);
        user.setReceivedServerInfos();

        //QString message = QString("%1 has joined the room.").arg(user.getName());
        //broadcastServerMessage(message, socket); // broadcast to everybody, except the connected user
    }
}

void Server::sendConnectedUsersTo(QTcpSocket *socket)
{
    if (!remoteUsers.contains(socket))
        return;

    const RemoteUser & connectedUser = remoteUsers[socket];

    UserInfoChangeNotifyMessage msg;

    for (const RemoteUser &user : remoteUsers.values()) {
        QString fullName(user.getFullName());
        if (fullName != connectedUser.getFullName()) {
            for (const auto & channel : user.getChannels()) {
                msg.addUserChannel(fullName, channel);
            }
        }
    }

    msg.serializeToDevice(socket);
}

void Server::broadcastNetworkData(const QByteArray& messageData, QTcpSocket *excludeSocket) {
    for (auto iterator = remoteUsers.begin(); iterator != remoteUsers.end(); ++iterator) {
        QTcpSocket* socket = iterator.key();
        if (socket != excludeSocket) {
            socket->write(messageData);
        }
    }
}

void Server::broadcastNetworkMessage(const INetworkMessage& message, QTcpSocket *excludeSocket) {
    QByteArray broadcastMessageData;
    message.serializeToBuffer(broadcastMessageData);
    broadcastNetworkData(broadcastMessageData, excludeSocket);
}

void Server::broadcastUserChanges(const QString userFullName, const QList<UserChannel> &userChannels)
{
    UserInfoChangeNotifyMessage msg;
    for (int c = 0; c < userChannels.size(); ++c) {
        msg.addUserChannel(userFullName, userChannels.at(c));
    }

    QByteArray userChangesMessageData;
    msg.serializeToBuffer(userChangesMessageData);

    for (auto socket : remoteUsers.keys()) {
        if (remoteUsers[socket].getFullName() != userFullName) {
            socket->write(userChangesMessageData);
        }
    }
}

void Server::processUploadIntervalBegin(QTcpSocket *senderSocket, const UploadIntervalBegin &msg)
{
    if (!remoteUsers.contains(senderSocket))
        return;

    auto senderFullName = remoteUsers[senderSocket].getFullName();
    broadcastNetworkMessage(DownloadIntervalBegin::from(msg, senderFullName), senderSocket);
}

void Server::processUploadIntervalWrite(QTcpSocket *senderSocket, const DownloadIntervalWrite &msg)
{
    if (!remoteUsers.contains(senderSocket))
        return;

    // parsing the DownloadIntervalWrite directly, because the message is identical to UploadIntervaWrite
    broadcastNetworkMessage(msg, senderSocket);
}

void Server::broadcastVotingSystemMessage(const QString &message)
{
    broadcastNetworkMessage(ServerToClientChatMessage::buildVoteSystemMessage(message));
}

void Server::broadcastPublicChatMessage(const ClientToServerChatMessage &receivedMessage, const QString &userFullName)
{
    Q_ASSERT(receivedMessage.isPublicMessage());

    QString messageText = receivedMessage.getArguments().at(0);
    broadcastNetworkMessage(ServerToClientChatMessage::buildPublicMessage(userFullName, messageText));
}

void Server::sendPrivateMessage(const QString &sender, const ClientToServerChatMessage &receivedMessage)
{
    QString destinationUserName = receivedMessage.getArguments().at(0);
    if (!destinationUserName.contains("@"))
        destinationUserName += "@" + tcpServer.serverAddress().toString();

    QString text = receivedMessage.getArguments().at(1);

    for (auto s : remoteUsers.keys()) {
        const RemoteUser &user = remoteUsers[s];
        if (user.getFullName() == destinationUserName) {
            auto msg = ServerToClientChatMessage::buildPrivateMessage(sender, text);
            msg.serializeToDevice(s);
            break;
        }
    }
}

void Server::setTopic(const QString &newTopic)
{
    if (newTopic != topic) {
        topic = newTopic;
        broadcastNetworkMessage(ServerToClientChatMessage::buildTopicMessage(newTopic));
    }
}

void Server::setBpi(quint16 newBpi)
{
    if (newBpi != bpi && newBpi > 0) {
        bpi = newBpi;
        broadcastNetworkMessage(ConfigChangeNotifyMessage(bpm, bpi));
    }
}

void Server::setBpm(quint16 newBpm)
{
    if (newBpm != bpm && newBpm > 0) {
        bpm = newBpm;
        broadcastNetworkMessage(ConfigChangeNotifyMessage(bpm, bpi));
    }
}

void Server::processAdminCommand(const QString &cmd)
{
    AdminCommand command = getAdminCommand(cmd);
    if (command == AdminCommand::Invalid)
        return;

    QStringList parts = cmd.split(" ");
    if (parts.size() < 2)
        return;

    switch (command) {
    case Topic:
        setTopic(parts.at(1));
        break;

    case Bpi: {
        QVariant value(parts.at(1));
        if (value.canConvert<int>())
            setBpi(value.toInt());
        break;
    }
    case Bpm: {
        QVariant value(parts.at(1));
        if (value.canConvert<int>())
            setBpm(value.toInt());
        break;
    }
    default:
        qCritical() << "Invalid admin command received: " << cmd;
        break;
    }
}

Voting *Server::createBpiVoting()
{
    auto newVoting = new Voting(this);

    connect(newVoting, &Voting::expired, this, &Server::bpiVotingExpired);
    connect(newVoting, &Voting::accepted, this, &Server::bpiVotingAccepted);
    connect(newVoting, &Voting::incremented, this, &Server::bpiVotingIncremented);

    return newVoting;
}

Voting *Server::createBpmVoting()
{
    auto newVoting = new Voting(this);

    connect(newVoting, &Voting::expired, this, &Server::bpmVotingExpired);
    connect(newVoting, &Voting::accepted, this, &Server::bpmVotingAccepted);
    connect(newVoting, &Voting::incremented, this, &Server::bpmVotingIncremented);

    return newVoting;
}

void Server::processVoteMessage(const QString &userFullName, quint16 voteValue, quint16 currentValue, VotingMap &votings,
                                                                                            std::function<Voting *()> createVoting)
{
    bool canVote = voteValue && voteValue != currentValue;
    if (canVote) {
        if (!votings.contains(voteValue))
            votings.insert(voteValue, createVoting());

        auto voting = votings[voteValue];
        if (!voting->isRunning()) {
            auto requiredVotes = static_cast<quint8>(remoteUsers.size() * votingSettings.trheshold);
            voting->start(voteValue, requiredVotes, votingSettings.expirationPeriod);
        }

        voting->registerVote(userFullName, voteValue);
    }
}

void Server::processBpiVoteMessage(const ninjam::client::ClientToServerChatMessage &msg, const QString &userFullName)
{
    Q_ASSERT(msg.isBpiVoteMessage());

    quint16 voteValue = msg.extractBpiVoteValue();

    processVoteMessage(userFullName, voteValue, bpi, bpiVotings, std::bind(&Server::createBpiVoting, this));
}

void Server::processBpmVoteMessage(const ninjam::client::ClientToServerChatMessage &msg, const QString &userFullName)
{
    Q_ASSERT(msg.isBpmVoteMessage());

    quint16 voteValue = msg.extractBpmVoteValue();

    processVoteMessage(userFullName, voteValue, bpm, bpmVotings, std::bind(&Server::createBpmVoting, this));
}

void Server::processChatMessage(QTcpSocket *socket, const ClientToServerChatMessage &msg)
{
    if (!remoteUsers.contains(socket))
        return;

    QString userFullName = remoteUsers[socket].getFullName();

    if (msg.isPublicMessage()) {
        broadcastPublicChatMessage(msg, userFullName);
        if (msg.isBpiVoteMessage()) {
            processBpiVoteMessage(msg, userFullName);
        } else if (msg.isBpmVoteMessage()) {
            processBpmVoteMessage(msg, userFullName);
        }
    } else if (msg.isPrivateMessage()) {
        sendPrivateMessage(userFullName, msg);
    } else if (msg.isAdminMessage()) {
        processAdminCommand(msg.getArguments().at(0));
    } else {
        qCritical() << "Error handling chat message!" << msg.getCommand() << msg.getArguments();
    }
}

void Server::processKeepAlive(QTcpSocket *socket)
{
    auto iterator = remoteUsers.find(socket);
    if (iterator != remoteUsers.end()) {
        iterator.value().setLastKeepAliveToNow();
    }
}

void Server::updateKeepAliveInfos()
{
    QByteArray keepAliveMessageData;
    KeepAliveMessage().serializeToBuffer(keepAliveMessageData); /// TODO: check serializeTo count and handle it

    // check if remote users need keep alive request
    for (auto socket : remoteUsers.keys()) {
        const auto &user = remoteUsers[socket];
        auto now = QDateTime::currentMSecsSinceEpoch();
        auto delta = (now - user.getLastKeepAliveReceived()) / 1000; // in seconds
        if (delta >= keepAlivePeriod) {
            if (delta >= keepAlivePeriod * 3) { // client is not responding
                disconnectClient(socket);
            } else {
                socket->write(keepAliveMessageData); /// TODO: check write count and handle it
            }
        }
    }
}

void Server::processClientSetUserMask(QTcpSocket *socket, const ninjam::MessageHeader &header)
{
    //auto msg = ClientSetUserMask::from(socket, header.getPayload());

}

void Server::processReceivedBytes()
{
    updateKeepAliveInfos();

    auto socket = qobject_cast<QTcpSocket *>(QObject::sender());
    if (!socket) {
        qCritical("Error, socket is NULL!");
        return;
    }

    if (!remoteUsers.contains(socket)) {
        qCritical("not contain socket!");
        if (socket->isOpen()) {
            socket->close();
            socket->deleteLater();
        }
        return;
    }

    qint64 bytesAvailable = socket->bytesAvailable();

    RemoteUser &user = remoteUsers[socket];

    while (socket->bytesAvailable() >= 5) { // all messages have minimum of 5 bytes

        MessageHeader header = user.getCurrentHeader();
        if (!header.isValid()) {
            header = MessageHeader::from(socket);
            user.setCurrentHeader(header);
        }

        Q_ASSERT(header.isValid());

        if (socket->bytesAvailable() < header.getPayload()) {
//            qDebug() << "not enough bytes " << socket->bytesAvailable() << "/" << header.getPayload()
//                     << " bytes available  msg code:"<< QString::number(static_cast<quint8>(header.getMessageType()), 16);
            return;
        }

        NinjamInputDataStream stream(socket, header.getPayload());


        switch (header.getMessageType()) {
        case MessageType::ClientAuthUser: {
            ClientAuthUserMessage message;
            message.unserializeFrom(stream);
            processClientAuthUserMessage(socket, message);
            break;
        }
        case MessageType::ClientSetChannel: {
            ClientSetChannel message;
            message.unserializeFrom(stream);
            processClientSetChannel(socket, message);
            break;
        }
        case MessageType::KeepAlive:
            processKeepAlive(socket);
            break;
        case MessageType::UploadIntervalBegin: {
            UploadIntervalBegin message;
            message.unserializeFrom(stream);
            processUploadIntervalBegin(socket, message);
            break;
        }
        case MessageType::UploadIntervalWrite: {
            DownloadIntervalWrite message;
            message.unserializeFrom(stream);
            processUploadIntervalWrite(socket, message);
            break;
        }
        case MessageType::ChatMessage: {
            ClientToServerChatMessage message;
            message.unserializeFrom(stream);
            processChatMessage(socket, message);
            break;
        }
        case MessageType::ClientSetUserMask:
            processClientSetUserMask(socket, header);
            break;

        default:
            qFatal("not handled message code: %s",
                   QString::number(static_cast<quint8>(header.getMessageType()), 16).toStdString().c_str());
        }
        //stream.skipRemainingPayload();

        user.setCurrentHeader(MessageHeader()); // invalidate header to force a new parsing in next loop iteration
    }

    user.setLastKeepAliveToNow();

    qint64 bytesRemaining = socket->bytesAvailable();
    totalDownloadMeasurer.addTransferedBytes(bytesAvailable - bytesRemaining);
}

void Server::handleDisconnection()
{
    auto socket = qobject_cast<QTcpSocket *>(QObject::sender());
    if (socket)
        disconnectClient(socket);
}

QStringList Server::getConnectedUsersNames() const
{
    QStringList names;

    for (const RemoteUser &user : remoteUsers.values())
        names.append(user.getFullName());

    return names;
}

void Server::disconnectClient(QTcpSocket *socket)
{
    if (remoteUsers.contains(socket)) {
        const RemoteUser &user = remoteUsers[socket];

        QString userFullName = user.getFullName();

        // send the PART message and deactivate all user channels
        auto msg = UserInfoChangeNotifyMessage::buildDeactivationMessage(user);
        auto partMsg = ServerToClientChatMessage::buildUserPartMessage(userFullName);
        QByteArray broadcastMsgData;
        partMsg.serializeToBuffer(broadcastMsgData);
        msg.serializeToBuffer(broadcastMsgData);
        broadcastNetworkData(broadcastMsgData, socket);

        remoteUsers.remove(socket);
        socket->deleteLater();

        emit userLeave(userFullName);
    }
}

void Server::handleClientSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    //qDebug() << qobject_cast<QTcpSocket *>(QObject::sender())->errorString();
}

void Server::handleAcceptError(QAbstractSocket::SocketError socketError)
{
    qCritical() << socketError <<  tcpServer.errorString();
}

quint16 Server::getPort() const
{
    return tcpServer.serverPort();
}

QString Server::getIP() const
{
    return tcpServer.serverAddress().toString();
}

void Server::shutdown()
{
    if (tcpServer.isListening()) {
        tcpServer.close();

        for (auto socket : remoteUsers.keys())
            disconnectClient(socket);

        remoteUsers.clear();

        emit serverStopped();
    }
}
