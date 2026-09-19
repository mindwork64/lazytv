#pragma once
#include <QString>
#include <optional>

class SessionStore {
public:
  SessionStore();

  std::optional<QString> ip() const { return m_ip; }
  void setIp(const QString &ip);

  std::optional<QString> session() const { return m_session; }
  void setSession(const QString &s);

  int themeMode() const { return m_themeMode; }
  void setThemeMode(int m);

  void clearSession();
  void clearAll();

private:
  void load();
  void save();

  QString m_path;
  std::optional<QString> m_ip;
  std::optional<QString> m_session;
  int m_themeMode = 1;
};