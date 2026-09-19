#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

#include <functional> // std::function

class QNetworkAccessManager;

class LgNetCastClient : public QObject {
  Q_OBJECT
public:
  static constexpr int kPort = 8080;
  static constexpr qint64 kMinCommandIntervalMs = 200;

  enum Cmd {
    POWER = 1,
    NUMBER_0 = 2,
    NUMBER_1 = 3,
    NUMBER_2 = 4,
    NUMBER_3 = 5,
    NUMBER_4 = 6,
    NUMBER_5 = 7,
    NUMBER_6 = 8,
    NUMBER_7 = 9,
    NUMBER_8 = 10,
    NUMBER_9 = 11,
    UP = 12,
    DOWN = 13,
    LEFT = 14,
    RIGHT = 15,
    OK = 20,
    HOME_MENU = 21,
    BACK = 23,
    VOLUME_UP = 24,
    VOLUME_DOWN = 25,
    MUTE_TOGGLE = 26,
    CHANNEL_UP = 27,
    CHANNEL_DOWN = 28,
    BLUE = 29,
    GREEN = 30,
    RED = 31,
    YELLOW = 32,
    PLAY = 33,
    PAUSE = 34,
    STOP = 35,
    FAST_FORWARD = 36,
    REWIND = 37,
    SKIP_FORWARD = 38,
    SKIP_BACKWARD = 39,
    RECORD = 40,
    REPEAT = 42,
    LIVE_TV = 43,
    EPG = 44,
    INFO = 45,
    ASPECT_RATIO = 46,
    EXTERNAL_INPUT = 47,
    SHOW_SUBTITLE = 49,
    TELE_TEXT = 51,
    PREVIOUS_CHANNEL = 403,
    FAVORITE_CHANNEL = 404,
    QUICK_MENU = 405,
    AV_MODE = 410,
    SIMPLINK = 411,
    EXIT = 412,
    APPS = 417,
  };

  explicit LgNetCastClient(QString host, QObject *parent = nullptr);

  void setSession(const QString &sid) { m_session = sid; }
  QString session() const { return m_session; }

  void requestPairingKey();
  void confirmPairing(const QString &key);
  void sendCommand(int cmd);

signals:
  void pairingKeyResult(bool ok);
  void pairingConfirmResult(const QString &session); // пусто = ошибка
  void commandResult(bool ok);

private:
  void post(const QString &url, const QByteArray &body,
            std::function<void(int, QByteArray)> cb);
  void tryProcessNext();

  QString m_host;
  QString m_session;
  QNetworkAccessManager *m_nam = nullptr;

  QQueue<int> m_queue;
  bool m_inFlight = false;
  qint64 m_lastCommandAt = 0;
};