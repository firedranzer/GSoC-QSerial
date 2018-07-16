#include <QCoreApplication>
#include <QSerialPort>
#include <QDebug>
QSerialPort serial;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    serial.setPortName("/dev/ttys000");
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    serial.open(QIODevice::ReadWrite);
    serial.write("****Hello-World****");
    serial.close();
    return a.exec();
}
