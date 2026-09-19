#include "ui/PairingScreen.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include <lgremote/app_container.hpp>
#include <lgremote/client.hpp>

#include "theme/Theme.hpp"

PairingScreen::PairingScreen(lgremote::AppContainer *container, QWidget *parent)
    : QWidget(parent), m_container(container) {

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(24, 24, 24, 24);
  root->setSpacing(16);

  auto *title = new QLabel("Подключение к телевизору", this);
  QFont f = title->font();
  f.setPointSize(16);
  title->setFont(f);
  root->addWidget(title);

  m_ip = new QLineEdit(this);
  m_ip->setPlaceholderText("IP телевизора");
  m_ip->setText(m_container->store().ip().value_or(QString()));
  root->addWidget(m_ip);

  m_reqBtn = new QPushButton("Запросить код на ТВ", this);
  connect(m_reqBtn, &QPushButton::clicked, this, &PairingScreen::requestKey);
  root->addWidget(m_reqBtn);

  m_key = new QLineEdit(this);
  m_key->setPlaceholderText("Код сопряжения (6 цифр)");
  m_key->setMaxLength(6);
  m_key->setEchoMode(QLineEdit::Password);
  connect(m_key, &QLineEdit::textChanged, this, [this](const QString &t) {
    QString digits = t;
    digits.remove(QRegularExpression("[^0-9]"));
    if (digits != m_key->text())
      m_key->setText(digits);
  });
  root->addWidget(m_key);

  m_confirmBtn = new QPushButton("Подключиться", this);
  connect(m_confirmBtn, &QPushButton::clicked, this, &PairingScreen::confirm);
  root->addWidget(m_confirmBtn);

  m_progress = new QProgressBar(this);
  m_progress->setRange(0, 0);
  m_progress->setVisible(false);
  root->addWidget(m_progress);

  m_infoLabel = new QLabel(this);
  m_infoLabel->setWordWrap(true);
  m_errorLabel = new QLabel(this);
  m_errorLabel->setWordWrap(true);
  root->addWidget(m_infoLabel);
  root->addWidget(m_errorLabel);

  root->addStretch();

  connect(&ThemeManager::instance(), &ThemeManager::changed, this, [this]() {
    const auto &p = ThemeManager::instance().palette();
    m_infoLabel->setStyleSheet(QString("color: %1;").arg(p.primary.name()));
    m_errorLabel->setStyleSheet(QString("color: %1;").arg(p.error.name()));
  });
}

void PairingScreen::setLoading(bool v) {
  m_reqBtn->setEnabled(!v);
  m_confirmBtn->setEnabled(!v);
  m_ip->setEnabled(!v);
  m_key->setEnabled(!v);
  m_progress->setVisible(v);
}

void PairingScreen::setInfo(const QString &s) { m_infoLabel->setText(s); }
void PairingScreen::setError(const QString &s) { m_errorLabel->setText(s); }

void PairingScreen::requestKey() {
  const QString ip = m_ip->text().trimmed();
  if (ip.isEmpty()) {
    setError("Введите IP телевизора");
    return;
  }

  setError({});
  setInfo({});
  setLoading(true);

  m_activeClient = m_container->createClient(ip);
  connect(m_activeClient, &lgremote::Client::pairingKeyResult, this,
          [this](bool ok) {
            setLoading(false);
            if (ok)
              setInfo("Смотрите код на экране ТВ и введите его ниже");
            else
              setError("Телевизор не принял запрос");
          });
  m_activeClient->requestPairingKey();
}

void PairingScreen::confirm() {
  const QString ip = m_ip->text().trimmed();
  const QString key = m_key->text().trimmed();
  if (ip.isEmpty()) {
    setError("Введите IP");
    return;
  }
  if (key.length() != 6) {
    setError("Код — 6 цифр");
    return;
  }

  setError({});
  setInfo({});
  setLoading(true);

  m_activeClient = m_container->createClient(ip);
  connect(m_activeClient, &lgremote::Client::pairingConfirmResult, this,
          [this, ip](const QString &session) {
            setLoading(false);
            if (session.isEmpty()) {
              setError("Телевизор отклонил код");
              return;
            }
            m_container->saveSession(ip, session);
            emit connected();
          });
  m_activeClient->confirmPairing(key);
}