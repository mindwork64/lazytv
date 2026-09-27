#pragma once

#include <QHash>
#include <QWidget>

#include <lazytv/client.hpp>

#include "ui/widgets/StatusBar.hpp"

namespace lazytv { class AppContainer; }

class QStackedWidget;
class QLabel;
class QTimer;
class QKeyEvent;
class HotkeyHandler;
class IconButton;
class KeypadKey;
class DPad;
class RockerColumn;

class RemoteScreen : public QWidget {
    Q_OBJECT
public:
    explicit RemoteScreen(lazytv::AppContainer* container, QWidget* parent = nullptr);

    /** Включает/выключает горячие клавиши (настройка «Горячие клавиши»). */
    void setHotkeysEnabled(bool v);

signals:
    void openSettings();
    void disconnected();

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QWidget* buildMainPage();
    QWidget* buildNumbersPage();

    void sendCommand(lazytv::Client::Command cmd);
    void flashButton(lazytv::Client::Command cmd);
    void onCommandResult(bool ok);
    void recomputeStatus();
    void updateErrorBannerStyle();

    lazytv::AppContainer* m_container = nullptr;
    HotkeyHandler*  m_hotkeys   = nullptr;
    StatusBar*      m_statusBar = nullptr;
    QStackedWidget* m_pages     = nullptr;
    QLabel*         m_errorBanner = nullptr;
    QTimer*         m_statusTimer = nullptr;

    // Кнопки для подсветки по горячей клавише: у IconButton и KeypadKey
    // нет собственной привязки к команде, поэтому соответствие хранит экран.
    QHash<int, IconButton*> m_iconByCmd;
    QHash<int, KeypadKey*>  m_keyByDigit;
    DPad*         m_dpad = nullptr;
    RockerColumn* m_vol  = nullptr;
    RockerColumn* m_ch   = nullptr;

    ConnectionStatus m_status = ConnectionStatus::Stale;
    qint64 m_lastSuccessAt = 0;
    bool   m_lastAttemptFailed = false;
};