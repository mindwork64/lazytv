#pragma once

#include <QString>
#include <memory>

#include <lazytv/lazytv_export.h>
#include <lazytv/session_store.hpp>

namespace lazytv {

class Client;

/**
 * Владелец клиента и хранилища сессии.
 *
 * Управляет жизненным циклом активного Client, восстанавливает сессию
 * из SessionStore. Не потокобезопасен.
 */
class LAZYTV_EXPORT AppContainer {
public:
  AppContainer();
  ~AppContainer();

  AppContainer(const AppContainer &) = delete;
  AppContainer &operator=(const AppContainer &) = delete;

  SessionStore &store() { return m_store; }

  /**
   * Возвращает клиент с восстановленной сессией
   * или nullptr, если сессии нет.
   */
  Client *getClient();

  /**
   * Новый клиент для процесса сопряжения.
   * Владение передаётся вызывающему — удалить через deleteLater().
   */
  Client *createClient(const QString &ip);

  /** Сохраняет сессию и делает клиент активным. */
  void saveSession(const QString &ip, const QString &session);

  void clearSession();

private:
  SessionStore m_store;
  std::unique_ptr<Client> m_client;
};

} // namespace lazytv