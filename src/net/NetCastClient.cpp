#include <lazytv/client.hpp>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QXmlStreamReader>

namespace lazytv {

Client::Client(QString host, QObject* parent)
    : QObject(parent), m_host(std::move(host)),
      m_nam(new QNetworkAccessManager(this)) {}

Client::Command Client::digit(int d) {
    if (d < 0) d = 0;
    if (d > 9) d = 9;
    return static_cast<Command>(static_cast<int>(Command::Number0) + d);
}

void Client::warmUp() {
    if (m_host.isEmpty()) return;

    // GET на корень API. ТВ ответит 404 или что-то ещё — нам важен
    // только сам факт установки TCP-соединения в QNetworkAccessManager.
    const QString url = QStringLiteral("http://%1:%2/").arg(m_host).arg(kPort);

    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(kWarmUpTimeoutMs);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void Client::post(const QString& url, const QByteArray& body,
                  std::function<void(int, QByteArray)> cb) {
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/atom+xml");
    req.setTransferTimeout(kTimeoutMs);

    QNetworkReply* reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this,
            [reply, cb = std::move(cb)]() {
                const int code = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const QByteArray data = reply->readAll();
                cb(code, data);
                reply->deleteLater();
            });
}

void Client::requestPairingKey() {
    const QString url = QStringLiteral("http://%1:%2/roap/api/auth")
                            .arg(m_host).arg(kPort);
    const QByteArray body =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<auth><type>AuthKeyReq</type></auth>";

    post(url, body, [this](int code, QByteArray) {
        emit pairingKeyResult(code == 200);
    });
}

void Client::confirmPairing(const QString& key) {
    const QString url = QStringLiteral("http://%1:%2/roap/api/auth")
                            .arg(m_host).arg(kPort);
    const QByteArray body =
        QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                       "<auth><type>AuthReq</type><value>%1</value></auth>")
            .arg(key).toUtf8();

    post(url, body, [this](int code, QByteArray data) {
        if (code != 200) { emit pairingConfirmResult(QString()); return; }

        QString session;
        QXmlStreamReader r(data);
        while (!r.atEnd()) {
            r.readNext();
            if (r.isStartElement() && r.name() == QStringLiteral("session")) {
                session = r.readElementText();
                break;
            }
        }
        if (!session.isEmpty()) m_session = session;
        emit pairingConfirmResult(session);
    });
}

void Client::sendCommand(Command cmd) {
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
    const bool isFirstCommand = !m_firstCommandSent;
    m_firstCommandSent = true;
    m_inFlight = true;

    const QString url = QStringLiteral("http://%1:%2/roap/api/command")
                            .arg(m_host).arg(kPort);
    const QByteArray body =
        QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                       "<command><session>%1</session>"
                       "<name>HandleKeyInput</name>"
                       "<value>%2</value></command>")
            .arg(m_session).arg(static_cast<int>(cmd)).toUtf8();

    post(url, body, [this, cmd, isFirstCommand](int code, QByteArray) {
        m_lastCommandAt = QDateTime::currentMSecsSinceEpoch();
        m_inFlight = false;

        const bool ok = (code == 200);

        // Retry ровно один раз, только для самой первой команды после
        // создания клиента. Первая попытка могла уйти по «холодному»
        // соединению и потеряться; вторая пойдёт по уже установленному TCP.
        if (!ok && isFirstCommand) {
            m_queue.prepend(cmd);
            QTimer::singleShot(kFirstCommandRetryDelayMs, this,
                               &Client::tryProcessNext);
            return;
        }

        emit commandResult(ok);
        tryProcessNext();
    });
}

} // namespace lazytv