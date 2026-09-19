#include "data/AppContainer.hpp"

#include "net/LgNetCastClient.hpp"

AppContainer::AppContainer() = default;

AppContainer::~AppContainer() = default;

LgNetCastClient *AppContainer::getClient() {
  const auto ip = m_store.ip();
  const auto sess = m_store.session();
  if (!ip || !sess)
    return nullptr;

  if (!m_client || m_client->session() != *sess) {
    m_client = std::make_unique<LgNetCastClient>(*ip);
    m_client->setSession(*sess);
  }
  return m_client.get();
}

LgNetCastClient *AppContainer::createClient(const QString &ip) {
  // Владение передаётся вызывающему через deleteLater()
  return new LgNetCastClient(ip);
}

void AppContainer::saveSession(const QString &ip, const QString &session) {
  m_store.setIp(ip);
  m_store.setSession(session);
  m_client = std::make_unique<LgNetCastClient>(ip);
  m_client->setSession(session);
}

void AppContainer::clearSession() {
  m_store.clearSession();
  m_client.reset();
}