#include "serial_port_base.h"
#include <QDebug>
#include <QThread>

SerialPortBase::SerialPortBase(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_readTimeout(Constants::Serial::READ_TIMEOUT_MS)
    , m_writeTimeout(Constants::Serial::WRITE_TIMEOUT_MS)
{
    connect(m_serialPort, &QSerialPort::readyRead, 
            this, &SerialPortBase::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, 
            this, &SerialPortBase::onErrorOccurred);
}

SerialPortBase::~SerialPortBase()
{
    closePort();
}

bool SerialPortBase::openPort(const SerialConfig &config, int maxRetry)
{
    if (m_serialPort->isOpen()) {
        closePort();
    }
    
    m_config = config;
    
    for (int i = 0; i < maxRetry; ++i) {
        m_serialPort->setPortName(config.portName);
        m_serialPort->setBaudRate(config.baudRate);
        m_serialPort->setDataBits(config.dataBits);
        m_serialPort->setParity(config.parity);
        m_serialPort->setStopBits(config.stopBits);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
        
        if (m_serialPort->open(QIODevice::ReadWrite)) {
            qDebug() << "串口打开成功:" << config.portName;
            emit connected();
            return true;
        }
        
        if (i < maxRetry - 1) {
            qDebug() << "串口打开失败，重试中..." << (i + 1) << "/" << maxRetry;
            QThread::msleep(Constants::Serial::RETRY_DELAY_MS);
        }
    }
    
    QString error = QString("串口打开失败: %1 - %2")
                    .arg(config.portName)
                    .arg(m_serialPort->errorString());
    qDebug() << error;
    emit errorOccurred(error);
    return false;
}

bool SerialPortBase::openPort(const QString &portName, 
                              qint32 baudRate,
                              QSerialPort::DataBits dataBits,
                              QSerialPort::Parity parity,
                              QSerialPort::StopBits stopBits)
{
    SerialConfig config;
    config.portName = portName;
    config.baudRate = baudRate;
    config.dataBits = dataBits;
    config.parity = parity;
    config.stopBits = stopBits;
    return openPort(config);
}

void SerialPortBase::closePort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qDebug() << "串口已关闭:" << m_serialPort->portName();
        emit disconnected();
    }
}

bool SerialPortBase::isOpen() const
{
    return m_serialPort->isOpen();
}

qint64 SerialPortBase::writeData(const QByteArray &data)
{
    if (!m_serialPort->isOpen()) {
        emit errorOccurred("串口未打开");
        return -1;
    }
    
    qint64 bytesWritten = m_serialPort->write(data);
    
    // 等待数据写入完成
    if (!m_serialPort->waitForBytesWritten(m_writeTimeout)) {
        emit errorOccurred("写入数据超时");
        return -1;
    }
    
    return bytesWritten;
}

QByteArray SerialPortBase::readData()
{
    if (!m_serialPort->isOpen()) {
        emit errorOccurred("串口未打开");
        return QByteArray();
    }
    
    // 等待数据可读
    if (m_serialPort->waitForReadyRead(m_readTimeout)) {
        return m_serialPort->readAll();
    }
    
    return QByteArray();
}

QByteArray SerialPortBase::readAll()
{
    return m_serialPort->readAll();
}

QString SerialPortBase::portName() const
{
    return m_serialPort->portName();
}

SerialConfig SerialPortBase::getConfig() const
{
    return m_config;
}

QStringList SerialPortBase::availablePorts()
{
    QStringList portList;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        portList << info.portName();
    }
    return portList;
}

void SerialPortBase::setReadTimeout(int msec)
{
    m_readTimeout = msec;
}

void SerialPortBase::setWriteTimeout(int msec)
{
    m_writeTimeout = msec;
}

void SerialPortBase::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialPortBase::onErrorOccurred(QSerialPort::SerialPortError error)
{
    // 忽略NoError
    if (error == QSerialPort::NoError) {
        return;
    }
    
    QString errorString = m_serialPort->errorString();
    qDebug() << "串口错误:" << errorString;
    emit errorOccurred(errorString);
}
