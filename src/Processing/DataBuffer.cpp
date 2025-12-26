#include "DataBuffer.h"

#include <QtMath>
#include <QMutexLocker>
#include <QThread>
#include <QtDebug>
#include <algorithm>

#include "TcpSocketWorker.h"

namespace
{
    constexpr int QUEUE_WAIT_MS = 10;
}

DataBuffer &DataBuffer::instance()
{
    static DataBuffer bufferInstance;
    return bufferInstance;
}

DataBuffer::DataBuffer()
{
    for (int i = 0; i < DATA_BUF_NUM; ++i)
    {
        m_buffers[i].resize(DATA_BUF_SIZE);
        m_buffers[i].fill(0);
        m_wrQueue.enqueue(i);
    }
    if (!m_wrQueue.isEmpty())
    {
        m_wrPtr = m_wrQueue.dequeue();
    }
}

QByteArray &DataBuffer::buffer(int index)
{
    return m_buffers[index];
}

void DataBuffer::addProcessData(quint16 X, quint16 Y, quint16 Z, quint16 A, quint16 B)
{
    addData(B, A, Z, Y, X, 0x00FF);
}

void DataBuffer::addProcessJumpData(quint16 X, quint16 Y, quint16 Z, quint16 A, quint16 B)
{
    addData(B, A, Z, Y, X, 0);
}

void DataBuffer::addProcessBegin()
{
    handleBegin();
    addData(0, 0, 0, 0, 0, 0xFF00, 0, 0);
}

void DataBuffer::addProcessEnd()
{
    addData(0, 0, 0, 0, 0, 0x1100, 0, 0);
}

void DataBuffer::setFreqData(int freq)
{
    handleBegin();
    const int cnt = 50000 / freq;
    addData(0xAA, 0, 0, 0, 0, 0xAA00, 0, 0);
    addData(static_cast<quint16>(cnt & 0xFFFF), static_cast<quint16>(cnt >> 16));
    addData(0xAA, 0, 0, 0, 0, 0x5500, 0, 0);
}

void DataBuffer::setPowerData(double power)
{
    if (power > 100.0)
    {
        power = 100.0;
    }
    power = 5.5366 + 2.67805 * power - 0.107836 * qPow(power, 2) + 0.00241519 * qPow(power, 3) -
            0.0000248153 * qPow(power, 4) + 0.0000000964112 * qPow(power, 5);

    const auto p = static_cast<quint16>(power * 65535.0 / 100.0);
    for (int i = 0; i < 10; ++i)
    {
        addData(p, 0, 0, 0, 0, 0xbb00, 0, 11451);
    }
    forceFill();
}

void DataBuffer::forceFill()
{
    auto &buf = m_buffers[m_wrPtr];
    if (m_ptr < DATA_BUF_SIZE)
    {
        std::fill(buf.begin() + m_ptr, buf.end(), 0);
    }
    m_ptr = DATA_BUF_SIZE;
    handleBufferFilled();
}

int DataBuffer::getWriteBuf()
{
    QMutexLocker locker(&m_queueMutex);
    while (m_wrQueue.isEmpty())
    {
        m_wrAvailable.wait(&m_queueMutex, QUEUE_WAIT_MS);
    }
    return m_wrQueue.dequeue();
}

int DataBuffer::getReadBuf()
{
    QMutexLocker locker(&m_queueMutex);
    while (m_rdQueue.isEmpty())
    {
        m_rdAvailable.wait(&m_queueMutex, QUEUE_WAIT_MS);
    }
    return m_rdQueue.dequeue();
}

void DataBuffer::writeEnd(int p)
{
    QMutexLocker locker(&m_queueMutex);
    if (m_rdQueue.size() >= DATA_BUF_NUM)
    {
        qWarning() << "读队列异常";
    }
    m_rdQueue.enqueue(p);
    m_rdAvailable.wakeOne();
}

void DataBuffer::readEnd(int p)
{
    QMutexLocker locker(&m_queueMutex);
    if (m_wrQueue.size() >= DATA_BUF_NUM)
    {
        qWarning() << "写队列异常";
    }
    m_wrQueue.enqueue(p);
    m_wrAvailable.wakeOne();
}

void DataBuffer::addData(quint16 arg1, quint16 arg2, quint16 arg3, quint16 arg4, quint16 arg5, quint16 arg6,
                         quint16 arg7, quint16 arg8)
{
    const quint16 args[8] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
    auto &buf = m_buffers[m_wrPtr];
    for (quint16 value : args)
    {
        if (m_ptr + 2 > DATA_BUF_SIZE)
        {
            handleBufferFilled();
            if (m_ptr + 2 > DATA_BUF_SIZE)
            {
                return;
            }
        }
        buf[m_ptr++] = static_cast<char>(value & 0xFF);
        buf[m_ptr++] = static_cast<char>(value >> 8);
    }
    handleBufferFilled();
}

void DataBuffer::handleBufferFilled()
{
    if (m_ptr >= DATA_BUF_SIZE)
    {
        if (!m_tcpThreadStarted.exchange(true))
        {
            qInfo() << "启动 TCP 线程";
            TcpSocketWorker::instance().ensureRunning();
        }
        qInfo() << "写入成功，缓冲区:" << m_wrPtr;
        writeEnd(m_wrPtr);
        m_wrPtr = getWriteBuf();
        m_ptr = 0;
    }
}

void DataBuffer::handleBegin()
{
    for (int i = 0; i < 2; ++i)
    {
        addData(0, 0, 0, 0, 0, 0xFF00, 0, 0);
        addProcessJumpData(0x8000, 0x8000, 0x8000, 0x8000, 0x8000);
        addData(0, 0, 0, 0, 0, 0x1100, 0, 0);
        forceFill();
    }
}