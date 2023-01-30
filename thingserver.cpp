#include "thingserver.h"

#include <QtWebSockets>
#include <QtCore>
#include <QStringList>
#include <QHostAddress>
#include <QNetworkInterface>

#include <cstdio>

using namespace std;

QT_USE_NAMESPACE

static QString getIdentifier(QWebSocket *peer)
    {
        return QStringLiteral("%1:%2").arg(peer->peerAddress().toString(),QString::number(peer->peerPort()));
    }

ThingServer::ThingServer(quint16 port, QObject *parent):QObject(parent),
                         m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Thing Server"),QWebSocketServer::NonSecureMode,this))
    {
        QTextStream(stdout)<<"QT " << QT_VERSION_STR << "\n";
        get_host_address();
        ///set up the server
        if (m_pWebSocketServer->listen(QHostAddress::Any, port))
            {
                QTextStream(stdout)  << "Thing Server listening on port " << port << "\n";

                connect(m_pWebSocketServer, &QWebSocketServer::newConnection,this, &ThingServer::onNewConnection);
            }
        server_name="SRV1";
        ///set up the keep alive pinger
        keep_alive_timer=new QTimer(this);
        keep_alive_timer->setSingleShot(false);
        connect(keep_alive_timer,SIGNAL(timeout()), this, SLOT(PING()));
        keep_alive_timer->start(PING_T);
    }
void ThingServer::get_host_address()
    {
        QString localhostname =  QHostInfo::localHostName();
        QString localhostIP;
        QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
        foreach (const QHostAddress& address, hostList)
            {
                if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false)
                    {
                        localhostIP = address.toString();
                    }
            }
        QString localMacAddress;
        QString localNetmask;
        foreach (const QNetworkInterface& networkInterface, QNetworkInterface::allInterfaces())
            {
                foreach (const QNetworkAddressEntry& entry, networkInterface.addressEntries())
                    {
                        if (entry.ip().toString() == localhostIP)
                            {
                                localMacAddress = networkInterface.hardwareAddress();
                                localNetmask = entry.netmask().toString();
                                break;
                            }
                    }
            }
        QTextStream(stdout) <<  "Localhost name: " << localhostname<<"\n";
        QTextStream(stdout) <<  "IP = " << localhostIP<<"\n";
        QTextStream(stdout) <<  "MAC = " << localMacAddress<<"\n";
        QTextStream(stdout) <<  "Netmask = " << localNetmask<<"\n";
    }

void ThingServer::PING()
    {

        ///ping all the connected clients in order to prevent connection time out

        ///QTextStream(stdout) << "ping "<<m_time.currentDateTime().toString()<<endl;

        if(m_active_clients.isEmpty()) return;
        QMap<QString, QWebSocket *>::Iterator i;
        QTextStream(stdout) << "sent ping" <<" \n" ;
        QTextStream(stdout) << "map size"<<m_active_clients.size()<<"\n";

        for(i=m_active_clients.begin();i!=m_active_clients.constEnd();i++)
            {
                QTextStream(stdout) << "send to "<<i.key()<<"\n";
                if(i.value()->isValid())
                    {
                        i.value()->sendTextMessage(create_ping());
                    }
                /*    else
                    {  ///remove invalid nodes
                        QWebSocket *w=i.value();
                        m_active_clients.remove(i.key());
                        w->deleteLater();
                    }*/
            }
    }


QString ThingServer::create_ping(void)
    {
        ///Ping ping|name
        QString ping="PING";
        ping.append(DELIMIT);
        ping.append(server_name);
        return ping;
    }

ThingServer::~ThingServer()
    {
        m_pWebSocketServer->close();
    }

void ThingServer::onNewConnection()
    {
        auto  pSocket = m_pWebSocketServer->nextPendingConnection();
        QTextStream(stdout) << getIdentifier(pSocket) << "connected!\n";
        pSocket->setParent(this);
        connect(pSocket, &QWebSocket::textMessageReceived,this, &ThingServer::processMessage);
        connect(pSocket, &QWebSocket::disconnected,this, &ThingServer::socketDisconnected);
        temp_client=pSocket;
        /// just tidy a bit
        cleanup_connections();
    }

void ThingServer::processMessage(const QString &message)
    {
        /// QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());  // a pointer to the sender just in case
        QTextStream(stdout) <<"got message:  "<<message<<" "<<"\n";
        ///Proccess pong here
        if(message.contains("PONG"))
            {
                QTextStream(stdout) <<" got "<<message;
                return;
            }
        /*  if(message.at(0)=="#")
            {
                /// parse server instruction
            }*/
        else
            {
                QTextStream(stdout) <<"ThingServer::processMessage ";
                parse_message(message);
            }
        //  parse_message(message);
    }

void ThingServer::parse_message(const QString &message)
    {
        QStringList data;
        data=message.split(DELIMIT); ///split message by delimiter
        QTextStream(stdout) << " message"<<"    "<<message<<"\n";
        if(data.size()>=3)
            {
                if(data.at(0).compare(AUTH)==0) /// client requests new connection
                    {
                        /// connect operaation
                        /// AUTH|thing_type|thing_name
                        register_connection(data);
                        return;
                    }

                if(data.at(0).compare(SEND)==0)  /// message to be sent to target device
                    {
                        ///message structure
                        ///    0      1      2        3        4 .....
                        /// request|emiter|target|instruction|data.....
                        ///
                        QString target=data.at(2);
                        forward_message(target,message);
                    }
                if(data.at(0).compare(BRDCAST)==0)
                    {
                        broadcast_message(message);
                    }
            }
        else
            {
                temp_client->disconnect("Improperly configured device \n");
                temp_client->deleteLater();
                // QTextStream(stdout) << "  data 0 "<<data.at(0);
                // QTextStream(stdout) << "  data 1 "<<data.at(1);
            }
    }

void ThingServer::broadcast_message(const QString &msg)
    {
        QMap<QString, QWebSocket *>::Iterator i;
        ///message structure
        ///    0      1      2        3        4 .....
        /// request|emiter|target|instruction|data.....
        /// no change in the message structure
        for(i=m_active_clients.begin();i!=m_active_clients.constEnd();i++)
            {
                QTextStream(stdout) << "send to "<<i.key()<<"\n";
                if(i.value()->isValid())
                    {
                        i.value()->sendTextMessage(msg);
                    }
            }
    }

void ThingServer::forward_message(const QString &target, const QString &msg)
    {
        ///message structure
        ///
        /// emiter|target|instruction|data
        ///
        if(m_active_clients.contains(target))
            {
                QWebSocket *dest=m_active_clients.value(target);
                ///send the message
                dest->sendTextMessage(msg);
            }
        else
            {
                QTextStream(stdout) << "error: "<<target<<"not connected" <<"\n";
            }
    }

void ThingServer::register_connection(const QStringList &data)
    {
        /// add to active list
        /// data.at(2) is the internal name of the device
        /// it is used as the map key
        ///
        ///  /// AUTH|thing_type|thing_name
        if(!m_active_clients.contains(data.at(2)))
            {
                m_active_clients.insert(data.at(2),temp_client);
                QTextStream(stdout)<<"###connected client### "<<data.at(2)<<"\n";
            }
        else
            {
                // temp_client->disconnect("duplicate names not allowed \n");
                QTextStream(stdout)<<"duplicate names not allowed \n"<<data.at(2)<<"\n";
                temp_client->deleteLater();
                ///    m_active_clients.clear();
            }
    }

void ThingServer::socketDisconnected()
    {
        ///must implement a clean disconnect here
        QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
        QTextStream(stdout) << getIdentifier(pClient) << " disconnected!\n";
        // pClient->deleteLater();
        /*
        if (pClient!=NULL)
            {
                //  m_active_clients.clear();//removeAll(pClient);
                //  m_control_clients.clear();
                pClient->deleteLater();
            }
            */
        /// clean the client map from any disconnected devices and dead sockets
        cleanup_connections();
    }

void ThingServer::cleanup_connections()
    {
        QMap<QString, QWebSocket *>::Iterator i;
        for(i=m_active_clients.begin();i!=m_active_clients.constEnd();i++)
            {
                if(!(i.value()->isValid()))
                    {
                        QWebSocket *w=i.value();
                        m_active_clients.remove(i.key());
                        w->deleteLater();
                    }
            }
    }

