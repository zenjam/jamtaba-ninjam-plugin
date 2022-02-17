#ifndef COMMONMESSAGES_H
#define COMMONMESSAGES_H
#endif // COMMONMESSAGES_H

#include <QDebug>
#include "ninjam/Ninjam.h"

namespace ninjam {

namespace common {

class CommonMessage : public INetworkMessage {
public:
    using INetworkMessage::INetworkMessage;
};

// ++++++++++++++++++++++++++

class KeepAliveMessage : public CommonMessage
{
public:
    KeepAliveMessage();
    quint32 getSerializePayload() const override;
    bool unserializeFrom(NinjamInputDataStream& stream) override;
    void printDebug(QDebug &dbg) const override;
};

}

}
