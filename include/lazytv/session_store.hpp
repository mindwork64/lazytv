#pragma once

#include <QString>
#include <optional>

#include <lazytv/lazytv_export.h>

namespace lazytv {

/**
 * Постоянное хранилище сессии и настроек.
 *
 * Файл: $XDG_CONFIG_HOME/lazytv/config.json
 * По умолчанию ~/.config/lazytv/config.json
 *
 * Не потокобезопасен. Рассчитан на использование из GUI-потока.
 */
class LAZYTV_EXPORT SessionStore {
public:
  SessionStore();

  std::optional<QString> ip() const { return m_ip; }
  void setIp(const QString &ip);

  std::optional<QString> session() const { return m_session; }
  void setSession(const QString &s);

  /**
   * Ключ сопряжения (6-значный код с экрана ТВ).
   *
   * Долговременный учётный ключ: по нему приложение при каждом запуске
   * заново получает свежую сессию (AuthReq), поэтому его нужно хранить
   * вместе с IP.
   */
  std::optional<QString> pairingKey() const { return m_pairingKey; }
  void setPairingKey(const QString &key);

  /** 0 = системная, 1 = тёмная, 2 = светлая. */
  int themeMode() const { return m_themeMode; }
  void setThemeMode(int m);

  /** Горячие клавиши на экране пульта (по умолчанию включены). */
  bool hotkeysEnabled() const { return m_hotkeysEnabled; }
  void setHotkeysEnabled(bool v);

  void clearSession();
  void clearAll();

  /** Путь к файлу конфигурации (для отладки / миграции). */
  QString configPath() const { return m_path; }

private:
  void load();
  void save();

  QString m_path;
  std::optional<QString> m_ip;
  std::optional<QString> m_session;
  std::optional<QString> m_pairingKey;
  int m_themeMode = 1;
  bool m_hotkeysEnabled = true;
};

} // namespace lazytv