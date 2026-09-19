#pragma once

#include <QWidget>

namespace lgremote {
class AppContainer;
class Client;
} // namespace lgremote

class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;

class PairingScreen : public QWidget {
  Q_OBJECT
public:
  explicit PairingScreen(lgremote::AppContainer *container,
                         QWidget *parent = nullptr);

signals:
  void connected();

private:
  void requestKey();
  void confirm();
  void setLoading(bool v);
  void setInfo(const QString &);
  void setError(const QString &);

  lgremote::AppContainer *m_container;
  QLineEdit *m_ip;
  QLineEdit *m_key;
  QPushButton *m_reqBtn;
  QPushButton *m_confirmBtn;
  QProgressBar *m_progress;
  QLabel *m_infoLabel;
  QLabel *m_errorLabel;
  lgremote::Client *m_activeClient = nullptr;
};