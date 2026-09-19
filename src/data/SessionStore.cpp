#include <lgremote/session_store.hpp>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace lgremote {

SessionStore::SessionStore() {
  const QString base =
      QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  QDir().mkpath(base + "/lgremote");
  m_path = base + "/lgremote/config.json";
  load();
}

void SessionStore::load() {
  QFile f(m_path);
  if (!f.open(QIODevice::ReadOnly))
    return;
  const auto doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject())
    return;
  const auto o = doc.object();

  if (o.contains("ip") && !o["ip"].isNull())
    m_ip = o["ip"].toString();
  if (o.contains("session") && !o["session"].isNull())
    m_session = o["session"].toString();
  m_themeMode = o.value("themeMode").toInt(1);
}

void SessionStore::save() {
  QJsonObject o;
  o["ip"] = m_ip ? QJsonValue(*m_ip) : QJsonValue::Null;
  o["session"] = m_session ? QJsonValue(*m_session) : QJsonValue::Null;
  o["themeMode"] = m_themeMode;

  QFile f(m_path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return;
  f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

void SessionStore::setIp(const QString &ip) {
  m_ip = ip;
  save();
}

void SessionStore::setSession(const QString &s) {
  m_session = s;
  save();
}

void SessionStore::setThemeMode(int m) {
  m_themeMode = m;
  save();
}

void SessionStore::clearSession() {
  m_session.reset();
  save();
}

void SessionStore::clearAll() {
  m_ip.reset();
  m_session.reset();
  m_themeMode = 1;
  save();
}

} // namespace lgremote