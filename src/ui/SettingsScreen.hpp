#pragma once

#include <QWidget>

namespace lazytv {
class AppContainer;
}

class QLabel;
class QRadioButton;

class SettingsScreen : public QWidget {
  Q_OBJECT
public:
  explicit SettingsScreen(lazytv::AppContainer *container,
                          QWidget *parent = nullptr);

  void refresh();

signals:
  void disconnect();
  void back();
  void themeModeChanged(int);

private:
  lazytv::AppContainer *m_container;
  QLabel *m_ipLabel = nullptr;
  QRadioButton *m_sys = nullptr;
  QRadioButton *m_dark = nullptr;
  QRadioButton *m_light = nullptr;
};