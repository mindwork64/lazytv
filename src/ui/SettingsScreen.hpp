#pragma once

#include <QWidget>

namespace lgremote {
class AppContainer;
}

class QLabel;
class QRadioButton;

class SettingsScreen : public QWidget {
  Q_OBJECT
public:
  explicit SettingsScreen(lgremote::AppContainer *container,
                          QWidget *parent = nullptr);

  void refresh();

signals:
  void disconnect();
  void back();
  void themeModeChanged(int);

private:
  lgremote::AppContainer *m_container;
  QLabel *m_ipLabel = nullptr;
  QRadioButton *m_sys = nullptr;
  QRadioButton *m_dark = nullptr;
  QRadioButton *m_light = nullptr;
};