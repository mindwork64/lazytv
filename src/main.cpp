#include <QApplication>

#include <lazytv/app_container.hpp>

#include "theme/Theme.hpp"
#include "ui/MainWindow.hpp"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("lazytv");
  app.setApplicationDisplayName("LazyTV");
  app.setOrganizationName("mindwork64");

  lazytv::AppContainer container;

  ThemeManager::instance().apply(container.store().themeMode());

  MainWindow w(&container);
  w.show();

  return app.exec();
}