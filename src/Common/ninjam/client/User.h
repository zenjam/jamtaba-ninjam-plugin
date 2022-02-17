#ifndef USER_H
#define USER_H

#include <QMap>
#include "UserChannel.h"

namespace ninjam
{

namespace client
{
    QString extractUserName(const QString &userFullName);
    QString extractUserIP(const QString &userFullName);
    QString maskIpInUserFullName(const QString &userFullName);
    QString maskIP(const QString& ip);
    bool ipIsMasked(const QString &ip);

    class User
    {

    public:
        explicit User(const QString &fullName = "");
        virtual ~User();

        inline bool hasChannels() const
        {
            return !channels.isEmpty();
        }

        bool hasActiveChannels() const;
        bool hasChannel(int channelIndex) const;

        inline const QList<UserChannel>& getChannels() const {
            return channels;
        }

        inline QList<UserChannel>& getChannels() {
            return channels;
        }

        inline bool operator<(const User &other) const {
            return fullName < other.fullName;
        }

        inline const QString& getIp() const {
            return ip;
        }

        inline const QString& getName() const {
            return name;
        }

        inline const QString& getFullName() const {
            return fullName;
        }

        void addChannel(const UserChannel &channel);
        bool removeChannel(quint8 channelIndex);

        inline int getChannelsCount() const {
            return channels.size();
        }

        template<class F>
        inline bool visitChannel(quint8 channelIndex, F&& functor) const {
            for (int i = 0; i < channels.size(); i++) {
                if (channels[i].getIndex() == channelIndex) {
                    functor(channels[i]);
                    return true;
                }
            }
            return false;
        }

        template<class F>
        inline bool visitChannel(quint8 channelIndex, F&& functor) {
            for (int i = 0; i < channels.size(); i++) {
                if (channels[i].getIndex() == channelIndex) {
                    functor(channels[i]);
                    return true;
                }
            }
            return false;
        }

    protected:
        QString fullName;
        QString name;
        QString ip;
        QList<UserChannel> channels;
    };

    QDebug &operator<<(QDebug &out, const User &user);

} //ns

} // ns

#endif
