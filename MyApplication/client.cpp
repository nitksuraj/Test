#include "client.h"
#include "ui_client.h"

#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QtNetwork>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static inline QByteArray IntToArray(qint32 len);
QFile w_file("C:/Users/ADMIN/Desktop/Response.txt");

Client::Client(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Client)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);

    if(w_file.exists())
    {
        if(!w_file.open(QIODevice::WriteOnly | QIODevice::Text))
            qDebug()<<"Failed to open the Response File!";
    }
    else
        qDebug()<<"Response file does not exists! ";

    ui->comboBox_ServerName->setEditable(true);

    //Find out the Name of the Machine
    QString name = QHostInfo::localHostName();
    if(!name.isEmpty())
    {
        ui->comboBox_ServerName->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if(!domain.isEmpty())
            ui->comboBox_ServerName->addItem(domain);
    }
    if(name != QString("localhost"))
        ui->comboBox_ServerName->addItem(QString("localhost"));

    //Find out Ip-Address of the Machine
    QList<QHostAddress> ipAddressList = QNetworkInterface::allAddresses();
    for(int i = 0; i < ipAddressList.size(); i++)
    {
        if(!ipAddressList.at(i).isLoopback())
            ui->comboBox_ServerName->addItem(ipAddressList.at(i).toString());
        else
            ui->comboBox_ServerName->addItem(ipAddressList.at(i).toString());
    }

    ui->lineEdit_Port->setValidator(new QIntValidator(1,65535,this));
    ui->lineEdit_Port->setFocus();


    //Connect to Target automatically
    QFile server_file("C:/Program Files (x86)/ComAvia/PATS++/Server.txt");

    if(!server_file.exists())
    {
         qDebug()<<"File doesnot exist!";
         return;
    }
    if(!server_file.open(QIODevice::ReadOnly))
    {
        qWarning()<<"Failed to open the File !";
        return;
    }

    QTextStream stream(&server_file);
    QString line = stream.readLine();
    QStringList ip_port_list = line.split("\t");
    QString ip = ip_port_list[1];
    int port = ip_port_list[2].toInt();
    //qDebug()<<"Ip : "<<ip<<"Port : "<<port;
    if(connectToHost(ip,port))
    {
        qDebug()<<"Connected Successfully";
    }
    else
    {
        connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    }
    //socket->connectToHost(ip,qint16(port));
    //connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    sendInitialCommand();
    connect(socket,SIGNAL(readyRead()),this,SLOT(readAck()));
    //connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));
    //connect(socket,SIGNAL(readyRead()),this,SLOT(readData()));
}

Client::~Client()
{
    delete ui;

    w_file.flush();
    w_file.close();
}

bool Client::connectToHost(QString host, int port)
{
    qDebug()<<"Ip : "<<host<<"Port : "<<port;
    socket->abort();
    socket->connectToHost(host,qint16(port));
    return socket->waitForConnected();
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        QMessageBox::information(this,tr("My Client"),tr("the host was not found. Pls check the host name and port setting"));

        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this,tr("My Client"),tr("The connection was refused by the peer. Make sure the server is running and "
                                                              "check the host name and port setting are correct"));
        break;
    default:
        QMessageBox::information(this,tr("My Client"),tr("The following error occured %1").arg(socket->errorString()));
        break;
    }
}

void Client::on_SendMessage_clicked()
{
    QString message = ui->textEdit_Message->toPlainText();
    int len = message.length();
    QByteArray q_b(reinterpret_cast<const char*>(&len),sizeof(qint32));
    qDebug("QByteArray has bytes %s",qPrintable(q_b));
    socket->write(q_b);

    //Only send the text to server if its not Empty
    if(!message.isEmpty())
    {
        socket->write(QString(message).toUtf8());
        qDebug()<<"Message Sent : "<<endl;
        qDebug().noquote()<<message<<endl;
    }
    else
        qDebug()<<"Message Not Sent!"<<endl;

    ui->textEdit_Message->clear();
    ui->textEdit_Message->setFocus();
}

bool Client::writeData(QString data)
{
    if(socket->state() == QAbstractSocket::ConnectedState)
    {
        qDebug()<<"Length : "<<data.size();
        socket->write(IntToArray(data.size())); //write the size of data
        socket->write(data.toUtf8()); //write the data itself
        return socket->waitForBytesWritten();
    }
    else
        return false;
}

//Use qint32 to ensure that the number have 4 bytes
QByteArray IntToArray(qint32 len)
{
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << len;
    return temp;
}



void Client::readAck()
{
    qDebug()<<"Ack Reading...";
    QDataStream in(socket);
 //   QTextStream stream(socket);
    in.setVersion(QDataStream::Qt_4_0);
  //  QTcpSocket *socket = (QTcpSocket *)sender();
//    QString msg;
//    QString line = QString::fromUtf8(socket->readAll()).trimmed();
//    while(socket->canReadLine())
//    {
//        QString line = QString::fromUtf8(socket->readLine()).trimmed();
//        if(msg.isNull())
//            msg += line;
//        else
//            msg += '\n'+line;
//    }

//    ui->textEdit_Ack->setText(line);
//    qDebug()<<line;
    QByteArray ack;
    ack = socket->readAll().trimmed();
    ack.remove(0,4);
    ui->textEdit_Ack->setText(ack);
    qDebug().noquote()<<ack<<endl;

    //Write to a File
    QTextStream out(&w_file);
    out<< QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")<<" : "<<ack<<endl;
    qDebug()<<"\n"<<endl;
    qDebug()<<"Writing Finished.";
}

void Client::readyRead()
{
    qDebug()<<"Inside ReadyRead";
    QByteArray *buffer = new QByteArray();
            qint32 *s = new qint32(0);
//            buffers.insert(socket, buffer);
//            sizes.insert(socket, s);
//    QByteArray *buffer = buffers.value(socket);
//    qint32 *s = sizes.value(socket);
    qint32 size = *s;
    qDebug()<<"data : "<<socket->bytesAvailable();
    while(socket->bytesAvailable() > 0)
    {
        qDebug()<<"Read ack";
        buffer->append(socket->readAll());
        while ((size == 0 && buffer->size() >= 4) || (size > 0 && buffer->size() >= size)) {
            if(size == 0 && buffer->size() >= 4)
            {
                size = ArrayToint(buffer->mid(0,4));
                *s = size;
                buffer->remove(0,4);
                qDebug()<<"size : "<<size<<endl;
            }
            if(size > 0 && buffer->size() >= size)
            {
                QByteArray data = buffer->mid(0, size);
                buffer->remove(0,size);
                size = 0;
                *s = size;
                qDebug()<<"Data :"<<data;
            }
        }
    }
}

qint32 ArrayToint(QByteArray source)
{
    qint32 temp;
    QDataStream data(&source, QIODevice::ReadWrite);
    data >> temp;
    return temp;
}

void Client::readData()
{
    int bytes = socket->bytesAvailable();
    qDebug()<<"Read data here" << " : "<<bytes;
    bool contains_enough_data = true;
    while(contains_enough_data)
    {
        if(!header_Read && bytes >= sizeof(int))
        {
            QByteArray header = socket->readAll();
            size_of_data_to_read = 4;
            qDebug()<<"size of data : "<<size_of_data_to_read<<"  "<<header<<endl;
            header_Read = true;
        }
        else if(header_Read && bytes>=size_of_data_to_read)
        {
            QByteArray data = socket->readAll();
            qDebug()<<"Data is : "<<data;
            header_Read = false;
        }
        else
        {
            contains_enough_data = false;
        }
    }
}



void Client::sendInitialCommand()
{
    //Make an Initial JSON Command

    QJsonObject innercmd;
    innercmd["Command"] = "Hardware";
    innercmd["SubCmd"] = "ALL";
    innercmd["Data"] = "";

    QJsonObject InitialCommand;
    InitialCommand["Interface"] = "Hardware",
    InitialCommand.insert("Interface Data",innercmd);

    QJsonDocument j_doc(InitialCommand);
    QString j_str = j_doc.toJson();
    qDebug().noquote()<<"Initial Command : "<<j_str;

    //writeData(j_str);

    int length = j_str.length();
    qDebug()<<"len : "<<length;

    QByteArray int_arr(reinterpret_cast<const char *>(&length),sizeof(qint32));
    socket->write(int_arr);

    if(!j_str.isEmpty())
    {
        socket->write(j_str.toUtf8());
    }
}



void Client::on_pushButton_startSimulation_clicked()
{
    QJsonArray msg;
    //msg = [144,0,0,0,0,0,0,0,0,204];
    msg.append(144);
    msg.append(0);
    msg<<0<<0<<0<<0<<0<<0<<0<<204;

    QJsonArray dyn_data;

    QJsonObject msg_info;
    msg_info["Message Name"] = "P11_EXT_UART4_RX";
    msg_info.insert("Message",msg);
    msg_info["Dynamic"] = 0;
    msg_info.insert("DynData",dyn_data);

    QJsonArray message_info;
    message_info.append(msg_info);

    QJsonObject frame_info_obj;
    frame_info_obj["Frame ID"] = 0;
    frame_info_obj.insert("Message Info",message_info);

    QJsonArray frame_info_arr;
    frame_info_arr.append(frame_info_obj);

    QJsonObject data_obj;
    data_obj["Port"] = "COM3";
    data_obj["Baud Rate"] = 9600;
    data_obj["Data Bits"] = 8;
    data_obj["Parity"] = 0;
    data_obj["Stop Bits"] = 10;
    data_obj["Flow Control"] = 0;
    data_obj["Timeout"] = 10000;
    data_obj["Minor Frame Count"] = 1;
    data_obj["Minor Frame Freq"] = 80;
    //data_obj["Frame Info"] = frame_info_arr;
    data_obj.insert("Frame Info",frame_info_arr);

    QJsonArray data_arr;
    data_arr.append(data_obj);

    QJsonObject interface_data_obj;
    interface_data_obj["Command"] = "Configure";
    interface_data_obj["SubCmd"] = "Async";
    interface_data_obj["Data"] = data_arr;

    QJsonObject rootObj;
    rootObj["Interface"] = "Async";
    rootObj["Interface Data"] = interface_data_obj;

    QJsonDocument config_doc(rootObj);
    QString config_str = config_doc.toJson();

    qDebug().noquote()<<"Config Command : "<<config_str;
    writeData(config_str);

//    int length = config_str.length();
//    qDebug()<<"len : "<<length;

//    QByteArray int_arr(reinterpret_cast<const char *>(&length),sizeof(qint32));
//    socket->write(int_arr);

//    if(!config_str.isEmpty())
//    {
//        socket->write(config_str.toUtf8());
//        qDebug().noquote()<<"Config Command : "<<config_str;
//    }

    //Start Simulation
    QJsonObject obj;
    obj["Command"] = "Configure";
    obj["Interface"] = "Async";
    obj["Status"] = "Ok";
    obj["Board"] = "COM3";

    QJsonDocument start_doc(obj);
    QString start_str = start_doc.toJson();

    qDebug().noquote()<<"Start Command : "<<start_str;
    writeData(start_str);

//    int length1 = start_str.length();
//    qDebug()<<"len : "<<length1;

//    QByteArray int_arr1(reinterpret_cast<const char *>(&length1),sizeof(qint32));
//    socket->write(int_arr1);

//    if(!start_str.isEmpty())
//    {
//        socket->write(start_str.toUtf8());
//        qDebug().noquote()<<"Start Command : "<<start_str;
//    }
}

void Client::on_pushButton_stopSimulation_clicked()
{
    QJsonObject interface_data;
    interface_data["Command"] = "Stop";
    interface_data["SubCmd"] = "Async";

    QJsonArray data_arr;
    data_arr.append("COM3");

    interface_data["Data"] = data_arr;

    QJsonObject root_obj;
    root_obj["Interface"] = "Async";
    root_obj["Interface Data"] = interface_data;

    QJsonDocument stop_doc(root_obj);
    QString stop_cmd = stop_doc.toJson();

//    int length = stop_cmd.length();
//    qDebug()<<"len : "<<length;

//    QByteArray int_arr(reinterpret_cast<const char *>(&length),sizeof(qint32));
//    socket->write(int_arr);

//    if(!stop_cmd.isEmpty())
//    {
//        socket->write(stop_cmd.toUtf8());
//    }

    writeData(stop_cmd);
    qDebug().noquote()<<"Stop Command : "<<stop_cmd;
}

void Client::on_pushButton_updateSimulation_clicked()
{
    QJsonArray update_msg;
    update_msg<<144<<0<<0<<0<<0<<0<<0<<0<<0<<204;
    QJsonArray dynData;

    QJsonObject msg_info;
    msg_info["Message Name"] = "BU1_COM";
    msg_info["Message"] = update_msg;
    msg_info["Dynamic"] = 0;
    msg_info["DynData"] = dynData;

    QJsonArray msg_info_arr;
    msg_info_arr.append(msg_info);

    QJsonObject data;
    data["Port"] = "COM39";
    data["Message"] = msg_info_arr;

    QJsonArray data_arr;
    data_arr.append(data);

    QJsonObject InterfaceData;
    InterfaceData["Command"] = "Update";
    InterfaceData["SubCmd"] = "Async";
    InterfaceData["Data"] = data_arr;

    QJsonObject rootObj;
    rootObj["Interface"]  ="Async";
    rootObj["Interface Data"] = InterfaceData;

    QJsonDocument update_doc(rootObj);
    QString update_cmd = update_doc.toJson();

//    int length = update_cmd.length();
//    qDebug()<<"len : "<<length;

//    QByteArray int_arr(reinterpret_cast<const char *>(&length),sizeof(qint32));
//    socket->write(int_arr);

//    if(!update_cmd.isEmpty())
//    {
//        socket->write(update_cmd.toUtf8());
//    }
    writeData(update_cmd);
    qDebug().noquote()<<"update command : "<<update_cmd;
}


