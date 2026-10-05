#include <lazytv/app_container.hpp>
#include <lazytv/client.hpp>

namespace lazytv {

AppContainer::AppContainer() = default;
AppContainer::~AppContainer() = default;

Client *AppContainer::getClient() {
  const auto ip = m_store.ip();
  const auto sess = m_store.session();
  if (!ip || !sess)
    return nullptr;

  if (!m_client || m_client->session() != *sess) {
    m_client = std::make_unique<Client>(*ip);
    m_client->setSession(*sess);
  }
  return m_client.get();
}

Client *AppContainer::createClient(const QString &ip) { return new Client(ip); }

void AppContainer::saveSession(const QString &ip, const QString &session) {
  m_store.setIp(ip);
  m_store.setSession(session);
  m_client = std::make_unique<Client>(ip);
  m_client->setSession(session);
}

bool AppContainer::hasSavedPairing() const {
  const auto key = m_store.pairingKey();
  return m_store.ip().has_value() && key.has_value() && !key->isEmpty();
}

void AppContainer::reauth(std::function<void(bool, bool)> cb) {
  const auto ip  = m_store.ip();
  const auto key = m_store.pairingKey();
  if (!ip || !key || key->isEmpty()) {
    cb(false, false);
    return;
  }

  const QString host = *ip;
  auto *client = new Client(host);
  QObject::connect(client, &Client::pairingConfirmResult,
                   [this, client, host, cb](const Client::PairingResult &r) {
    const bool ok = !r.session.isEmpty();
    const bool rejected =
        !ok && (r.httpStatus == 401 || r.roapError == 401);
    if (ok) {
      m_store.setSession(r.session);
      m_client = std::make_unique<Client>(host);
      m_client->setSession(r.session);
    } else if (rejected) {
      // ТВ больше не помнит это сопряжение — нужен новый код.
      m_store.clearSession();
      m_client.reset();
    }
    // При сетевой ошибке (ТВ выключен/недоступен) ключ сохраняем:
    // следующий запуск повторит попытку.
    client->deleteLater();
    cb(ok, rejected);
  });
  client->confirmPairing(*key);
}

void AppContainer::clearSession() {
  m_store.clearSession();
  m_client.reset();
}

} // namespace lazytv