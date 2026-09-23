#include <QApplication>

#include <cstdio>
#include <cstring>

#include <lazytv/app_container.hpp>

#include "theme/Theme.hpp"
#include "ui/MainWindow.hpp"

#ifndef LAZYTV_VERSION
#define LAZYTV_VERSION "unknown"
#endif

int main(int argc, char** argv) {
    // --version обрабатывается до создания QApplication,
    // чтобы работать без графического окружения (в CI, в headless).
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("lazytv %s\n", LAZYTV_VERSION);
            return 0;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("lazytv");
    app.setApplicationDisplayName("LazyTV");
    app.setApplicationVersion(LAZYTV_VERSION);
    app.setOrganizationName("mindwork64");

    lazytv::AppContainer container;

    ThemeManager::instance().apply(container.store().themeMode());

    MainWindow w(&container);
    w.show();

    return app.exec();
}