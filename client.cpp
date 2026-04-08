#include "client.h"
#include <QDebug>

Client::Client(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);
    socket->connectToHost("127.0.0.1", 12345);

    connect(socket, &QTcpSocket::connected, this, [=]() {
        qDebug() << "서버에 성공적으로 연결되었습니다!";
        sendData("HMI_CLIENT_CONNECTED");
    });
}

void Client::sendData(const QString &data) {
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data.toUtf8());
        socket->flush();
    }
}