#include "Ninjam.h"

#include <QDateTime>

namespace ninjam {

NetworkUsageMeasurer::NetworkUsageMeasurer() :
    totalBytesTransfered(0),
    lastMeasureTimeStamp(0),
    transferRate(0)
{

}

void NetworkUsageMeasurer::addTransferedBytes(qint64 bytesTransfered)
{
    totalBytesTransfered += bytesTransfered;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (!lastMeasureTimeStamp)
        lastMeasureTimeStamp = now;

    const qint64 ellapsedTime = now - lastMeasureTimeStamp;

    if (ellapsedTime >= 1000) {
        transferRate = totalBytesTransfered / (ellapsedTime / 1000.0);
        totalBytesTransfered = 0;
        lastMeasureTimeStamp = now;
    }
}

long NetworkUsageMeasurer::getTransferRate() const
{
    return transferRate;
}

// ---------------------------------------------------------------

MessageHeader::MessageHeader(quint8 type, quint32 payload) :
    messageType(static_cast<MessageType>(type)),
    payload(payload)
{

}

MessageHeader::MessageHeader() :
    MessageHeader(static_cast<quint8>(MessageType::Invalid), 0) // invalid header
{

}

MessageHeader MessageHeader::from(QIODevice *device)
{
    QDataStream stream(device);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint8 type;
    quint32 payload;

    stream >> type >> payload;

    return MessageHeader(type, payload);
}

// ---------------------------------------------------------------

INetworkMessage::INetworkMessage(MessageType msgType)
    : msgType(msgType)
{
    //
}

INetworkMessage::~INetworkMessage()
{
    //
}

bool INetworkMessage::serializeTo(NinjamOutputDataStream& stream) const
{
    return stream.writeHeader(msgType, getSerializePayload());
}

} // namespace


