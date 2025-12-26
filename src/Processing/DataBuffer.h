#pragma once

#include <array>
#include <atomic>

#include <QByteArray>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QtGlobal>

class TcpSocketWorker;

class DataBuffer
{
public:
    static DataBuffer &instance();

    QByteArray &buffer(int index);

    void addProcessData(quint16 X, quint16 Y, quint16 Z, quint16 A, quint16 B);
    void addProcessJumpData(quint16 X, quint16 Y, quint16 Z, quint16 A, quint16 B);
    void addProcessBegin();
    void addProcessEnd();
    void setFreqData(int freq);
    void setPowerData(double power);
    void forceFill();

    int getWriteBuf();
    int getReadBuf();
    void writeEnd(int p);
    void readEnd(int p);

private:
    DataBuffer();
    void addData(quint16 arg1 = 0, quint16 arg2 = 0, quint16 arg3 = 0, quint16 arg4 = 0,
                 quint16 arg5 = 0, quint16 arg6 = 0, quint16 arg7 = 0, quint16 arg8 = 0);
    void handleBufferFilled();
    void handleBegin();

    static constexpr int DATA_BUF_NUM = 2;
    static constexpr int DATA_BUF_SIZE = 1'600'000;

    std::array<QByteArray, DATA_BUF_NUM> m_buffers{};
    QQueue<int> m_wrQueue;
    QQueue<int> m_rdQueue;
    int m_wrPtr{0};
    int m_ptr{0};
    QMutex m_queueMutex;
    QWaitCondition m_wrAvailable;
    QWaitCondition m_rdAvailable;
    std::atomic<bool> m_tcpThreadStarted{false};
};