#include "UserChannel.h"

#include <QDebug>

using ninjam::client::UserChannel;

UserChannel::UserChannel(const QString &channelName, quint8 channelIndex, UserChannel::Flags flags, bool active,
                         quint16 volume, quint8 pan) :
    channelName(channelName),
    active(active),
    index(channelIndex),
    volume(volume),
    pan(pan),
    flags(flags)
{
    //
}

UserChannel::UserChannel() :
    UserChannel("invalid", -1, UserChannel::Flags::Intervalic, false, 0, 0)
{
    //
}

UserChannel::~UserChannel()
{
    //
}

