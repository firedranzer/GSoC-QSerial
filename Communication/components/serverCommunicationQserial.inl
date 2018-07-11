#ifndef SOFA_CONTROLLER_ServerCommunicationQSerial_INL
#define SOFA_CONTROLLER_ServerCommunicationQSerial_INL

#include "serverCommunicationQSerial.h"
#include <Communication/components/CommunicationSubscriber.h>

namespace sofa
{

namespace component
{

namespace communication
{

ServerCommunicationQSerial::ServerCommunicationQSerial()
    : Inherited()
{
    m_thread = std::thread(&ServerCommunicationQSerial::run, this);
}

ServerCommunicationQSerial::~ServerCommunicationQSerial()
{
    this->m_running = false;

    if(isVerbose())
        msg_info(this) << "waiting for timeout";

    if(m_thread.joinable())
    m_thread.join();
    Inherited::closeCommunication();
}

ServerCommunicationQSerial::QSerialDataFactory* ServerCommunicationQSerial::getFactoryInstance(){
    static QSerialDataFactory* s_localfactory = nullptr ;
    if(s_localfactory==nullptr)
        s_localfactory = new ServerCommunicationQSerial::QSerialDataFactory() ;
    return s_localfactory ;
}

void ServerCommunicationQSerial::initTypeFactory()
{
    getFactoryInstance()->registerCreator("QSerialfloat", new DataCreator<float>());
    getFactoryInstance()->registerCreator("QSerialdouble", new DataCreator<double>());
    getFactoryInstance()->registerCreator("QSerialint", new DataCreator<int>());
    getFactoryInstance()->registerCreator("QSerialstring", new DataCreator<std::string>());

    getFactoryInstance()->registerCreator("matrixfloat", new DataCreator<FullMatrix<float>>());
    getFactoryInstance()->registerCreator("matrixdouble", new DataCreator<FullMatrix<double>>());
    getFactoryInstance()->registerCreator("matrixint", new DataCreator<FullMatrix<int>>());
}

std::string ServerCommunicationQSerial::defaultDataType()
{
    return "string";
}

/******************************************************************************
*                                                                             *
* SEND PART                                                                   *
*                                                                             *
******************************************************************************/

void ServerCommunicationQSerial::sendData(const QByteArray &data)
{
    while(m_running)
    {
        foreach ( const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            QSerialPort serial;
            serial.setPort(info);
            serial.setBaudRate(QSerialPort::Baud9600);
            serial.setDataBits(QSerialPort::Data8);
            if (serial.open(QIODevice::ReadOnly))
            {
                serial.setDataTerminalReady(true);
                if(serial.waitForReadyRead(2000))
                {
                    serial.write(data);
                }
                serial.close();
            }
        }
    }
}

/******************************************************************************
*                                                                             *
* RECEIVE PART                                                                *
*                                                                             *
******************************************************************************/

void ServerCommunicationQSerial::receiveData()
{
    while(m_running)
    {
        foreach ( const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            QSerialPort serial;
            serial.setPort(info);
            serial.setBaudRate(QSerialPort::Baud9600);
            serial.setDataBits(QSerialPort::Data8);
            if (serial.open(QIODevice::ReadOnly))
            {
                serial.setDataTerminalReady(true);
                //                    serial.setRequestToSend(true);
                if(serial.waitForReadyRead(2000))
                {
                    QByteArray byteArray = serial.readAll();
                    parseData(byteArray);
                }
                serial.close();
            }
        }
    }
}

void ServerCommunicationQSerial::processMessage(QByteArray msg)
{
    std::string s = message.toStdString();
    std::cout << s << std::endl;
    std::string delimiter = " ";

    std::vector<std::string> arguments;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        arguments.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    arguments.push_back(s);

    int id = -1, value = -1, tmp = -1;
    if (arguments.size() != 2)
        return;

    try
    {
        foreach (s, arguments)
        {
            tmp = std::stoi(s.substr(s.find("=") + 1));
            if (s.find("id") != std::string::npos)
            id = tmp;
            if (s.find("value") != std::string::npos)
            value = tmp;
        }
        if (id != -1 && value != -1)
            emit volumeChanged(id, value);
    } catch (std::invalid_argument err)
    {
        std::cerr << "received corrupted datas ... " << std::endl;
    }
}
}   //communication
}   //component
}   //sofa
