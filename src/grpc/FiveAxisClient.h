#pragma once

#include <QObject>
#include <QThread>
#include <QUrl>
#include <QString>
#include <QMetaType>

#include <grpcpp/grpcpp.h>

#include "five_axis.grpc.pb.h"

Q_DECLARE_METATYPE(LineData)
Q_DECLARE_METATYPE(RectangleData)
Q_DECLARE_METATYPE(CircleData)
Q_DECLARE_METATYPE(EllipseData)
Q_DECLARE_METATYPE(DelayData)
Q_DECLARE_METATYPE(FreqData)

class FiveAxisWorker : public QObject {
    Q_OBJECT
public slots:
    QString connectToServer(const QUrl& endpoint);
    QString channelStateString() const;
    void processLine(const LineData& request);
    void processRectangle(const RectangleData& request);
    void processCircle(const CircleData& request);
    void processEllipse(const EllipseData& request);
    void setDelay(const DelayData& request);
    void setLaserFreq(const FreqData& request);

signals:
    void replyReceived(const QString& operation, const QString& message);
    void errorReceived(const QString& operation, int code, const QString& message);

private:
    void emitNotConnected(const QString& operation);
    void emitException(const QString& operation, const std::exception& ex);
    void handleStatus(const QString& operation, const grpc::Status& status, const ServerReply& reply);

    QString describeState(grpc_connectivity_state state) const;

    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<FiveAxis::FiveAxis::Stub> m_stub;
};

class FiveAxisClient : public QObject {
    Q_OBJECT
public:
    explicit FiveAxisClient(QObject* parent = nullptr);
    ~FiveAxisClient() override;

    QString connectToServer(const QUrl& endpoint);
    QString channelStateString() const;
    void processLine(const LineData& request);
    void processRectangle(const RectangleData& request);
    void processCircle(const CircleData& request);
    void processEllipse(const EllipseData& request);
    void setDelay(const DelayData& request);
    void setLaserFreq(const FreqData& request);

signals:
    void replyReceived(const QString& operation, const QString& message);
    void errorReceived(const QString& operation, int code, const QString& message);

private:
    FiveAxisWorker* m_worker;
    QThread m_workerThread;
};