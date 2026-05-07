#include "device_base.h"

DeviceBase::DeviceBase(QObject *parent)
    : QObject(parent)
    , m_status(DeviceStatus::Disconnected)
    , m_deviceName("Unknown Device")
{
}

DeviceBase::~DeviceBase()
{
}

void DeviceBase::setStatus(DeviceStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}

void DeviceBase::setError(const QString &error)
{
    m_lastError = error;
    setStatus(DeviceStatus::Error);
    emit errorOccurred(error);
}

void DeviceBase::setDeviceName(const QString &name)
{
    m_deviceName = name;
}
