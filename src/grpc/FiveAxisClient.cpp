#include "FiveAxisClient.h"

#include <exception>
#include <chrono>
#include <QMetaType>
#include <grpcpp/grpcpp.h>

FiveAxisClient::FiveAxisClient(QObject* parent)
    : QObject(parent)
    , m_worker(new FiveAxisWorker()) {
    qRegisterMetaType<LineData>("LineData");
    qRegisterMetaType<RectangleData>("RectangleData");
    qRegisterMetaType<CircleData>("CircleData");
    qRegisterMetaType<EllipseData>("EllipseData");
    qRegisterMetaType<DelayData>("DelayData");
    qRegisterMetaType<FreqData>("FreqData");

    m_worker->moveToThread(&m_workerThread);
    connect(m_worker, &FiveAxisWorker::replyReceived, this, &FiveAxisClient::replyReceived);
    connect(m_worker, &FiveAxisWorker::errorReceived, this, &FiveAxisClient::errorReceived);
    m_workerThread.start();
}

FiveAxisClient::~FiveAxisClient() {
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_worker;
}

QString FiveAxisClient::connectToServer(const QUrl& endpoint) {
    QString details;
    QMetaObject::invokeMethod(
        m_worker,
        "connectToServer",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, details),
        Q_ARG(QUrl, endpoint));
    return details;
}

QString FiveAxisClient::channelStateString() const {
    QString state;
    QMetaObject::invokeMethod(
        m_worker,
        "channelStateString",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, state));
    return state;
}

void FiveAxisClient::processLine(const LineData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "processLine",
        Qt::QueuedConnection,
        Q_ARG(LineData, request));
}

void FiveAxisClient::processRectangle(const RectangleData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "processRectangle",
        Qt::QueuedConnection,
        Q_ARG(RectangleData, request));
}

void FiveAxisClient::processCircle(const CircleData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "processCircle",
        Qt::QueuedConnection,
        Q_ARG(CircleData, request));
}

void FiveAxisClient::processEllipse(const EllipseData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "processEllipse",
        Qt::QueuedConnection,
        Q_ARG(EllipseData, request));
}

void FiveAxisClient::setDelay(const DelayData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "setDelay",
        Qt::QueuedConnection,
        Q_ARG(DelayData, request));
}

void FiveAxisClient::setLaserFreq(const FreqData& request) {
    QMetaObject::invokeMethod(
        m_worker,
        "setLaserFreq",
        Qt::QueuedConnection,
        Q_ARG(FreqData, request));
}

QString FiveAxisWorker::connectToServer(const QUrl& endpoint) {
    QString address = endpoint.toString();
    if (address.startsWith(QStringLiteral("grpc://"))) {
        address = address.mid(QStringLiteral("grpc://").size());
    }
    m_channel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
    const grpc_connectivity_state initialState = m_channel->GetState(true);
    const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
    const bool connected = m_channel->WaitForConnected(deadline);
    const grpc_connectivity_state finalState = m_channel->GetState(false);
    m_stub = FiveAxis::FiveAxis::NewStub(m_channel);

    return QStringLiteral("Endpoint: %1\n"
        " - Scheme: %2\n"
        " - Host: %3\n"
        " - Port: %4\n"
        " - Target: %5\n"
        " - Initial state: %6\n"
        " - Connected within 2s: %7\n"
        " - Final state: %8")
        .arg(endpoint.toString(),
            endpoint.scheme(),
            endpoint.host(),
            QString::number(endpoint.port(-1)),
            address,
            describeState(initialState),
            connected ? QStringLiteral("true") : QStringLiteral("false"),
            describeState(finalState));
}

QString FiveAxisWorker::channelStateString() const {
    if (!m_channel) {
        return QStringLiteral("not initialized");
    }
    return describeState(m_channel->GetState(false));
}

void FiveAxisWorker::processLine(const LineData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("ProcessLine"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->ProcessLine(&context, request, &reply);
        handleStatus(QStringLiteral("ProcessLine"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("ProcessLine"), ex);
    }
}

void FiveAxisWorker::processRectangle(const RectangleData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("ProcessRectangle"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->ProcessRectangle(&context, request, &reply);
        handleStatus(QStringLiteral("ProcessRectangle"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("ProcessRectangle"), ex);
    }
}

void FiveAxisWorker::processCircle(const CircleData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("ProcessCircle"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->ProcessCircle(&context, request, &reply);
        handleStatus(QStringLiteral("ProcessCircle"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("ProcessCircle"), ex);
    }
}

void FiveAxisWorker::processEllipse(const EllipseData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("ProcessEllipse"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->ProcessEllipse(&context, request, &reply);
        handleStatus(QStringLiteral("ProcessEllipse"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("ProcessEllipse"), ex);
    }
}

void FiveAxisWorker::setDelay(const DelayData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("SetDelay"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->SetDelay(&context, request, &reply);
        handleStatus(QStringLiteral("SetDelay"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("SetDelay"), ex);
    }
}

void FiveAxisWorker::setLaserFreq(const FreqData& request) {
    if (!m_stub) {
        emitNotConnected(QStringLiteral("SetLaserFreq"));
        return;
    }
    try {
        ServerReply reply;
        grpc::ClientContext context;
        const auto status = m_stub->SetLaserFreq(&context, request, &reply);
        handleStatus(QStringLiteral("SetLaserFreq"), status, reply);
    }
    catch (const std::exception& ex) {
        emitException(QStringLiteral("SetLaserFreq"), ex);
    }
}

void FiveAxisWorker::emitNotConnected(const QString& operation) {
    emit errorReceived(operation, -1, QStringLiteral("Not connected to gRPC service"));
}

void FiveAxisWorker::emitException(const QString& operation, const std::exception& ex) {
    emit errorReceived(operation, -2, QString::fromLocal8Bit(ex.what()));
}

void FiveAxisWorker::handleStatus(const QString& operation, const grpc::Status& status, const ServerReply& reply) {
    if (status.ok()) {
        emit replyReceived(operation, QString::fromStdString(reply.message()));
    }
    else {
        emit errorReceived(operation, status.error_code(), QString::fromStdString(status.error_message()));
    }
}

QString FiveAxisWorker::describeState(grpc_connectivity_state state) const {
    switch (state) {
    case grpc_connectivity_state::GRPC_CHANNEL_IDLE:
        return QStringLiteral("IDLE");
    case grpc_connectivity_state::GRPC_CHANNEL_CONNECTING:
        return QStringLiteral("CONNECTING");
    case grpc_connectivity_state::GRPC_CHANNEL_READY:
        return QStringLiteral("READY");
    case grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE:
        return QStringLiteral("TRANSIENT_FAILURE");
    case grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN:
        return QStringLiteral("SHUTDOWN");
    }
    return QStringLiteral("UNKNOWN");
}