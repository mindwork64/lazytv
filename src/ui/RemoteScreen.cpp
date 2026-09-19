#include "ui/RemoteScreen.hpp"

#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "data/AppContainer.hpp"
#include "net/LgNetCastClient.hpp"
#include "theme/Theme.hpp"
#include "ui/widgets/DPad.hpp"
#include "ui/widgets/IconButton.hpp"
#include "ui/widgets/Keypad.hpp"
#include "ui/widgets/RockerColumn.hpp"

namespace {

QWidget *wrapColumn(std::initializer_list<QWidget *> widgets,
                    int spacing = 14) {
  auto *w = new QWidget;
  auto *l = new QVBoxLayout(w);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(spacing);
  for (auto *c : widgets)
    l->addWidget(c);
  return w;
}

QWidget *wrapRow(std::initializer_list<QWidget *> widgets, int spacing = 12) {
  auto *w = new QWidget;
  auto *l = new QHBoxLayout(w);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(spacing);
  for (auto *c : widgets)
    l->addWidget(c);
  return w;
}

} // namespace

RemoteScreen::RemoteScreen(AppContainer *container, QWidget *parent)
    : QWidget(parent), m_container(container) {

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(20, 12, 20, 12);
  root->setSpacing(14);

  m_statusBar = new StatusBar(this);
  connect(m_statusBar, &StatusBar::settingsClicked, this,
          &RemoteScreen::openSettings);
  root->addWidget(m_statusBar);

  m_pages = new QStackedWidget(this);
  m_pages->addWidget(buildMainPage());
  m_pages->addWidget(buildNumbersPage());
  root->addWidget(m_pages, 1);

  m_errorBanner = new QLabel(this);
  m_errorBanner->setWordWrap(true);
  m_errorBanner->setVisible(false);
  root->addWidget(m_errorBanner);

  m_statusTimer = new QTimer(this);
  m_statusTimer->setInterval(5000);
  connect(m_statusTimer, &QTimer::timeout, this,
          &RemoteScreen::recomputeStatus);
  m_statusTimer->start();

  connect(&ThemeManager::instance(), &ThemeManager::changed, this, [this]() {
    m_statusBar->update();
    updateErrorBannerStyle();
  });
}

void RemoteScreen::showEvent(QShowEvent * /*event*/) {
  if (!m_container->store().session())
    emit disconnected();
  m_statusBar->setIp(m_container->store().ip().value_or(QString()));
}

QWidget *RemoteScreen::buildMainPage() {
  auto *page = new QWidget;

  // Power — круглая кнопка
  auto *power = new IconButton(":/icons/power.svg", "", page);
  power->setLabelVisible(false);
  power->setFixedSize(56, 56);
  connect(power, &QAbstractButton::clicked, this,
          [this] { sendCommand(LgNetCastClient::POWER); });

  auto *powerWrap = new QWidget;
  auto *powerLayout = new QHBoxLayout(powerWrap);
  powerLayout->setContentsMargins(0, 0, 0, 0);
  powerLayout->addWidget(power);
  powerLayout->addStretch();

  // Рокер громкости
  auto *vol = new RockerColumn("VOL", page);
  connect(vol, &RockerColumn::plus, this,
          [this] { sendCommand(LgNetCastClient::VOLUME_UP); });
  connect(vol, &RockerColumn::minus, this,
          [this] { sendCommand(LgNetCastClient::VOLUME_DOWN); });

  // Home / Exit
  auto *home = new IconButton(":/icons/home.svg", "HOME", page);
  home->setMinimumHeight(72);
  connect(home, &QAbstractButton::clicked, this,
          [this] { sendCommand(LgNetCastClient::HOME_MENU); });

  auto *exitBtn = new IconButton(":/icons/exit.svg", "EXIT", page);
  exitBtn->setMinimumHeight(72);
  connect(exitBtn, &QAbstractButton::clicked, this,
          [this] { sendCommand(LgNetCastClient::EXIT); });

  auto *homeExitColumn = wrapColumn({home, exitBtn}, 12);

  // Рокер каналов
  auto *ch = new RockerColumn("CH", page);
  connect(ch, &RockerColumn::plus, this,
          [this] { sendCommand(LgNetCastClient::CHANNEL_UP); });
  connect(ch, &RockerColumn::minus, this,
          [this] { sendCommand(LgNetCastClient::CHANNEL_DOWN); });

  auto *topRow = new QWidget;
  auto *topLayout = new QHBoxLayout(topRow);
  topLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->setSpacing(12);
  topLayout->addWidget(vol, 1);
  topLayout->addWidget(homeExitColumn, 12);
  topLayout->addWidget(ch, 1);

  // Средний ряд
  auto makeBtn = [&](const QString &svg, const QString &lbl,
                     int cmd) -> QWidget * {
    auto *b = new IconButton(svg, lbl, page);
    b->setMinimumHeight(72);
    connect(b, &QAbstractButton::clicked, this,
            [this, cmd] { sendCommand(cmd); });
    return b;
  };

  QWidget *mute =
      makeBtn(":/icons/mute.svg", "MUTE", LgNetCastClient::MUTE_TOGGLE);
  QWidget *back = makeBtn(":/icons/back.svg", "BACK", LgNetCastClient::BACK);
  QWidget *kbd = makeBtn(":/icons/kbd.svg", "KBD", 0);
  QWidget *input =
      makeBtn(":/icons/input.svg", "INPUT", LgNetCastClient::EXTERNAL_INPUT);

  connect(kbd, &QWidget::customContextMenuRequested, this,
          [] {}); // no-op (заглушка)
  // Просто навешиваем клик через отдельный connect на сам IconButton — но kbd
  // уже указан как QWidget*, поэтому добавим переключение страницы иначе:
  // (сделаем это через dynamic_cast на QAbstractButton)
  if (auto *kbdBtn = qobject_cast<QAbstractButton *>(kbd)) {
    connect(kbdBtn, &QAbstractButton::clicked, this,
            [this] { m_pages->setCurrentIndex(1); });
  }

  auto *midRow = wrapRow({mute, back, kbd, input}, 10);

  // D-Pad
  auto *dpad = new DPad(page);
  connect(dpad, &DPad::up, this, [this] { sendCommand(LgNetCastClient::UP); });
  connect(dpad, &DPad::down, this,
          [this] { sendCommand(LgNetCastClient::DOWN); });
  connect(dpad, &DPad::left, this,
          [this] { sendCommand(LgNetCastClient::LEFT); });
  connect(dpad, &DPad::right, this,
          [this] { sendCommand(LgNetCastClient::RIGHT); });
  connect(dpad, &DPad::ok, this, [this] { sendCommand(LgNetCastClient::OK); });

  // INFO
  QWidget *info = makeBtn(":/icons/info.svg", "INFO", LgNetCastClient::INFO);
  info->setFixedWidth(96);

  auto *infoWrap = new QWidget;
  auto *infoLayout = new QHBoxLayout(infoWrap);
  infoLayout->setContentsMargins(0, 0, 0, 0);
  infoLayout->addStretch();
  infoLayout->addWidget(info);
  infoLayout->addStretch();

  // Компоновка
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(14);
  layout->addWidget(powerWrap);
  layout->addWidget(topRow);
  layout->addWidget(midRow);
  layout->addWidget(dpad, 1, Qt::AlignHCenter);
  layout->addWidget(infoWrap);

  return page;
}

QWidget *RemoteScreen::buildNumbersPage() {
  auto *page = new QWidget;
  auto *grid = new QGridLayout(page);
  grid->setContentsMargins(4, 0, 4, 0);
  grid->setSpacing(16);

  for (int i = 0; i < 9; ++i) {
    const int digit = i + 1;
    auto *key = new KeypadKey(digit, page);
    connect(key, &QAbstractButton::clicked, this,
            [this, digit] { sendCommand(LgNetCastClient::NUMBER_0 + digit); });
    grid->addWidget(key, i / 3, i % 3);
  }

  auto *zero = new KeypadKey(0, page);
  connect(zero, &QAbstractButton::clicked, this,
          [this] { sendCommand(LgNetCastClient::NUMBER_0); });
  grid->addWidget(zero, 3, 1);

  auto *backKey = new KeypadKey(-1, page);
  connect(backKey, &QAbstractButton::clicked, this,
          [this] { m_pages->setCurrentIndex(0); });
  grid->addWidget(backKey, 3, 2);

  return page;
}

void RemoteScreen::sendCommand(int cmd) {
  auto *client = m_container->getClient();
  if (!client) {
    m_lastAttemptFailed = true;
    m_errorBanner->setText("Сессия не установлена");
    updateErrorBannerStyle();
    m_errorBanner->setVisible(true);
    recomputeStatus();
    return;
  }
  connect(client, &LgNetCastClient::commandResult, this,
          &RemoteScreen::onCommandResult, Qt::UniqueConnection);
  client->sendCommand(cmd);
}

void RemoteScreen::onCommandResult(bool ok) {
  if (ok) {
    m_lastSuccessAt = QDateTime::currentMSecsSinceEpoch();
    m_lastAttemptFailed = false;
    m_errorBanner->setVisible(false);
  } else {
    m_lastAttemptFailed = true;
    m_errorBanner->setText("Телевизор не принял команду");
    updateErrorBannerStyle();
    m_errorBanner->setVisible(true);
  }
  recomputeStatus();
}

void RemoteScreen::recomputeStatus() {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  ConnectionStatus s;
  if (m_lastAttemptFailed) {
    s = ConnectionStatus::Offline;
  } else if (m_lastSuccessAt > 0 && now - m_lastSuccessAt < 60'000) {
    s = ConnectionStatus::Online;
  } else {
    s = ConnectionStatus::Stale;
  }
  m_status = s;
  m_statusBar->setStatus(s);
}

void RemoteScreen::updateErrorBannerStyle() {
  if (!m_errorBanner)
    return;
  const auto &p = ThemeManager::instance().palette();
  m_errorBanner->setStyleSheet(
      QStringLiteral("QLabel { background-color: %1; color: %2; "
                     "border-radius: 10px; padding: 10px; }")
          .arg(p.errorContainer.name(), p.onErrorContainer.name()));
}