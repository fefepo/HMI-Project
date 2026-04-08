#include "server.h"
#include <QByteArray>

Server::Server(QObject *parent)
    : QTcpServer(parent), clientSocket(nullptr)
{
    if (!this->listen(QHostAddress::Any, 12345)) {
        qDebug() << "서버 시작 실패!";
    } else {
        qDebug() << "서버가 12345 포트에서 대기 중입니다.";
    }
}

Server::~Server() {
    if (clientSocket) clientSocket->close();
}

void Server::incomingConnection(qintptr socketDescriptor) {
    clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    qDebug() << "새로운 클라이언트 연결됨.";
}

void Server::onReadyRead() {
    QByteArray data = clientSocket->readAll();
    QString message = QString::fromUtf8(data);
    emit dataReceived(message); // MainWindow로 데이터 전송
}