#include <lazytv/session_store.hpp>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace lazytv {

SessionStore::SessionStore() {
  const QString base =
      QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  QDir().mkpath(base + "/lazytv");
  m_path = base + "/lazytv/config.json";
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
  if (o.contains("pairingKey") && !o["pairingKey"].isNull())
    m_pairingKey = o["pairingKey"].toString();
  m_themeMode = o.value("themeMode").toInt(1);
  m_hotkeysEnabled = o.value("hotkeysEnabled").toBool(true);
}

void SessionStore::save() {
  QJsonObject o;
  o["ip"] = m_ip ? QJsonValue(*m_ip) : QJsonValue::Null;
  o["session"] = m_session ? QJsonValue(*m_session) : QJsonValue::Null;
  o["pairingKey"] = m_pairingKey ? QJsonValue(*m_pairingKey) : QJsonValue::Null;
  o["themeMode"] = m_themeMode;
  o["hotkeysEnabled"] = m_hotkeysEnabled;

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

void SessionStore::setPairingKey(const QString &key) {
  m_pairingKey = key;
  save();
}

void SessionStore::setThemeMode(int m) {
  m_themeMode = m;
  save();
}

void SessionStore::setHotkeysEnabled(bool v) {
  m_hotkeysEnabled = v;
  save();
}

void SessionStore::clearSession() {
  m_session.reset();
  m_pairingKey.reset();
  save();
}

void SessionStore::clearAll() {
  m_ip.reset();
  m_session.reset();
  m_pairingKey.reset();
  m_themeMode = 1;
  m_hotkeysEnabled = true;
  save();
}

} // namespace lazytv