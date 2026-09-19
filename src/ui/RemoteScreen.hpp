#pragma once

#include <QWidget>

#include "ui/widgets/StatusBar.hpp" // ConnectionStatus

class AppContainer;
class QStackedWidget;
class QLabel;
class QTimer;

class RemoteScreen : public QWidget {
  Q_OBJECT
public:
  explicit RemoteScreen(AppContainer *container, QWidget *parent = nullptr);

signals:
  void openSettings();
  void disconnected();

protected:
  void showEvent(QShowEvent *event) override;

private:
  QWidget *buildMainPage();
  QWidget *buildNumbersPage();

  void sendCommand(int cmd);
  void onCommandResult(bool ok);
  void recomputeStatus();
  void updateErrorBannerStyle();

  AppContainer *m_container = nullptr;
  StatusBar *m_statusBar = nullptr;
  QStackedWidget *m_pages = nullptr;
  QLabel *m_errorBanner = nullptr;
  QTimer *m_statusTimer = nullptr;

  ConnectionStatus m_status = ConnectionStatus::Stale;
  qint64 m_lastSuccessAt = 0;
  bool m_lastAttemptFailed = false;
};