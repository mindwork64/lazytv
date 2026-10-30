#include <lazytv/client.hpp>

#include <QDateTime>
#include <QHash>
#include <QTcpSocket>
#include <QTimer>
#include <QXmlStreamReader>

#include <memory>

namespace lazytv {

namespace {

/** Разбирает HTTP-ответ, возвращает (status, body). */
std::pair<int, QByteArray> parseHttp(const QByteArray& raw) {
    const int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0) return {0, {}};

    const int firstLineEnd = raw.indexOf("\r\n");
    if (firstLineEnd < 0) return {0, {}};

    const QByteArray firstLine = raw.left(firstLineEnd);
    const QList<QByteArray> parts = firstLine.split(' ');
    if (parts.size() < 2) return {0, {}};

    bool ok = false;
    const int status = parts.at(1).toInt(&ok);
    if (!ok) return {0, {}};

    return {status, raw.mid(headerEnd + 4)};
}

/** Разбирает <envelope> из ответа ROAP. */
struct RoapEnvelope {
    int     roapError = 0;
    QString detail;
    QString session;
};

RoapEnvelope parseRoap(const QByteArray& body) {
    RoapEnvelope env;
    QXmlStreamReader r(body);
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        if (r.name() == QStringLiteral("ROAPError")) {
            env.roapError = r.readElementText().toInt();
        } else if (r.name() == QStringLiteral("ROAPErrorDetail")) {
            env.detail = r.readElementText();
        } else if (r.name() == QStringLiteral("session")) {
            env.session = r.readElementText();
        }
    }
    return env;
}

/** Поля первого <data> из ответа /api/data (cur_channel, volume_info). */
QHash<QString, QString> parseDataFields(const QByteArray& body) {
    QHash<QString, QString> fields;
    QXmlStreamReader r(body);
    bool inData = false;

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            if (r.name() == QStringLiteral("data")) {
                inData = true;
                continue;
            }
            if (inData)
                fields.insert(r.name().toString(), r.readElementText());
        } else if (r.isEndElement() && r.name() == QStringLiteral("data")
                   && inData) {
            break;
        }
    }
    return fields;
}

/** Все <data> из channel_list. */
QVector<ChannelInfo> parseChannelList(const QByteArray& body) {
    QVector<ChannelInfo> channels;
    QXmlStreamReader r(body);
    ChannelInfo current;
    bool inData = false;

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            if (r.name() == QStringLiteral("data")) {
                current = ChannelInfo{};
                inData = true;
                continue;
            }
            if (!inData) continue;

            const QString name = r.name().toString();
            const QString value = r.readElementText();
            if (name == QStringLiteral("chtype"))             current.chtype = value;
            else if (name == QStringLiteral("major"))         current.major = value;
            else if (name == QStringLiteral("displayMajor"))  current.displayMajor = value;
            else if (name == QStringLiteral("minor"))         current.minor = value;
            else if (name == QStringLiteral("displayMinor"))  current.displayMinor = value;
            else if (name == QStringLiteral("sourceIndex"))   current.sourceIndex = value;
            else if (name == QStringLiteral("physicalNum"))   current.physicalNum = value;
            else if (name == QStringLiteral("chname"))        current.chname = value;
            else if (name == QStringLiteral("progName"))      current.progName = value;
            else if (name == QStringLiteral("audioCh"))       current.audioCh = value;
            else if (name == QStringLiteral("inputSourceName")) current.inputSourceName = value;
            else if (name == QStringLiteral("inputSourceType")) current.inputSourceType = value;
            else if (name == QStringLiteral("labelName"))     current.labelName = value;
            else if (name == QStringLiteral("inputSourceIdx")) current.inputSourceIdx = value;
        } else if (r.isEndElement() && r.name() == QStringLiteral("data")
                   && inData) {
            inData = false;
            if (current.isValid())
                channels.append(current);
            current = ChannelInfo{};
        }
    }
    return channels;
}

ChannelInfo channelFromFields(const QHash<QString, QString>& f) {
    ChannelInfo c;
    c.chtype          = f.value(QStringLiteral("chtype"));
    c.major           = f.value(QStringLiteral("major"));
    c.displayMajor    = f.value(QStringLiteral("displayMajor"));
    c.minor           = f.value(QStringLiteral("minor"));
    c.displayMinor    = f.value(QStringLiteral("displayMinor"));
    c.sourceIndex     = f.value(QStringLiteral("sourceIndex"));
    c.physicalNum     = f.value(QStringLiteral("physicalNum"));
    c.chname          = f.value(QStringLiteral("chname"));
    c.progName        = f.value(QStringLiteral("progName"));
    c.audioCh         = f.value(QStringLiteral("audioCh"));
    c.inputSourceName = f.value(QStringLiteral("inputSourceName"));
    c.inputSourceType = f.value(QStringLiteral("inputSourceType"));
    c.labelName       = f.value(QStringLiteral("labelName"));
    c.inputSourceIdx  = f.value(QStringLiteral("inputSourceIdx"));
    return c;
}

VolumeInfo volumeFromFields(const QHash<QString, QString>& f) {
    VolumeInfo v;
    bool ok = false;
    v.level = f.value(QStringLiteral("level")).toInt(&ok);
    if (!ok) v.level = -1;

    v.minLevel = f.value(QStringLiteral("minLevel")).toInt();
    v.maxLevel = f.value(QStringLiteral("maxLevel")).toInt();
    if (v.maxLevel <= 0) v.maxLevel = 100;
    v.muted = f.value(QStringLiteral("mute")) == QStringLiteral("true");
    return v;
}

} // namespace

QString ChannelInfo::number() const {
    QString n = displayMajor.isEmpty() ? major : displayMajor;
    if (n.isEmpty()) return n;

    const QString m = displayMinor.isEmpty() ? minor : displayMinor;
    if (!m.isEmpty() && m != QStringLiteral("-1")
        && m != QStringLiteral("65535")) {
        n += QLatin1Char('.');
        n += m;
    }
    return n;
}

QString ChannelInfo::name() const {
    if (!chname.isEmpty()) return chname;
    if (!inputSourceName.isEmpty()) return inputSourceName;
    return labelName;
}

Client::Client(QString host, QObject* parent)
    : QObject(parent), m_host(std::move(host)) {}

Client::Command Client::digit(int d) {
    if (d < 0) d = 0;
    if (d > 9) d = 9;
    return static_cast<Command>(static_cast<int>(Command::Number0) + d);
}

void Client::sendHttp(const QByteArray& method,
                      const QString& path,
                      const QByteArray& body,
                      HttpCallback cb) {
    auto* socket = new QTcpSocket(this);
    auto buffer       = std::make_shared<QByteArray>();
    auto finished     = std::make_shared<bool>(false);
    auto callback     = std::make_shared<HttpCallback>(std::move(cb));
    auto expectedBody = std::make_shared<int>(-1);

    auto finish = [socket, buffer, finished, callback](int status) {
        if (*finished) return;
        *finished = true;

        HttpResponse r;
        r.statusCode = status;
        if (status > 0) {
            r.body = parseHttp(*buffer).second;
        }
        if (*callback) (*callback)(r);

        socket->abort();
        socket->deleteLater();
    };

    auto tryComplete = [buffer, finished, expectedBody, finish]() {
        if (*finished) return;

        const int headerEnd = buffer->indexOf("\r\n\r\n");
        if (headerEnd < 0) return;

        const int firstLineEnd = buffer->indexOf("\r\n");
        if (firstLineEnd < 0) return;

        const QByteArray firstLine = buffer->left(firstLineEnd);
        const QList<QByteArray> parts = firstLine.split(' ');
        if (parts.size() < 2) return;
        bool ok = false;
        const int status = parts.at(1).toInt(&ok);
        if (!ok) return;

        if (*expectedBody < 0) {
            const QByteArray headers = buffer->left(headerEnd).toLower();
            const int clPos = headers.indexOf("content-length:");
            if (clPos >= 0) {
                const int lineEnd = headers.indexOf("\r\n", clPos);
                const QByteArray clLine = headers.mid(clPos, lineEnd - clPos);
                const QByteArray clValue =
                    clLine.mid(clLine.indexOf(':') + 1).trimmed();
                *expectedBody = clValue.toInt();
            } else {
                *expectedBody = -2;
            }
        }

        if (*expectedBody >= 0) {
            const int bodyReceived = buffer->size() - headerEnd - 4;
            if (bodyReceived >= *expectedBody) {
                finish(status);
            }
        }
    };

    QTimer::singleShot(kRequestTimeoutMs, socket,
                       [finish]() { finish(0); });

    connect(socket, &QTcpSocket::connected, this,
            [socket, method, path, body, this]() {
        QByteArray req;
        req.append(method).append(' ').append(path.toUtf8())
           .append(" HTTP/1.1\r\n");
        req.append("Host: ").append(m_host.toUtf8())
           .append(':').append(QByteArray::number(kPort)).append("\r\n");
        req.append("User-Agent: LazyTV/1.0\r\n");
        req.append("Connection: close\r\n");
        if (!body.isEmpty()) {
            req.append("Content-Type: application/atom+xml\r\n");
            req.append("Content-Length: ")
               .append(QByteArray::number(body.size())).append("\r\n");
        }
        req.append("\r\n");
        req.append(body);

        socket->write(req);
        socket->flush();
    });

    connect(socket, &QTcpSocket::readyRead, this,
            [socket, buffer, tryComplete]() {
        *buffer += socket->readAll();
        tryComplete();
    });

    connect(socket, &QTcpSocket::disconnected, this,
            [buffer, finish]() {
        const int status = parseHttp(*buffer).first;
        finish(status);
    });

    connect(socket, &QTcpSocket::errorOccurred, this,
            [finish](QAbstractSocket::SocketError) { finish(0); });

    socket->connectToHost(m_host, kPort);
}

void Client::requestPairingKey() {
    const QByteArray body =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<auth><type>AuthKeyReq</type></auth>";

    sendHttp("POST", QStringLiteral("/roap/api/auth"), body,
             [this](const HttpResponse& r) {
        emit pairingKeyResult(r.statusCode == 200);
    });
}

void Client::confirmPairing(const QString& key) {
    const QByteArray body =
        QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                       "<auth><type>AuthReq</type>"
                       "<value>%1</value></auth>")
            .arg(key).toUtf8();

    sendHttp("POST", QStringLiteral("/roap/api/auth"), body,
             [this](const HttpResponse& r) {
        PairingResult result;
        result.httpStatus = r.statusCode;

        if (r.statusCode == 200) {
            const auto env = parseRoap(r.body);
            result.roapError = env.roapError;
            result.detail    = env.detail;
            result.session   = env.session;

            if (!result.session.isEmpty()) {
                m_session = result.session;
                m_sessionExpired = false;
            }
        }

        emit pairingConfirmResult(result);
    });
}

void Client::sendCommand(Command cmd) {
    if (m_sessionExpired) {
        return;
    }
    CommandRequest request;
    request.cmd = cmd;
    m_queue.enqueue(request);
    tryProcessNext();
}

void Client::changeChannel(const ChannelInfo& channel) {
    if (m_sessionExpired || !channel.isValid()) {
        return;
    }
    CommandRequest request;
    request.channelChange = true;
    request.channel = channel;
    m_queue.enqueue(request);
    tryProcessNext();
}

void Client::requestCurrentChannel() {
    requestData(Query::CurrentChannel, QStringLiteral("cur_channel"));
}

void Client::requestChannelList() {
    requestData(Query::ChannelList, QStringLiteral("channel_list"));
}

void Client::requestVolumeInfo() {
    requestData(Query::VolumeInfo, QStringLiteral("volume_info"));
}

void Client::requestScreenImage() {
    requestData(Query::ScreenImage, QStringLiteral("screen_image"));
}

void Client::requestData(Query query, const QString& target) {
    const int id = static_cast<int>(query);
    if (m_pendingQueries.contains(id)) return;

    m_pendingQueries.insert(id);
    const QString base = m_dataBase.isEmpty() ? QStringLiteral("roap")
                                              : m_dataBase;
    fetchData(query, target, base, true);
}

void Client::fetchData(Query query, const QString& target,
                       const QString& base, bool allowFallback) {
    const QString path =
        QStringLiteral("/%1/api/data?target=%2&_=%3")
            .arg(base, target)
            .arg(QDateTime::currentMSecsSinceEpoch());

    sendHttp("GET", path, {},
             [this, query, target, base, allowFallback](
                 const HttpResponse& r) {
        if (r.statusCode == 200) {
            // Некоторые ТВ отвечают 200 и кладут ошибку в ROAPError.
            if (allowFallback && base == QStringLiteral("roap")
                && query != Query::ScreenImage) {
                const RoapEnvelope env = parseRoap(r.body);
                if (env.roapError != 0 && env.roapError != 200) {
                    fetchData(query, target, QStringLiteral("udap"), false);
                    return;
                }
            }

            m_dataBase = base;
            m_pendingQueries.remove(static_cast<int>(query));
            handleData(query, r.body);
            return;
        }

        // ROAP-ТВ отвечают на /roap/api/data, UDAP-ТВ — на /udap/api/data.
        if (allowFallback && base == QStringLiteral("roap")
            && r.statusCode == 404) {
            fetchData(query, target, QStringLiteral("udap"), false);
            return;
        }

        m_pendingQueries.remove(static_cast<int>(query));
        emit queryFailed(query, r.statusCode);
    });
}

void Client::handleData(Query query, const QByteArray& body) {
    switch (query) {
        case Query::CurrentChannel: {
            const ChannelInfo channel =
                channelFromFields(parseDataFields(body));
            if (!channel.isValid()) {
                emit queryFailed(query, 200);
                return;
            }
            emit currentChannelResult(channel);
            break;
        }
        case Query::ChannelList:
            emit channelListResult(parseChannelList(body));
            break;
        case Query::VolumeInfo: {
            const VolumeInfo volume = volumeFromFields(parseDataFields(body));
            if (!volume.isValid()) {
                emit queryFailed(query, 200);
                return;
            }
            emit volumeInfoResult(volume);
            break;
        }
        case Query::ScreenImage: {
            const bool jpeg =
                body.size() > 2
                && static_cast<unsigned char>(body.at(0)) == 0xFF
                && static_cast<unsigned char>(body.at(1)) == 0xD8;
            if (!jpeg) {
                emit queryFailed(query, 200);
                return;
            }
            emit screenImageResult(body);
            break;
        }
    }
}

void Client::tryProcessNext() {
    if (m_inFlight || m_queue.isEmpty()) return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = now - m_lastCommandAt;
    if (m_lastCommandAt > 0 && elapsed < kMinCommandIntervalMs) {
        QTimer::singleShot(static_cast<int>(kMinCommandIntervalMs - elapsed),
                           this, &Client::tryProcessNext);
        return;
    }

    if (m_session.isEmpty()) {
        m_queue.clear();
        emit commandResult(false);
        return;
    }

    const CommandRequest request = m_queue.dequeue();
    m_inFlight = true;

    QByteArray body;
    if (request.channelChange) {
        const ChannelInfo& c = request.channel;
        body = QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                   "<command><session>%1</session>"
                   "<name>HandleChannelChange</name>"
                   "<major>%2</major><minor>%3</minor>"
                   "<sourceIndex>%4</sourceIndex>"
                   "<physicalNum>%5</physicalNum></command>")
                   .arg(m_session, c.major, c.minor,
                        c.sourceIndex, c.physicalNum)
                   .toUtf8();
    } else {
        body = QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                   "<command><session>%1</session>"
                   "<name>HandleKeyInput</name>"
                   "<value>%2</value></command>")
                   .arg(m_session)
                   .arg(static_cast<int>(request.cmd))
                   .toUtf8();
    }

    sendHttp("POST", QStringLiteral("/roap/api/command"), body,
             [this](const HttpResponse& r) {
        m_lastCommandAt = QDateTime::currentMSecsSinceEpoch();
        m_inFlight = false;

        if (r.statusCode == 401) {
            m_sessionExpired = true;
            m_queue.clear();
            m_session.clear();
            emit sessionExpired();
            return;
        }

        const bool ok = (r.statusCode == 200);
        emit commandResult(ok);
        tryProcessNext();
    });
}

} // namespace lazytv