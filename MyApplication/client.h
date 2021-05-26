#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

static inline qint32 ArrayToint(QByteArray source);

namespace Ui {
class Client;
}

class Client : public QMainWindow
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = 0);
    ~Client();

    QTcpSocket *socket;

protected:
    bool header_Read = false;
    int size_of_data_to_read;

public slots:
    bool connectToHost(QString host, int port);
    bool writeData(QString data);

private slots:
    void displayError(QAbstractSocket::SocketError socketError);

    void sendInitialCommand();

    void readAck();
    void readyRead();
    void readData();

    void on_pushButton_startSimulation_clicked();

    void on_pushButton_stopSimulation_clicked();

    void on_pushButton_updateSimulation_clicked();

    void on_SendMessage_clicked();

private:
    Ui::Client *ui;
    QHash<QTcpSocket*, QByteArray*> buffers;
    QHash<QTcpSocket*, qint32*> sizes;
};

#endif // CLIENT_H
