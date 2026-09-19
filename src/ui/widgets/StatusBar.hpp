#pragma once
#include <QWidget>

enum class ConnectionStatus { Online, Stale, Offline };

class StatusBar : public QWidget {
  Q_OBJECT
public:
  explicit StatusBar(QWidget *parent = nullptr);

  void setIp(const QString &ip);
  void setStatus(ConnectionStatus s);

signals:
  void settingsClicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *) override;

private:
  QString m_ip;
  ConnectionStatus m_status = ConnectionStatus::Stale;
  QRect m_settingsRect;
};