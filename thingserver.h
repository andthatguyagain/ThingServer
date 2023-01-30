#ifndef THINGSERVER_H
#define THINGSERVER_H

#include "defines.h"

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QTimer>
#include <QDateTime>
///#include <QNetworkInterface>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)
QT_FORWARD_DECLARE_CLASS(QString)



typedef struct
    {
        QWebSocket *m_client;  ///client websocket
        QString m_type; ///type of thing
        QString m_id;  ///uninque internal id for thing
        QString m_IP;  ///IP of thing
    }   client;


class ThingServer : public QObject
    {
         Q_OBJECT
    public:

        explicit ThingServer(quint16 port, QObject *parent = nullptr);
        ~ThingServer() override;
        QTimer *keep_alive_timer;
        QDateTime m_time;

    public slots:
        void PING();

    private slots:
        void onNewConnection();
        void processMessage(const QString &message);
        void socketDisconnected();

    private:

        void get_host_address();
        QWebSocketServer *m_pWebSocketServer;
     // QList<QWebSocket *> m_clients;
        QMap<QString, QWebSocket *> m_active_clients,m_control_clients;
        QWebSocket *temp_client ;
        QString dev_type;
        QString server_name;
        void parse_message(const QString &message);
        void register_connection(const QStringList &data);
        QString create_ping();
        void forward_message(const QString &data,const QString &msg);
        void broadcast_message(const QString &msg);
        void cleanup_connections();

      ///  QNetworkInterface  n_interface;

    };

#endif // THINGSERVER_H
