#pragma once

#include <QString>
#include <memory>

#include "data/SessionStore.hpp"

class LgNetCastClient;

class AppContainer {
public:
  AppContainer();
  ~AppContainer();

  AppContainer(const AppContainer &) = delete;
  AppContainer &operator=(const AppContainer &) = delete;

  SessionStore &store() { return m_store; }

  LgNetCastClient *getClient();
  LgNetCastClient *createClient(const QString &ip);
  void saveSession(const QString &ip, const QString &session);
  void clearSession();

private:
  SessionStore m_store;
  std::unique_ptr<LgNetCastClient> m_client;
};