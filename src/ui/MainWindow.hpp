#pragma once

#include <QMainWindow>

namespace lazytv {
class AppContainer;
}

class QStackedWidget;
class PairingScreen;
class RemoteScreen;
class SettingsScreen;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(lazytv::AppContainer *container);

private:
  enum Screen { Pairing = 0, Remote, Settings };

  void showScreen(Screen s);

  lazytv::AppContainer *m_container;
  QStackedWidget *m_stack;
  PairingScreen *m_pairing;
  RemoteScreen *m_remote;
  SettingsScreen *m_settings;
};