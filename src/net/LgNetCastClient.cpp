#include "net/LgNetCastClient.hpp"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QXmlStreamReader>

LgNetCastClient::LgNetCastClient(QString host, QObject *parent)
    : QObject(parent), m_host(std::move(host)),
      m_nam(new QNetworkAccessManager(this)) {}

void LgNetCastClient::post(const QString &url, const QByteArray &body,
                           std::function<void(int, QByteArray)> cb) {
  QNetworkRequest req{QUrl(url)};
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/atom+xml");
  req.setTransferTimeout(3000);

  QNetworkReply *reply = m_nam->post(req, body);
  connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
    const int code =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    cb(code, data);
    reply->deleteLater();
  });
}

void LgNetCastClient::requestPairingKey() {
  const QString url =
      QStringLiteral("http://%1:%2/roap/api/auth").arg(m_host).arg(kPort);
  const QByteArray body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                          "<auth><type>AuthKeyReq</type></auth>";

  post(url, body,
       [this](int code, QByteArray) { emit pairingKeyResult(code == 200); });
}

void LgNetCastClient::confirmPairing(const QString &key) {
  const QString url =
      QStringLiteral("http://%1:%2/roap/api/auth").arg(m_host).arg(kPort);
  const QByteArray body =
      QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                     "<auth><type>AuthReq</type><value>%1</value></auth>")
          .arg(key)
          .toUtf8();

  post(url, body, [this](int code, QByteArray data) {
    if (code != 200) {
      emit pairingConfirmResult(QString());
      return;
    }

    QString session;
    QXmlStreamReader r(data);
    while (!r.atEnd()) {
      r.readNext();
      if (r.isStartElement() && r.name() == QStringLiteral("session")) {
        session = r.readElementText();
        break;
      }
    }
    if (!session.isEmpty())
      m_session = session;
    emit pairingConfirmResult(session);
  });
}

void LgNetCastClient::sendCommand(int cmd) {
  m_queue.enqueue(cmd);
  tryProcessNext();
}

void LgNetCastClient::tryProcessNext() {
  if (m_inFlight || m_queue.isEmpty())
    return;

  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  const qint64 elapsed = now - m_lastCommandAt;
  if (m_lastCommandAt > 0 && elapsed < kMinCommandIntervalMs) {
    QTimer::singleShot(static_cast<int>(kMinCommandIntervalMs - elapsed), this,
                       &LgNetCastClient::tryProcessNext);
    return;
  }

  if (m_session.isEmpty()) {
    m_queue.clear();
    emit commandResult(false);
    return;
  }

  const int cmd = m_queue.dequeue();
  m_inFlight = true;

  const QString url =
      QStringLiteral("http://%1:%2/roap/api/command").arg(m_host).arg(kPort);
  const QByteArray body =
      QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                     "<command><session>%1</session>"
                     "<name>HandleKeyInput</name>"
                     "<value>%2</value></command>")
          .arg(m_session)
          .arg(cmd)
          .toUtf8();

  post(url, body, [this](int code, QByteArray) {
    m_lastCommandAt = QDateTime::currentMSecsSinceEpoch();
    m_inFlight = false;
    emit commandResult(code == 200);
    tryProcessNext();
  });
}