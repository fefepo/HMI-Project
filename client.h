#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);
    void sendData(const QString &data);

private:
    // 포인터 초기화
    QTcpSocket *socket = nullptr;
};

#endif // CLIENT_H