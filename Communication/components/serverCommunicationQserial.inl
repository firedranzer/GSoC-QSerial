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
}

ServerCommunicationQSerial::~ServerCommunicationQSerial()
{
    this->m_running = false;

    if(isVerbose())
        msg_info(this) << "waiting for timeout";

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

void ServerCommunicationQSerial::sendData()
{
    std::string address = "tcp://*:";
    std::string port = this->d_port.getValueString();
    address.insert(address.length(), port);

    while (this->m_running)
    {
        serial = new QSerialPort(this);
        connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
        std::map<std::string, CommunicationSubscriber*> subscribersMap = getSubscribers();
        if (subscribersMap.size() == 0)
            continue;

        std::string messageStr;

        for (std::map<std::string, CommunicationSubscriber*>::iterator it = subscribersMap.begin(); it != subscribersMap.end(); it++)
        {
            CommunicationSubscriber* subscriber = it->second;
            ArgumentList argumentList = subscriber->getArgumentList();
            messageStr += subscriber->getSubject() + " ";

            try
            {
                for (ArgumentList::iterator itArgument = argumentList.begin(); itArgument != argumentList.end(); itArgument++ )
                    messageStr += createQSerialMessage(subscriber, *itArgument);

//                QSerial::message_t message(messageStr.length());
//                memcpy(message.data(), messageStr.c_str(), messageStr.length());

                bool status = serial->write(messageStr);
                if(!status)
                    msg_warning(this) << "Problem with communication";
            } catch(const std::exception& e) {
                if (isVerbose())
                    msg_info("ServerCommunicationQSerial") << e.what();
            }
            messageStr.clear();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(int(1000000.0/(double)this->d_refreshRate.getValue())));
    }
}

void ServerCommunicationQSerial::createQSerialMessage(CommunicationSubscriber *subscriber, std::string argument)
{
    std::stringstream messageStr;
    BaseData* data = fetchDataFromSenderBuffer(subscriber, argument);
    if (!data)
        throw std::invalid_argument("data is null");
    const AbstractTypeInfo *typeinfo = data->getValueTypeInfo();
    const void* valueVoidPtr = data->getValueVoidPtr();

    if (typeinfo->Container())
    {
        int nbRows = typeinfo->size();
        int nbCols  = typeinfo->size(data->getValueVoidPtr()) / typeinfo->size();
        messageStr << "matrix int:" << std::to_string(nbRows) << " int:" << std::to_string(nbCols) << " ";

        if( !typeinfo->Text() && !typeinfo->Scalar() && !typeinfo->Integer() )
        {
            msg_advice(data->getOwner()) << "BaseData_getAttr_value unsupported native type="<<data->getValueTypeString()<<" for data "<<data->getName()<<" ; returning string value" ;
            messageStr << "string:'" << (data->getValueString()) << "' ";
        }
        else if (typeinfo->Text())
            for (int i=0; i < nbRows; i++)
                for (int j=0; j<nbCols; j++)
                    messageStr << "string:" << typeinfo->getTextValue(valueVoidPtr,(i*nbCols) + j).c_str() << " ";
        else if (typeinfo->Scalar())
            for (int i=0; i < nbRows; i++)
                for (int j=0; j<nbCols; j++)
                    messageStr << "float:" << (float)typeinfo->getScalarValue(valueVoidPtr,(i*nbCols) + j) << " ";
        else if (typeinfo->Integer())
            for (int i=0; i < nbRows; i++)
                for (int j=0; j<nbCols; j++)
                    messageStr << "int:" << (int)typeinfo->getIntegerValue(valueVoidPtr,(i*nbCols) + j) << " ";
    }
    else
    {
        if( !typeinfo->Text() && !typeinfo->Scalar() && !typeinfo->Integer() )
        {
            msg_advice(data->getOwner()) << "BaseData_getAttr_value unsupported native type=" << data->getValueTypeString() << " for data "<<data->getName()<<" ; returning string value" ;
            messageStr << "string:'" << (data->getValueString()) << "' ";
        }
        if (typeinfo->Text())
        {
            messageStr << "string:'" << (data->getValueString()) << "' ";
        }
        else if (typeinfo->Scalar())
        {
            messageStr << "float:" << (data->getValueString()) << " ";
        }
        else if (typeinfo->Integer())
        {
            messageStr << "int:" << (data->getValueString()) << " ";
        }
    }
    delete data;
    return messageStr.str();
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
            if (info.description().compare("Arduino Leonardo") == 0)
            {
                serial = new QSerialPort(this);
                serial->setPort(info);
                serial->setBaudRate(QSerialPort::Baud9600);
                serial->setDataBits(QSerialPort::Data8);
                if (serial->open(QIODevice::ReadOnly))
                {
                    serial->setDataTerminalReady(true);
                    //                    serial->setRequestToSend(true);
                    if(serial->waitForReadyRead(2000))
                    {
                        QByteArray byteArray = serial->readAll();
                        processMessage(byteArray);
                    }
                    serial->close();
                }
            }
        }
    }
}

void ServerCommunicationQSerial::processMessage(QByteArray msg)
{
    std::string s = msg.toStdString();
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
    } catch (std::invalid_argument err)
    {
        std::cerr << "received corrupted datas ... " << std::endl;
    }
}

std::string ServerCommunicationQSerial::getArgumentValue(std::string value)
{
    std::string stringData = value;
    std::string returnValue;
    size_t pos = stringData.find(":"); // That's how QSerial messages could be. Type:value
    stringData.erase(0, pos+1);
    std::remove_copy(stringData.begin(), stringData.end(), std::back_inserter(returnValue), '\'');
    return returnValue;
}

std::string ServerCommunicationQSerial::getArgumentType(std::string value)
{
    std::string stringType = value;
    size_t pos = stringType.find(":"); // That's how QSerial messages could be. Type:value
    if (pos == std::string::npos)
        return "string";
    stringType.erase(pos, stringType.size()-1);
    return stringType;
}

}   /// namespace communication
}   /// namespace component
}   /// namespace sofa


#endif // SOFA_CONTROLLER_ServerCommunicationQSerial_INL
