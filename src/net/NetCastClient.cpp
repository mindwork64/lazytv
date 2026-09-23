#include <lazytv/client.hpp>

#include <QDateTime>
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

} // namespace

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
    m_queue.enqueue(cmd);
    tryProcessNext();
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

    const Command cmd = m_queue.dequeue();
    m_inFlight = true;

    const QByteArray body =
        QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                       "<command><session>%1</session>"
                       "<name>HandleKeyInput</name>"
                       "<value>%2</value></command>")
            .arg(m_session).arg(static_cast<int>(cmd)).toUtf8();

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