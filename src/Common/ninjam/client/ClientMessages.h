#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H

#include <QtGlobal>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QIODevice>
#include "ninjam/Ninjam.h"
#include "ninjam/client/Types.h"

namespace ninjam {

namespace client {

class User;
class UserChannel;

using ninjam::MessageType;

class ClientMessage : public INetworkMessage {
public:
    using INetworkMessage::INetworkMessage;
};

// ++++++++++++++++++++++++++++++++++++++=

class ClientAuthUserMessage : public ClientMessage
{
public:
    ClientAuthUserMessage();
    ClientAuthUserMessage(const QString &userName, const QByteArray &challenge,
                          quint32 protocolVersion, const QString &password);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

    inline QString getUserName() const
    {
        return userName;
    }

private:
    bool processReadedData(const QString& password);

    QByteArray passwordHash;
    QString userName;
    quint32 clientCapabilites;
    quint32 protocolVersion;
    QByteArray challenge;
};

// +++++++++++++++++++++++++++++++++++++++=

class ClientSetChannel : public ClientMessage
{
public:
    ClientSetChannel();
    explicit ClientSetChannel(const QList<ninjam::client::ChannelMetadata> &channels);

    void addChannel(const QString &channelName, quint8 flags, bool active = true);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

    inline QList<UserChannel> getChannels() const
    {
        return channels;
    }

    static quint8 toFlags(bool voiceChatActivated) {
         //Possible values: 0 - ninjam interval based , 2 - voice chat, 4 - session mode
        return voiceChatActivated ? 2 : 0;
    }
private:
    QList<UserChannel> channels;
};

// ++++++++++++++++++++++++++++++

class ClientSetUserMask : public ClientMessage
{
public:
    ClientSetUserMask();
    ClientSetUserMask(const QString &userName, quint32 channelsMask);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

private:
    QString userName;
    quint32 channelsMask;
};

// +++++++++++++++++++++++++++

class ClientToServerChatMessage : public ClientMessage
{
public:
    ClientToServerChatMessage();

    static ClientToServerChatMessage buildPublicMessage(const QString &message);
    static ClientToServerChatMessage buildPrivateMessage(const QString &message, const QString &destionationUserName);
    static ClientToServerChatMessage buildAdminMessage(const QString &message);

    inline QString getCommand() const
    {
        return command;
    }

    inline QStringList getArguments() const
    {
        return arguments;
    }

    bool isPublicMessage() const;
    bool isAdminMessage() const;
    bool isPrivateMessage() const;
    bool isBpiVoteMessage() const;
    bool isBpmVoteMessage() const;

    quint16 extractBpiVoteValue() const;
    quint16 extractBpmVoteValue() const;

    static quint16 extractVoteValue(const QString &string);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

private:
    ClientToServerChatMessage(const QString &comand, const QString &arg1, const QString &arg2, const QString &arg3, const QString &arg4);
    QString command;
    QStringList arguments;
};

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
Offset Type        Field
0x0    uint8_t[16] GUID (binary)
0x10   uint32_t    Estimated Size
0x14   uint8_t[4]  FourCC
0x18   uint8_t     Channel Index
0x19   ...         Username (NUL-terminated)
*/

class UploadIntervalBegin : public ClientMessage
{
public:
    UploadIntervalBegin();
    UploadIntervalBegin(const MessageGuid &GUID, quint8 channelIndex, bool isAudioInterval);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

    inline const MessageGuid& getGUID() const
    {
        return GUID;
    }

    inline const MessageFourCC& getFourCC() const
    {
        return fourCC;
    }

    inline quint32 getEstimatedSize() const
    {
        return estimatedSize;
    }

    inline quint8 getChannelIndex() const
    {
        return channelIndex;
    }

private:
    MessageGuid GUID;
    quint32 estimatedSize;
    MessageFourCC fourCC;
    quint8 channelIndex;
};

// ++++++++++++++++++++++++++++++++++++++++++++++++=

class UploadIntervalWrite : public ClientMessage
{
public:
    UploadIntervalWrite();
    UploadIntervalWrite(const MessageGuid &GUID, const QByteArray &encodedData, bool lastPart);

    quint32 getSerializePayload() const override;
    bool serializeTo(NinjamOutputDataStream& stream) const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;

    inline const MessageGuid& getGUID() const
    {
        return GUID;
    }

    inline const QByteArray& getEncodedData() const
    {
        return encodedData;
    }

    inline bool isLastPart() const
    {
        return lastPart;
    }

private:
    MessageGuid GUID;
    QByteArray encodedData;
    bool lastPart;
};
}     // ns
} // ns

#endif
