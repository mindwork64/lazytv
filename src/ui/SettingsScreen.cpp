#include "ui/SettingsScreen.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <lazytv/app_container.hpp>

#include "theme/Theme.hpp"

SettingsScreen::SettingsScreen(lazytv::AppContainer *container, QWidget *parent)
    : QWidget(parent), m_container(container) {

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 16);
  root->setSpacing(16);

  auto *header = new QHBoxLayout;
  auto *backBtn = new QPushButton("← Назад", this);
  connect(backBtn, &QPushButton::clicked, this, &SettingsScreen::back);
  header->addWidget(backBtn);

  auto *title = new QLabel("Настройки", this);
  QFont f = title->font();
  f.setPointSize(14);
  title->setFont(f);
  header->addWidget(title);
  header->addStretch();
  root->addLayout(header);

  root->addWidget(new QLabel("Телевизор", this));

  m_ipLabel = new QLabel(this);
  root->addWidget(m_ipLabel);

  auto *disc = new QPushButton("Отключиться и забыть сессию", this);
  connect(disc, &QPushButton::clicked, this, &SettingsScreen::disconnect);
  root->addWidget(disc);

  auto mkLine = [this]() {
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    const auto &p = ThemeManager::instance().palette();
    line->setStyleSheet(QString("color: %1;").arg(p.outline.name()));
    return line;
  };
  root->addWidget(mkLine());

  root->addWidget(new QLabel("Тема", this));

  const int mode = m_container->store().themeMode();
  m_sys = new QRadioButton("Системная", this);
  m_dark = new QRadioButton("Тёмная", this);
  m_light = new QRadioButton("Светлая", this);
  m_sys->setChecked(mode == 0);
  m_dark->setChecked(mode == 1);
  m_light->setChecked(mode == 2);

  auto emitMode = [this]() {
    if (m_sys->isChecked())
      emit themeModeChanged(0);
    if (m_dark->isChecked())
      emit themeModeChanged(1);
    if (m_light->isChecked())
      emit themeModeChanged(2);
  };
  connect(m_sys, &QRadioButton::clicked, this, emitMode);
  connect(m_dark, &QRadioButton::clicked, this, emitMode);
  connect(m_light, &QRadioButton::clicked, this, emitMode);

  root->addWidget(m_sys);
  root->addWidget(m_dark);
  root->addWidget(m_light);

  root->addWidget(mkLine());

  root->addWidget(new QLabel("Версия", this));
  root->addWidget(new QLabel("1.1.0 (1)", this));

  root->addStretch();
  refresh();
}

void SettingsScreen::refresh() {
  m_ipLabel->setText("IP: " + m_container->store().ip().value_or("—"));
  const int mode = m_container->store().themeMode();
  m_sys->setChecked(mode == 0);
  m_dark->setChecked(mode == 1);
  m_light->setChecked(mode == 2);
}