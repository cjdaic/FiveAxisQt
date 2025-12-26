#pragma once

#include <atomic>
#include <thread>

class TcpSocketWorker
{
public:
    static TcpSocketWorker &instance();

    void ensureRunning();
    void stop();

private:
    TcpSocketWorker() = default;
    void run();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
};