#include "TcpSocketWorker.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QThread>
#include <QtDebug>

#include "DataBuffer.h"

namespace {
    constexpr auto HOST = "192.168.1.10";
    constexpr quint16 PORT = 7;
    constexpr int READ_SIZE = 128;
}

TcpSocketWorker& TcpSocketWorker::instance() {
    static TcpSocketWorker worker;
    return worker;
}

void TcpSocketWorker::ensureRunning() {
    if (m_running.load()) {
        return;
    }
    m_stopRequested.store(false);
    m_thread = std::thread([this]() { run(); });
    m_thread.detach();
}

void TcpSocketWorker::stop() {
    m_stopRequested.store(true);
}

void TcpSocketWorker::run() {
    m_running.store(true);
    while (!m_stopRequested.load()) {
        QTcpSocket socket;
        socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);

        while (!m_stopRequested.load()) {
            socket.connectToHost(QHostAddress(QString::fromUtf8(HOST)), PORT);
            if (socket.waitForConnected(1000)) {
                break;
            }
            qWarning() << "TCP Failed" << socket.errorString();
            socket.abort();
            QThread::msleep(100);
        }

        if (m_stopRequested.load()) {
            break;
        }

        while (socket.state() == QAbstractSocket::ConnectedState && !m_stopRequested.load()) {
            if (!socket.waitForReadyRead(-1)) {
                break;
            }

            QByteArray inbound;
            while (inbound.size() < READ_SIZE && socket.state() == QAbstractSocket::ConnectedState) {
                const auto chunk = socket.read(READ_SIZE - inbound.size());
                if (!chunk.isEmpty()) {
                    inbound.append(chunk);
                }
                if (inbound.size() >= READ_SIZE) {
                    break;
                }
                if (!socket.waitForReadyRead(1000)) {
                    break;
                }
            }

            const int rdPtr = DataBuffer::instance().getReadBuf();
            auto& buf = DataBuffer::instance().buffer(rdPtr);

            qint64 offset = 0;
            while (offset < buf.size() && socket.state() == QAbstractSocket::ConnectedState) {
                const auto written = socket.write(buf.constData() + offset, buf.size() - offset);
                if (written <= 0) {
                    break;
                }
                offset += written;
                if (!socket.waitForBytesWritten(-1)) {
                    break;
                }
            }
            socket.flush();
            DataBuffer::instance().readEnd(rdPtr);
        }

        socket.disconnectFromHost();
        socket.waitForDisconnected(500);
    }
    m_running.store(false);
}