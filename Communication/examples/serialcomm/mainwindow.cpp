#include<QDebug>
#include<QtSerialPort/QtSerialPort>

QWidget::Widget(QWidget*parent):
QWidget(parent),
ui(newUi::Widget)
{
ui->setupUi(this);

QSerialPort*port=newQSerialPort(this);
charc1,c2;

port->setPortName("COM4");
if(port->open(QIODevice::ReadWrite))
{
port->setBaudRate(QSerialPort::Baud9600);
port->setDataBits(QSerialPort::Data8);
port->setStopBits(QSerialPort::OneStop);
port->setParity(QSerialPort::NoParity);
port->setFlowControl(QSerialPort::NoFlowControl);
qDebug()<<"portopen";
}
else
qDebug()<<"portopenerror";

port->clear();
c1='A';
port->write(&c1,1);
port->waitForReadyRead(3000);
port->read(&c2,1);
qDebug()<<c2;

c1='B';
port->write(&c1,1);
#if1
port->waitForReadyRead(3000);
#else
port->waitForReadyRead(100);
port->waitForReadyRead(100);
#endif
port->read(&c2,1);
qDebug()<<c2;
qDebug()<<"done";
}
