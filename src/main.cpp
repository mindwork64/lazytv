#include <QApplication>
#include <QDir>
#include <QIcon>

#include "data/AppContainer.hpp"
#include "theme/Theme.hpp"
#include "ui/MainWindow.hpp"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("lgremote");
  app.setApplicationDisplayName("LG Лентяйка");
  app.setOrganizationName("mindwork64");

  AppContainer container;

  ThemeManager::instance().apply(container.store().themeMode());

  MainWindow w(&container);
  w.show();

  return app.exec();
}