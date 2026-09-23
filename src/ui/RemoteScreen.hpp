#pragma once

#include <optional>

#include <QPointer>
#include <QWidget>

#include <lazytv/client.hpp>

#include "ui/widgets/StatusBar.hpp"

class QTcpSocket;

namespace lazytv { class AppContainer; }

class QStackedWidget;
class QLabel;
class QTimer;

class RemoteScreen : public QWidget {
    Q_OBJECT
public:
    explicit RemoteScreen(lazytv::AppContainer* container, QWidget* parent = nullptr);

signals:
    void openSettings();
    void disconnected();

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* buildMainPage();
    QWidget* buildNumbersPage();

    void sendCommand(lazytv::Client::Command cmd);
    void onCommandResult(bool ok);
    void recomputeStatus();
    void updateErrorBannerStyle();

    /** Асинхронно проверяет доступность порта ТВ (TCP 8080). */
    void checkReachability();

    lazytv::AppContainer* m_container = nullptr;
    StatusBar*      m_statusBar = nullptr;
    QStackedWidget* m_pages     = nullptr;
    QLabel*         m_errorBanner = nullptr;
    QTimer*         m_statusTimer = nullptr;
    QTimer*         m_reachabilityTimer = nullptr;

    /** Активный probe-сокет. QPointer — чтобы не держать висячий указатель. */
    QPointer<QTcpSocket> m_probeSocket;

    /**
     * Результат последней проверки порта.
     * nullopt = ещё не проверяли, true = порт открыт, false = порт закрыт.
     */
    std::optional<bool> m_reachable;

    ConnectionStatus m_status = ConnectionStatus::Stale;
    qint64 m_lastSuccessAt = 0;
    bool   m_lastAttemptFailed = false;
};