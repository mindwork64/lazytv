#pragma once

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QVector>

#include <functional>

#include <lazytv/lazytv_export.h>

namespace lazytv {

/** Канал и текущая программа (cur_channel / channel_list). */
struct LAZYTV_EXPORT ChannelInfo {
    QString chtype;
    QString major;
    QString displayMajor;
    QString minor;
    QString displayMinor;
    QString sourceIndex;
    QString physicalNum;
    QString chname;
    QString progName;
    QString audioCh;
    QString inputSourceName;
    QString inputSourceType;
    QString labelName;
    QString inputSourceIdx;

    bool isValid() const {
        return !major.isEmpty() || !chname.isEmpty()
               || !inputSourceName.isEmpty() || !labelName.isEmpty();
    }

    /** Номер канала для UI: displayMajor (+ displayMinor), иначе major. */
    QString number() const;

    /** Название: chname, иначе имя входа. */
    QString name() const;
};

/** Громкость из volume_info. */
struct LAZYTV_EXPORT VolumeInfo {
    int  level    = -1;
    int  minLevel = 0;
    int  maxLevel = 100;
    bool muted    = false;

    bool isValid() const { return level >= 0; }
};

/**
 * Клиент LG NetCast.
 *
 * Реализует протокол удалённого управления телевизорами LG (2010–2014,
 * серии LB/LA/LN/LV/LS/LE/UB/UC/UF). Работает через QTcpSocket напрямую:
 * HTTP/1.1 на IP-адрес, без прокси, без IPv6, без HTTP/2 negotiation.
 *
 * Обрабатывает истёкшую сессию: при HTTP 401 клиент эмитит сигнал
 * sessionExpired(), после чего все sendCommand игнорируются, пока
 * не будет установлена новая сессия через confirmPairing().
 */
class LAZYTV_EXPORT Client : public QObject {
    Q_OBJECT
public:
    static constexpr int    kPort = 8080;
    static constexpr qint64 kMinCommandIntervalMs = 200;
    static constexpr int    kRequestTimeoutMs = 8000;

    enum class Command : int {
        Power = 1,
        Number0 = 2, Number1 = 3, Number2 = 4, Number3 = 5,
        Number4 = 6, Number5 = 7, Number6 = 8, Number7 = 9,
        Number8 = 10, Number9 = 11,
        Up = 12, Down = 13, Left = 14, Right = 15,
        Ok = 20, HomeMenu = 21, Back = 23,
        VolumeUp = 24, VolumeDown = 25, MuteToggle = 26,
        ChannelUp = 27, ChannelDown = 28,
        Blue = 29, Green = 30, Red = 31, Yellow = 32,
        Play = 33, Pause = 34, Stop = 35,
        FastForward = 36, Rewind = 37,
        SkipForward = 38, SkipBackward = 39,
        Record = 40, Repeat = 42, LiveTv = 43, Epg = 44,
        Info = 45, AspectRatio = 46, ExternalInput = 47,
        ShowSubtitle = 49, TeleText = 51,
        PreviousChannel = 403, FavoriteChannel = 404,
        QuickMenu = 405, AvMode = 410, SimpLink = 411,
        Exit = 412, Apps = 417,
    };
    Q_ENUM(Command)

    /** Запросы данных через /roap/api/data (или /udap/api/data). */
    enum class Query : int {
        CurrentChannel = 1,
        ChannelList,
        VolumeInfo,
        ScreenImage,
    };
    Q_ENUM(Query)

    /** Результат сопряжения. */
    struct PairingResult {
        QString session;
        int     httpStatus = 0;
        int     roapError  = 0;
        QString detail;
    };

    explicit Client(QString host, QObject* parent = nullptr);

    void setSession(const QString& sid) { m_session = sid; }
    QString session() const { return m_session; }

    void requestPairingKey();
    void confirmPairing(const QString& key);
    void sendCommand(Command cmd);

    /** Переключить канал по записи из channel_list. */
    void changeChannel(const ChannelInfo& channel);

    /** Запросы состояния ТВ (без сессии). */
    void requestCurrentChannel();
    void requestChannelList();
    void requestVolumeInfo();
    void requestScreenImage();

    static Command digit(int d);

signals:
    void pairingKeyResult(bool ok);
    void pairingConfirmResult(const lazytv::Client::PairingResult& result);
    void commandResult(bool ok);
    void sessionExpired();

    void currentChannelResult(const lazytv::ChannelInfo& channel);
    void channelListResult(const QVector<lazytv::ChannelInfo>& channels);
    void volumeInfoResult(const lazytv::VolumeInfo& volume);
    void screenImageResult(const QByteArray& jpeg);
    void queryFailed(lazytv::Client::Query query, int httpStatus);

private:
    struct HttpResponse {
        int statusCode = 0;
        QByteArray body;
    };

    using HttpCallback = std::function<void(const HttpResponse&)>;

    struct CommandRequest {
        Command     cmd = Command::Power;
        bool        channelChange = false;
        ChannelInfo channel;
    };

    void sendHttp(const QByteArray& method,
                  const QString& path,
                  const QByteArray& body,
                  HttpCallback cb);

    void tryProcessNext();

    void requestData(Query query, const QString& target);
    void fetchData(Query query, const QString& target,
                   const QString& base, bool allowFallback);
    void handleData(Query query, const QByteArray& body);

    QString m_host;
    QString m_session;
    QString m_dataBase;

    QQueue<CommandRequest> m_queue;
    QSet<int> m_pendingQueries;
    bool   m_inFlight = false;
    qint64 m_lastCommandAt = 0;
    bool   m_sessionExpired = false;
};

} // namespace lazytv