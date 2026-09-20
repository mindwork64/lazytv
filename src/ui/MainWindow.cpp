#include "ui/MainWindow.hpp"

#include <QStackedWidget>

#include <lazytv/app_container.hpp>

#include "theme/Theme.hpp"
#include "ui/PairingScreen.hpp"
#include "ui/RemoteScreen.hpp"
#include "ui/SettingsScreen.hpp"

MainWindow::MainWindow(lazytv::AppContainer *container)
    : QMainWindow(nullptr), m_container(container) {

  setWindowTitle("LazyTV");
  setWindowIcon(QIcon(":/ic_launcher.svg"));
  resize(420, 820);
  setMinimumSize(380, 600);

  m_stack = new QStackedWidget(this);
  setCentralWidget(m_stack);

  m_pairing = new PairingScreen(container, this);
  m_remote = new RemoteScreen(container, this);
  m_settings = new SettingsScreen(container, this);

  m_stack->addWidget(m_pairing);
  m_stack->addWidget(m_remote);
  m_stack->addWidget(m_settings);

  connect(m_pairing, &PairingScreen::connected, this,
          [this] { showScreen(Remote); });
  connect(m_remote, &RemoteScreen::openSettings, this,
          [this] { showScreen(Settings); });
  connect(m_remote, &RemoteScreen::disconnected, this,
          [this] { showScreen(Pairing); });
  connect(m_settings, &SettingsScreen::back, this,
          [this] { showScreen(Remote); });
  connect(m_settings, &SettingsScreen::disconnect, this, [this] {
    m_container->clearSession();
    showScreen(Pairing);
  });
  connect(m_settings, &SettingsScreen::themeModeChanged, this, [this](int m) {
    m_container->store().setThemeMode(m);
    ThemeManager::instance().apply(m);
  });

  showScreen(container->store().session() ? Remote : Pairing);
}

void MainWindow::showScreen(Screen s) {
  if (s == Remote)
    m_settings->refresh();
  m_stack->setCurrentIndex(static_cast<int>(s));
}