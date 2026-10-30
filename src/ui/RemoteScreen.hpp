#pragma once

#include <QHash>
#include <QPixmap>
#include <QPoint>
#include <QVector>
#include <QWidget>

#include <lazytv/client.hpp>

#include "ui/widgets/StatusBar.hpp"

namespace lazytv { class AppContainer; }

class QStackedWidget;
class QLabel;
class QListWidget;
class QPushButton;
class QTimer;
class QKeyEvent;
class QHideEvent;
class QResizeEvent;
class HotkeyHandler;
class IconButton;
class KeypadKey;
class DPad;
class RockerColumn;
class NowPlayingBar;

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
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QWidget* buildMainPage();
    QWidget* buildNumbersPage();
    QWidget* buildChannelsPage();
    QWidget* buildScreenPage();

    void sendCommand(lazytv::Client::Command cmd);
    void flashButton(lazytv::Client::Command cmd);
    void onCommandResult(bool ok);
    void recomputeStatus();
    void updateErrorBannerStyle();

    void connectClientSignals(lazytv::Client* client);
    void refreshInfo();
    void scheduleInfoRefresh(int delayMs);
    void refreshChannelList();
    void refreshScreenImage();
    void updateScreenImage();
    void openChannelsPage();
    void openScreenPage();
    void showMoreMenu(const QPoint& globalPos);

    lazytv::AppContainer* m_container = nullptr;
    lazytv::Client* m_connectedClient = nullptr;
    HotkeyHandler*  m_hotkeys   = nullptr;
    StatusBar*      m_statusBar = nullptr;
    NowPlayingBar*  m_nowPlaying = nullptr;
    QStackedWidget* m_pages     = nullptr;
    QLabel*         m_errorBanner = nullptr;
    QTimer*         m_statusTimer = nullptr;
    QTimer*         m_infoTimer = nullptr;
    QTimer*         m_refreshTimer = nullptr;

    // Кнопки для подсветки по горячей клавише: у IconButton и KeypadKey
    // нет собственной привязки к команде, поэтому соответствие хранит экран.
    QHash<int, IconButton*> m_iconByCmd;
    QHash<int, KeypadKey*>  m_keyByDigit;
    DPad*         m_dpad = nullptr;
    RockerColumn* m_vol  = nullptr;
    RockerColumn* m_ch   = nullptr;

    QListWidget* m_channelList = nullptr;
    QLabel*      m_channelListHint = nullptr;
    QPushButton* m_channelRefresh = nullptr;
    QVector<lazytv::ChannelInfo> m_channels;

    QLabel*      m_screenImage = nullptr;
    QLabel*      m_screenHint = nullptr;
    QPushButton* m_screenRefresh = nullptr;
    QPixmap      m_screenPixmap;

    ConnectionStatus m_status = ConnectionStatus::Stale;
    qint64 m_lastSuccessAt = 0;
    bool   m_lastAttemptFailed = false;
};