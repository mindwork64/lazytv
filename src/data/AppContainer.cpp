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

void AppContainer::clearSession() {
  m_store.clearSession();
  m_client.reset();
}

} // namespace lazytv