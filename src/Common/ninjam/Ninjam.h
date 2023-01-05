#ifndef _NINJAM_H_
#define _NINJAM_H_

#include <QDebug>
#include <QString>
#include <QDataStream>
#include <QString>
#include <QByteArray>
#include <array>
#include <cstring>

namespace ninjam {

class NetworkUsageMeasurer // used in network statistics
{
public:
    NetworkUsageMeasurer();
    long getTransferRate() const;
    void addTransferedBytes(qint64 totalBytesTransfered);

private:
    long totalBytesTransfered;
    qint64 lastMeasureTimeStamp;
    long transferRate;
};

class MessageGuid : public std::array<char, 16> {
public:
    using array<char, 16>::array;
    MessageGuid() = default;
    explicit MessageGuid(const QByteArray& array) {
        Q_ASSERT(array.size() == 16);
        std::copy_n(array.data(), 16, data());
    }
    bool operator<(const MessageGuid& rhs) const {
        return std::memcmp(data(), rhs.data(), 16) < 0;
    }
};

using MessageFourCC = std::array<char, 4>;

enum class MessageType : quint8
{
    AuthChallenge = 0x00,               // received after connect in server
    ClientAuthUser = 0x80,              // sended to server after receive AUTH_CHALLENGE
    AuthReply = 0x01,                   // received after send to auth challenge
    ClientSetChannel = 0x82,            // sended to server after AuthReply describing channels
    ServerConfigChangeNotify = 0x02,    // received when BPI or BPM change
    UserInfoChangeNorify = 0x03,        // received when user add/remove channels or rename a channel
    DownloadIntervalBegin = 0x04, // received when server is notifiyng about the start of a new audio interval stream
    DownloadIntervalWrite = 0x05, // received for every audio interval chunk. We receive a lot of these messages while jamming.
    UploadIntervalBegin = 0x83,//
    UploadIntervalWrite = 0x84, //
    ClientSetUserMask = 0x81,
    KeepAlive = 0xfd,                   // server requesting a keepalive. If Jamtaba not respond the server will disconnect.
    ChatMessage = 0xc0,                 // received when users are sending chat messages
    Invalid = 0xff
};

class MessageHeader
{
public:
    MessageHeader(); // invalid/incomplete message header

    static MessageHeader from(QIODevice *device);

    inline bool isValid() const
    {
        return messageType != MessageType::Invalid;
    }

    inline MessageType getMessageType() const
    {
        return messageType;
    }

    inline quint32 getPayload() const
    {
        return payload;
    }

private:
    MessageHeader(quint8 type, quint32 payload);


    MessageType messageType = MessageType::Invalid;
    quint32 payload = 0;
};

class NinjamOutputDataStream final : private QDataStream {
public:
    NinjamOutputDataStream(QByteArray& buffer, QIODevice::OpenMode openMode)
        : QDataStream(&buffer, openMode) {
        setByteOrder(LittleEndian);
    }
    explicit NinjamOutputDataStream(QIODevice *device)
        : QDataStream(device) {
        setByteOrder(LittleEndian);
    }
    NinjamOutputDataStream(const NinjamOutputDataStream&) = delete;
    void operator=(const NinjamOutputDataStream&) = delete;
    bool writeHeader(MessageType msgType, quint32 payload) {
        return write<quint8>(static_cast<quint8>(msgType)) &&
                write<quint32>(payload);
    }
    bool writeRawData(const char* data, int len) {
        return QDataStream::writeRawData(data, len) == len;
    }
    bool writeByteArray(const QByteArray& value) {
        return writeRawData(value.data(), value.size());
    }
    template<class T, size_t N>
    bool writeByteArray(const std::array<T, N>& value) {
        static_assert(sizeof(T) == 1, "Invalid array element size");
        return writeRawData(value.data(), N);
    }
    template<class T>
    bool write(const T& value) {
        *this << value;
        return status() == QDataStream::Ok;
    }
    bool writeUtf8String(const QString& value) {
        QByteArray dataArray = value.toUtf8();
        if (!writeRawData(dataArray.data(), dataArray.size())) {
            return false;
        }
        return write<quint8>(0); // NUL TERMINATED
    }    
    static quint32 getByteArrayPayload(const QByteArray& value) {
        return value.size();
    }
    template<class T>
    static constexpr quint32 getPayload() {
        return sizeof(T);
    }
    static quint32 getUtf8StringPayload(const QString& value) {
        return value.toUtf8().size() + 1;
    }
};

class NinjamInputDataStream final : private QDataStream {
private:
    quint32 remainingPayload;

    inline void consumeRemainingPayload(quint32 size) {
        remainingPayload -= size;
    }

public:
    NinjamInputDataStream(const QByteArray& buffer, quint32 remainingPayload)
        : QDataStream(buffer), remainingPayload(remainingPayload) {
        setByteOrder(LittleEndian);
    }
    NinjamInputDataStream(QIODevice *device, quint32 remainingPayload)
        : QDataStream(device), remainingPayload(remainingPayload) {
        setByteOrder(LittleEndian);
    }
    NinjamInputDataStream(const NinjamInputDataStream&) = delete;
    void operator=(const NinjamInputDataStream&) = delete;
    inline quint32 getRemainingPayload() const {
        return remainingPayload;
    }
    inline bool hasRemainingPayload(quint32 size) const {
        return size <= remainingPayload;
    }
    bool skipRemainingPayload() {
        if (remainingPayload != 0) {
            int skipped = QDataStream::skipRawData(remainingPayload);
            if (skipped != -1) {
                remainingPayload -= skipped;
                return remainingPayload == 0;
            }
            qWarning() << "NinjamInputDataStream::skipRemainingPayload failed." <<
                          "Len: " << remainingPayload;
            return false;
        }
        return true;
    }
    bool skip(int len) {
        if (hasRemainingPayload(len)) {
            int skipped = QDataStream::skipRawData(len);
            if (skipped != -1) {
                consumeRemainingPayload(skipped);
                return skipped == len;
            }
            qWarning() << "NinjamInputDataStream::skip failed." << "Len: " << len;
        }
        return false;
    }
    bool readRawData(char* data, int len) {
        if (hasRemainingPayload(len)) {
            int readed = QDataStream::readRawData(data, len);
            if (readed != -1) {
                consumeRemainingPayload(readed);
                return readed == len;
            }
            qWarning() << "NinjamInputDataStream::readRawData failed." << "Len: " << len;
        }
        return false;
    }
    template<class T, size_t N>
    bool readByteArray(std::array<T, N>& value) {
        static_assert(sizeof(T) == 1, "Invalid array element size");
        return readRawData(value.data(), N);
    }
    bool readByteArray(QByteArray& value) {
        return readRawData(value.data(), value.size());
    }
    template<class T>
    bool read(T& value) {
        if (hasRemainingPayload(sizeof(T))) {
            *this >> value;
            QDataStream::Status result = status();
            if (result == QDataStream::Ok) {
                consumeRemainingPayload(sizeof(T));
                return true;
            }
            qWarning() << "NinjamInputDataStream::read failed. Status: " << result;
        }
        return false;
    }
    bool readAciiStringFixed(QString& value, quint32 size) {
        QByteArray byteArray(size, Qt::Uninitialized);
        if (readRawData(byteArray.data(), size)) {
            value = byteArray;
            return true;
        }
        return false;
    }
    bool readUtf8StringFixed(QString& value, quint32 size) {
        QByteArray byteArray(size, Qt::Uninitialized);
        if (readRawData(byteArray.data(), size)) {
            value = QString::fromUtf8(byteArray);
            return true;
        }
        return false;
    }
    bool readUtf8String(QString& value, bool allowNotTerminated = true) {
        quint8 byte;
        bool terminated = false;
        QByteArray byteArray;
        while (hasRemainingPayload(sizeof(byte))) {
            *this >> byte;
            QDataStream::Status result = status();
            if (result != QDataStream::Ok) {
                qWarning() << "NinjamInputDataStream::readUtf8String failed." <<
                              " Status: " << result;
                return false;
            }
            consumeRemainingPayload(sizeof(byte));
            if (byte == '\0') {
                terminated = true;
                break;
            }
            byteArray.append(byte);
        }
        if (terminated || allowNotTerminated) {
            value = QString::fromUtf8(byteArray.data(), byteArray.size());
            return true;
        }
        qWarning() << "NinjamInputDataStream::readUtf8String failed." <<
                      " No zero terminator";
        return false;
    }
    bool readUtf8Strings(QStringList& value, quint32 size) {
        QByteArray byteArray(size, Qt::Uninitialized);
        if (!readRawData(byteArray.data(), size)) {
            return false;
        }
        int index = byteArray.indexOf('\0');
        if (index == -1) {
            qWarning() << "NinjamInputDataStream::readUtf8Strings failed. No zero terminator";
            return false; // or return single string?
        }
        quint32 startIndex = 0;
        do {
            value.append(QString::fromUtf8(byteArray.data() + startIndex, index - startIndex));
            startIndex = index + 1;
            if (startIndex >= size) {
                break;
            }
            index = byteArray.indexOf('\0', startIndex);
        } while (index != -1);
        return true;
    }
};

class INetworkMessage
{
private:
    MessageType msgType;

protected:
    explicit INetworkMessage(MessageType msgType);
public:
    virtual ~INetworkMessage();
    virtual void printDebug(QDebug &dbg) const = 0;
    virtual quint32 getSerializePayload() const = 0;
    virtual bool serializeTo(NinjamOutputDataStream& stream) const;
    virtual bool unserializeFrom(NinjamInputDataStream& stream) = 0;

    inline MessageType getMsgType() const
    {
        return msgType;
    }

    inline bool serializeToBuffer(QByteArray& array) const {
        NinjamOutputDataStream stream(array, QIODevice::WriteOnly | QIODevice::Append);
        return serializeTo(stream);
    }
    inline bool serializeToDevice(QIODevice* device) const {
        NinjamOutputDataStream stream(device);
        return serializeTo(stream);
    }
};

} // namespace

#endif
