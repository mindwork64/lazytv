#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

#include <functional>

#include <lgremote/lgremote_export.h>

class QNetworkAccessManager;

namespace lgremote {

/**
 * Клиент LG NetCast.
 *
 * Реализует протокол удалённого управления телевизорами LG (2010–2014,
 * серии LB/LA/LN/LV/LS/LE/UB/UC/UF). Общается с телевизором по локальной
 * сети, HTTP/XML, порт 8080.
 *
 * Типичный сценарий использования:
 *   1. Client client(ip);
 *   2. connect(...pairingKeyResult...) + client.requestPairingKey();
 *   3. connect(...pairingConfirmResult...) + client.confirmPairing(key);
 *   4. Сохранить session, при следующих запусках — client.setSession(session);
 *   5. client.sendCommand(Client::Command::Power); // и т.д.
 */
class LGREMOTE_EXPORT Client : public QObject {
  Q_OBJECT
public:
  static constexpr int kPort = 8080;
  static constexpr qint64 kMinCommandIntervalMs = 200;

  enum class Command : int {
    Power = 1,
    Number0 = 2,
    Number1 = 3,
    Number2 = 4,
    Number3 = 5,
    Number4 = 6,
    Number5 = 7,
    Number6 = 8,
    Number7 = 9,
    Number8 = 10,
    Number9 = 11,
    Up = 12,
    Down = 13,
    Left = 14,
    Right = 15,
    Ok = 20,
    HomeMenu = 21,
    Back = 23,
    VolumeUp = 24,
    VolumeDown = 25,
    MuteToggle = 26,
    ChannelUp = 27,
    ChannelDown = 28,
    Blue = 29,
    Green = 30,
    Red = 31,
    Yellow = 32,
    Play = 33,
    Pause = 34,
    Stop = 35,
    FastForward = 36,
    Rewind = 37,
    SkipForward = 38,
    SkipBackward = 39,
    Record = 40,
    Repeat = 42,
    LiveTv = 43,
    Epg = 44,
    Info = 45,
    AspectRatio = 46,
    ExternalInput = 47,
    ShowSubtitle = 49,
    TeleText = 51,
    PreviousChannel = 403,
    FavoriteChannel = 404,
    QuickMenu = 405,
    AvMode = 410,
    SimpLink = 411,
    Exit = 412,
    Apps = 417,
  };
  Q_ENUM(Command)

  /** Хост — IP-адрес или hostname телевизора. */
  explicit Client(QString host, QObject *parent = nullptr);

  // --- Сессия -----------------------------------------------------------

  void setSession(const QString &sid) { m_session = sid; }
  QString session() const { return m_session; }

  // --- Сопряжение -------------------------------------------------------

  /** Запрос кода на экране ТВ. Результат — сигнал pairingKeyResult. */
  void requestPairingKey();

  /** Подтверждение кода. Результат — сигнал pairingConfirmResult. */
  void confirmPairing(const QString &key);

  // --- Управление -------------------------------------------------------

  /**
   * Отправляет команду. Вызовы сериализуются, интервал между
   * последовательными командами — не менее 200 мс (защита от
   * перегрузки командного канала телевизора).
   */
  void sendCommand(Command cmd);

  /** Преобразует цифру 0–9 в соответствующую команду. */
  static Command digit(int d);

signals:
  void pairingKeyResult(bool ok);
  /** Пустая строка — ошибка сопряжения. */
  void pairingConfirmResult(const QString &session);
  void commandResult(bool ok);

private:
  void post(const QString &url, const QByteArray &body,
            std::function<void(int, QByteArray)> cb);
  void tryProcessNext();

  QString m_host;
  QString m_session;
  QNetworkAccessManager *m_nam = nullptr;

  QQueue<Command> m_queue;
  bool m_inFlight = false;
  qint64 m_lastCommandAt = 0;
};

} // namespace lgremote