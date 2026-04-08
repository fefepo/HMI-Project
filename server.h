#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>

class Server : public QTcpServer {
    Q_OBJECT
public:
    Server(QObject *parent = nullptr);
    ~Server(); //

signals:
    void dataReceived(QString data);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();

private:
    QTcpSocket *clientSocket = nullptr;
};

#endif