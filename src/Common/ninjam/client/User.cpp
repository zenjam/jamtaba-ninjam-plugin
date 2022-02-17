#include "User.h"
#include "UserChannel.h"
#include <QDebug>
#include <QRegularExpression>
#include "log/Logging.h"

using ninjam::client::User;
using ninjam::client::UserChannel;

static const char* IP_MASK = ".x";

QString ninjam::client::extractUserName(const QString &fullName)
{
    QChar separator('@');
    if (fullName.contains(separator))
        return fullName.split(separator).at(0);

    return fullName;
}

QString ninjam::client::extractUserIP(const QString &fullName)
{
    QChar separator('@');
    if (fullName.contains(separator))
        return fullName.split(separator).at(1);

    return QString();
}

QString ninjam::client::maskIpInUserFullName(const QString &userFullName)
{
    auto name = extractUserName(userFullName);
    auto ip = extractUserIP(userFullName);

    if (ipIsMasked(ip))
        return userFullName;

    return QString("%1@%2").arg(name, maskIP(ip));
}

QString ninjam::client::maskIP(const QString &ip)
{
    auto index = ip.lastIndexOf(".");
    if (index)
        return ip.left(index) + IP_MASK; // masking

    return ip;
}

bool ninjam::client::ipIsMasked(const QString &ip)
{
    return ip.endsWith(IP_MASK);
}

User::User(const QString &fullName) :
    fullName(fullName),
    name(ninjam::client::extractUserName(fullName)),
    ip(ninjam::client::extractUserIP(fullName))
{
}

User::~User()
{
    //
}

void User::addChannel(const UserChannel &channel)
{
    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].getIndex() == channel.getIndex()) {
            channels[i] = UserChannel(channel);
            return;
        }
    }
    channels.append(UserChannel(channel));
}

bool User::hasActiveChannels() const
{
    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].isActive()) {
            return true;
        }
    }
    return false;
}

bool User::hasChannel(int channelIndex) const
{
    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].getIndex() == channelIndex) {
            return true;
        }
    }
    return false;
}

bool User::removeChannel(quint8 channelIndex)
{
    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].getIndex() == channelIndex) {
            channels.removeAt(i);
            return true;
        }
    }
    return false;
}

QDebug &ninjam::client::operator<<(QDebug &out, const User &user)
{
    out << "NinjamUser{"
        << "name=" << user.getName()
        << ", ip=" << user.getIp()
        << ", fullName=" << user.getFullName()
        << "}\n";

    if (!user.hasChannels())
        out << "\tNo channels!\n";

    out << "--------------\n";

    return out;
}
