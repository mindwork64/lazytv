#include "ui/RemoteScreen.hpp"

#include <QAction>
#include <QDateTime>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <lazytv/app_container.hpp>
#include <lazytv/client.hpp>

#include "theme/Theme.hpp"
#include "ui/HotkeyHandler.hpp"
#include "ui/widgets/DPad.hpp"
#include "ui/widgets/IconButton.hpp"
#include "ui/widgets/Keypad.hpp"
#include "ui/widgets/NowPlayingBar.hpp"
#include "ui/widgets/RockerColumn.hpp"

using Cmd = lazytv::Client::Command;

namespace {

/** Длительность подсветки кнопки при нажатии горячей клавиши, мс. */
constexpr int kFlashMs = 120;

/** Страницы QStackedWidget. */
constexpr int kMainPage = 0;
constexpr int kNumbersPage = 1;
constexpr int kChannelsPage = 2;
constexpr int kScreenPage = 3;

/** Период фонового опроса состояния ТВ, мс. */
constexpr int kInfoPollMs = 5'000;

/** Пауза после команды перед внеочередным опросом, мс. */
constexpr int kRefreshAfterChannelMs = 700;
constexpr int kRefreshAfterVolumeMs = 500;
constexpr int kRefreshAfterPowerMs = 1'500;
constexpr int kRefreshAfterDigitMs = 1'200;
constexpr int kRefreshAfterInputMs = 700;

QString shortError(int httpStatus) {
    return httpStatus == 0
               ? QStringLiteral("Телевизор не отвечает")
               : QStringLiteral("Ошибка телевизора (HTTP %1)")
                     .arg(httpStatus);
}

QWidget* wrapColumn(std::initializer_list<QWidget*> widgets, int spacing = 14) {
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(spacing);
    for (auto* c : widgets) l->addWidget(c);
    return w;
}

QWidget* wrapRow(std::initializer_list<QWidget*> widgets, int spacing = 12) {
    auto* w = new QWidget;
    auto* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(spacing);
    for (auto* c : widgets) l->addWidget(c);
    return w;
}

} // namespace

RemoteScreen::RemoteScreen(lazytv::AppContainer* container, QWidget* parent)
    : QWidget(parent), m_container(container) {

    // Фокус держим на себе: клавиатура — это пульт (см. keyPressEvent).
    setFocusPolicy(Qt::StrongFocus);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 12, 20, 12);
    root->setSpacing(14);

    m_statusBar = new StatusBar(this);
    connect(m_statusBar, &StatusBar::settingsClicked,
            this, &RemoteScreen::openSettings);
    root->addWidget(m_statusBar);

    m_nowPlaying = new NowPlayingBar(this);
    connect(m_nowPlaying, &NowPlayingBar::menuRequested,
            this, &RemoteScreen::showMoreMenu);
    root->addWidget(m_nowPlaying);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(buildMainPage());
    m_pages->addWidget(buildNumbersPage());
    m_pages->addWidget(buildChannelsPage());
    m_pages->addWidget(buildScreenPage());
    root->addWidget(m_pages, 1);

    m_hotkeys = new HotkeyHandler(this);
    m_hotkeys->setEnabled(m_container->store().hotkeysEnabled());
    connect(m_hotkeys, &HotkeyHandler::commandRequested, this,
            [this](Cmd cmd) {
                flashButton(cmd);
                sendCommand(cmd);
            });
    connect(m_hotkeys, &HotkeyHandler::switchPage, this, [this] {
        m_pages->setCurrentIndex(m_pages->currentIndex() == kMainPage
                                     ? kNumbersPage
                                     : kMainPage);
    });

    m_errorBanner = new QLabel(this);
    m_errorBanner->setWordWrap(true);
    m_errorBanner->setVisible(false);
    root->addWidget(m_errorBanner);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(5'000);
    connect(m_statusTimer, &QTimer::timeout,
            this, &RemoteScreen::recomputeStatus);
    m_statusTimer->start();

    // Опрос состояния идёт только пока пульт виден (см. showEvent/hideEvent).
    m_infoTimer = new QTimer(this);
    m_infoTimer->setInterval(kInfoPollMs);
    connect(m_infoTimer, &QTimer::timeout,
            this, &RemoteScreen::refreshInfo);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &RemoteScreen::refreshInfo);

    connect(&ThemeManager::instance(), &ThemeManager::changed, this, [this]() {
        m_statusBar->update();
        updateErrorBannerStyle();
    });

    // Если клиент создан и ТВ отверг сессию — переключаемся на сопряжение.
    if (auto* client = m_container->getClient()) {
        connectClientSignals(client);
    }
}

void RemoteScreen::showEvent(QShowEvent* /*event*/) {
    if (!m_container->store().session()) {
        emit disconnected();
        return;
    }
    m_statusBar->setIp(m_container->store().ip().value_or(QString()));
    // Без явного фокуса после возврата из «Настройки» клавиши
    // уходят экрану настроек, а не пульту.
    setFocus(Qt::OtherFocusReason);

    m_infoTimer->start();
    refreshInfo();

    if (m_pages->currentIndex() == kChannelsPage)
        refreshChannelList();
    else if (m_pages->currentIndex() == kScreenPage)
        refreshScreenImage();
}

void RemoteScreen::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (m_infoTimer) m_infoTimer->stop();
    if (m_refreshTimer) m_refreshTimer->stop();
}

void RemoteScreen::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_pages && m_pages->currentIndex() == kScreenPage)
        updateScreenImage();
}

void RemoteScreen::setHotkeysEnabled(bool v) {
    m_hotkeys->setEnabled(v);
}

void RemoteScreen::keyPressEvent(QKeyEvent* event) {
    if (m_hotkeys->handle(event)) {
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

QWidget* RemoteScreen::buildMainPage() {
    auto* page = new QWidget;

    auto* power = new IconButton(":/icons/power.svg", "", page);
    power->setLabelVisible(false);
    power->setFixedSize(56, 56);
    m_iconByCmd.insert(static_cast<int>(Cmd::Power), power);
    connect(power, &QAbstractButton::clicked, this,
            [this] { sendCommand(Cmd::Power); });

    auto* powerWrap = new QWidget;
    auto* powerLayout = new QHBoxLayout(powerWrap);
    powerLayout->setContentsMargins(0, 0, 0, 0);
    powerLayout->addWidget(power);
    powerLayout->addStretch();

    m_vol = new RockerColumn("VOL", page);
    connect(m_vol, &RockerColumn::plus,  this, [this] { sendCommand(Cmd::VolumeUp);   });
    connect(m_vol, &RockerColumn::minus, this, [this] { sendCommand(Cmd::VolumeDown); });

    auto* home = new IconButton(":/icons/home.svg", "HOME", page);
    home->setMinimumHeight(72);
    m_iconByCmd.insert(static_cast<int>(Cmd::HomeMenu), home);
    connect(home, &QAbstractButton::clicked, this,
            [this] { sendCommand(Cmd::HomeMenu); });

    auto* exitBtn = new IconButton(":/icons/exit.svg", "EXIT", page);
    exitBtn->setMinimumHeight(72);
    m_iconByCmd.insert(static_cast<int>(Cmd::Exit), exitBtn);
    connect(exitBtn, &QAbstractButton::clicked, this,
            [this] { sendCommand(Cmd::Exit); });

    auto* homeExitColumn = wrapColumn({home, exitBtn}, 12);

    m_ch = new RockerColumn("CH", page);
    connect(m_ch, &RockerColumn::plus,  this, [this] { sendCommand(Cmd::ChannelUp);   });
    connect(m_ch, &RockerColumn::minus, this, [this] { sendCommand(Cmd::ChannelDown); });

    auto* topRow = new QWidget;
    auto* topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(12);
    topLayout->addWidget(m_vol, 1);
    topLayout->addWidget(homeExitColumn, 12);
    topLayout->addWidget(m_ch, 1);

    auto makeBtn = [&](const QString& svg, const QString& lbl, Cmd cmd) -> IconButton* {
        auto* b = new IconButton(svg, lbl, page);
        b->setMinimumHeight(72);
        m_iconByCmd.insert(static_cast<int>(cmd), b);
        connect(b, &QAbstractButton::clicked, this,
                [this, cmd] { sendCommand(cmd); });
        return b;
    };

    IconButton* mute  = makeBtn(":/icons/mute.svg",  "MUTE",  Cmd::MuteToggle);
    IconButton* back  = makeBtn(":/icons/back.svg",  "BACK",  Cmd::Back);

    IconButton* kbd = new IconButton(":/icons/kbd.svg", "KBD", page);
    kbd->setMinimumHeight(72);
    connect(kbd, &QAbstractButton::clicked, this,
            [this] { m_pages->setCurrentIndex(kNumbersPage); });

    IconButton* input = makeBtn(":/icons/input.svg", "INPUT", Cmd::ExternalInput);

    auto* midRow = wrapRow({mute, back, kbd, input}, 10);

    m_dpad = new DPad(page);
    connect(m_dpad, &DPad::up,    this, [this] { sendCommand(Cmd::Up);    });
    connect(m_dpad, &DPad::down,  this, [this] { sendCommand(Cmd::Down);  });
    connect(m_dpad, &DPad::left,  this, [this] { sendCommand(Cmd::Left);  });
    connect(m_dpad, &DPad::right, this, [this] { sendCommand(Cmd::Right); });
    connect(m_dpad, &DPad::ok,    this, [this] { sendCommand(Cmd::Ok);    });

    IconButton* info = makeBtn(":/icons/info.svg", "INFO", Cmd::Info);
    info->setFixedWidth(96);

    auto* infoWrap = new QWidget;
    auto* infoLayout = new QHBoxLayout(infoWrap);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->addStretch();
    infoLayout->addWidget(info);
    infoLayout->addStretch();

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    layout->addWidget(powerWrap);
    layout->addWidget(topRow);
    layout->addWidget(midRow);
    layout->addWidget(m_dpad, 1, Qt::AlignHCenter);
    layout->addWidget(infoWrap);

    return page;
}

QWidget* RemoteScreen::buildNumbersPage() {
    auto* page = new QWidget;
    auto* grid = new QGridLayout(page);
    grid->setContentsMargins(4, 0, 4, 0);
    grid->setSpacing(16);

    for (int i = 0; i < 9; ++i) {
        const int digit = i + 1;
        auto* key = new KeypadKey(digit, page);
        m_keyByDigit.insert(digit, key);
        connect(key, &QAbstractButton::clicked, this,
                [this, digit] { sendCommand(lazytv::Client::digit(digit)); });
        grid->addWidget(key, i / 3, i % 3);
    }

    auto* zero = new KeypadKey(0, page);
    m_keyByDigit.insert(0, zero);
    connect(zero, &QAbstractButton::clicked, this,
            [this] { sendCommand(lazytv::Client::digit(0)); });
    grid->addWidget(zero, 3, 1);

    auto* backKey = new KeypadKey(-1, page);
    connect(backKey, &QAbstractButton::clicked, this,
            [this] { m_pages->setCurrentIndex(kMainPage); });
    grid->addWidget(backKey, 3, 2);

    return page;
}

QWidget* RemoteScreen::buildChannelsPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(10);

    auto* back = new QPushButton(QStringLiteral("← Назад"), page);
    back->setFocusPolicy(Qt::NoFocus);
    back->setCursor(Qt::PointingHandCursor);
    connect(back, &QPushButton::clicked, this,
            [this] { m_pages->setCurrentIndex(kMainPage); });

    auto* title = new QLabel(QStringLiteral("Каналы"), page);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(13.0);
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_channelRefresh = new QPushButton(QStringLiteral("Обновить"), page);
    m_channelRefresh->setFocusPolicy(Qt::NoFocus);
    m_channelRefresh->setCursor(Qt::PointingHandCursor);
    connect(m_channelRefresh, &QPushButton::clicked,
            this, &RemoteScreen::refreshChannelList);

    header->addWidget(back);
    header->addWidget(title, 1);
    header->addWidget(m_channelRefresh);
    layout->addLayout(header);

    m_channelListHint = new QLabel(QStringLiteral("Загрузка списка каналов…"),
                                   page);
    m_channelListHint->setAlignment(Qt::AlignCenter);
    m_channelListHint->setWordWrap(true);
    layout->addWidget(m_channelListHint);

    m_channelList = new QListWidget(page);
    m_channelList->setFocusPolicy(Qt::NoFocus);
    m_channelList->setAlternatingRowColors(true);
    m_channelList->setUniformItemSizes(true);
    m_channelList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_channelList->setVisible(false);
    connect(m_channelList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
        if (!item) return;
        const int index = item->data(Qt::UserRole).toInt();
        if (index < 0 || index >= m_channels.size()) return;

        auto* client = m_container->getClient();
        if (!client) return;
        connectClientSignals(client);
        client->changeChannel(m_channels.at(index));
        m_pages->setCurrentIndex(kMainPage);
        scheduleInfoRefresh(kRefreshAfterChannelMs);
    });
    layout->addWidget(m_channelList, 1);

    return page;
}

QWidget* RemoteScreen::buildScreenPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(10);

    auto* back = new QPushButton(QStringLiteral("← Назад"), page);
    back->setFocusPolicy(Qt::NoFocus);
    back->setCursor(Qt::PointingHandCursor);
    connect(back, &QPushButton::clicked, this,
            [this] { m_pages->setCurrentIndex(kMainPage); });

    auto* title = new QLabel(QStringLiteral("Экран ТВ"), page);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(13.0);
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_screenRefresh = new QPushButton(QStringLiteral("Обновить"), page);
    m_screenRefresh->setFocusPolicy(Qt::NoFocus);
    m_screenRefresh->setCursor(Qt::PointingHandCursor);
    connect(m_screenRefresh, &QPushButton::clicked,
            this, &RemoteScreen::refreshScreenImage);

    header->addWidget(back);
    header->addWidget(title, 1);
    header->addWidget(m_screenRefresh);
    layout->addLayout(header);

    m_screenImage = new QLabel(page);
    m_screenImage->setAlignment(Qt::AlignCenter);
    m_screenImage->setMinimumSize(320, 180);
    m_screenImage->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Expanding);
    m_screenImage->setStyleSheet(
        QStringLiteral("QLabel { background-color: #000000; "
                       "border-radius: 8px; }"));
    layout->addWidget(m_screenImage, 1);

    m_screenHint = new QLabel(
        QStringLiteral("Нажмите «Обновить», чтобы получить кадр"), page);
    m_screenHint->setAlignment(Qt::AlignCenter);
    m_screenHint->setWordWrap(true);
    layout->addWidget(m_screenHint);

    return page;
}

void RemoteScreen::flashButton(Cmd cmd) {
    const int id = static_cast<int>(cmd);

    // Цифры: Number0..Number9 → KeypadKey с digit 0..9.
    if (cmd >= Cmd::Number0 && cmd <= Cmd::Number9) {
        if (auto* key = m_keyByDigit.value(id - static_cast<int>(Cmd::Number0))) {
            key->setFlash(true);
            QTimer::singleShot(kFlashMs, key, [key] { key->setFlash(false); });
        }
        return;
    }

    if (auto* btn = m_iconByCmd.value(id)) {
        btn->setFlash(true);
        QTimer::singleShot(kFlashMs, btn, [btn] { btn->setFlash(false); });
        return;
    }

    if (!m_dpad || !m_vol || !m_ch)
        return;

    switch (cmd) {
        case Cmd::Up:    m_dpad->flash(DPad::Part::Up, kFlashMs);    break;
        case Cmd::Down:  m_dpad->flash(DPad::Part::Down, kFlashMs);  break;
        case Cmd::Left:  m_dpad->flash(DPad::Part::Left, kFlashMs);  break;
        case Cmd::Right: m_dpad->flash(DPad::Part::Right, kFlashMs); break;
        case Cmd::Ok:    m_dpad->flash(DPad::Part::Center, kFlashMs); break;

        case Cmd::VolumeUp:
            m_vol->flash(RockerColumn::Half::Plus, kFlashMs);
            break;
        case Cmd::VolumeDown:
            m_vol->flash(RockerColumn::Half::Minus, kFlashMs);
            break;
        case Cmd::ChannelUp:
            m_ch->flash(RockerColumn::Half::Plus, kFlashMs);
            break;
        case Cmd::ChannelDown:
            m_ch->flash(RockerColumn::Half::Minus, kFlashMs);
            break;

        default:
            break;
    }
}

void RemoteScreen::connectClientSignals(lazytv::Client* client) {
    if (!client || client == m_connectedClient) return;

    m_connectedClient = client;

    // Указатель нужен, чтобы после пересопряжения подключиться к новому
    // клиенту; Qt сам отключает сигналы при удалении клиента.
    connect(client, &QObject::destroyed, this, [this, client] {
        if (m_connectedClient == client)
            m_connectedClient = nullptr;
    });

    connect(client, &lazytv::Client::commandResult,
            this, &RemoteScreen::onCommandResult);
    connect(client, &lazytv::Client::sessionExpired,
            this, &RemoteScreen::disconnected);

    connect(client, &lazytv::Client::currentChannelResult, this,
            [this](const lazytv::ChannelInfo& channel) {
        m_nowPlaying->setChannel(channel);
    });

    connect(client, &lazytv::Client::volumeInfoResult, this,
            [this](const lazytv::VolumeInfo& volume) {
        m_nowPlaying->setVolume(volume);
    });

    connect(client, &lazytv::Client::channelListResult, this,
            [this](const QVector<lazytv::ChannelInfo>& channels) {
        m_channels = channels;
        m_channelList->clear();

        for (int i = 0; i < m_channels.size(); ++i) {
            const auto& channel = m_channels.at(i);
            QString text = channel.number();
            const QString name = channel.name();
            if (!name.isEmpty())
                text += (text.isEmpty() ? QString() : QStringLiteral("  "))
                        + name;
            if (text.isEmpty())
                text = QStringLiteral("Без названия");

            auto* item = new QListWidgetItem(text, m_channelList);
            item->setData(Qt::UserRole, i);
        }

        const bool empty = m_channels.isEmpty();
        m_channelList->setVisible(!empty);
        m_channelListHint->setText(
            empty ? QStringLiteral("Список каналов пуст")
                  : QString());
        m_channelListHint->setVisible(empty);
    });

    connect(client, &lazytv::Client::screenImageResult, this,
            [this](const QByteArray& jpeg) {
        QPixmap pixmap;
        if (!pixmap.loadFromData(jpeg)) {
            m_screenHint->setText(QStringLiteral("Не удалось декодировать кадр"));
            m_screenHint->setVisible(true);
            return;
        }
        m_screenPixmap = pixmap;
        m_screenHint->setVisible(false);
        updateScreenImage();
    });

    connect(client, &lazytv::Client::queryFailed, this,
            [this](lazytv::Client::Query query, int httpStatus) {
        if (query == lazytv::Client::Query::ChannelList) {
            m_channelListHint->setText(
                QStringLiteral("Не удалось получить список каналов: %1")
                    .arg(shortError(httpStatus)));
            m_channelListHint->setVisible(true);
        } else if (query == lazytv::Client::Query::ScreenImage) {
            m_screenHint->setText(
                QStringLiteral("Не удалось получить кадр: %1")
                    .arg(shortError(httpStatus)));
            m_screenHint->setVisible(true);
        }
    });
}

void RemoteScreen::refreshInfo() {
    auto* client = m_container->getClient();
    if (!client) {
        m_nowPlaying->setUnavailable(QStringLiteral("Сессия не установлена"));
        return;
    }

    connectClientSignals(client);
    client->requestCurrentChannel();
    client->requestVolumeInfo();
}

void RemoteScreen::scheduleInfoRefresh(int delayMs) {
    // Внеочередной опрос имеет смысл только пока пульт виден.
    if (!m_infoTimer->isActive()) return;
    m_refreshTimer->start(delayMs);
}

void RemoteScreen::refreshChannelList() {
    auto* client = m_container->getClient();
    if (!client) {
        m_channelListHint->setText(QStringLiteral("Сессия не установлена"));
        m_channelListHint->setVisible(true);
        return;
    }

    connectClientSignals(client);
    if (m_channels.isEmpty()) {
        m_channelListHint->setText(
            QStringLiteral("Загрузка списка каналов…"));
        m_channelListHint->setVisible(true);
    }
    client->requestChannelList();
}

void RemoteScreen::refreshScreenImage() {
    auto* client = m_container->getClient();
    if (!client) {
        m_screenHint->setText(QStringLiteral("Сессия не установлена"));
        m_screenHint->setVisible(true);
        return;
    }

    connectClientSignals(client);
    m_screenHint->setText(QStringLiteral("Загрузка кадра…"));
    m_screenHint->setVisible(true);
    client->requestScreenImage();
}

void RemoteScreen::updateScreenImage() {
    if (!m_screenImage || m_screenPixmap.isNull()) return;

    const QSize target = m_screenImage->size();
    if (target.isEmpty()) return;

    m_screenImage->setPixmap(m_screenPixmap.scaled(
        target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void RemoteScreen::openChannelsPage() {
    m_pages->setCurrentIndex(kChannelsPage);
    refreshChannelList();
}

void RemoteScreen::openScreenPage() {
    m_pages->setCurrentIndex(kScreenPage);
    updateScreenImage();
    refreshScreenImage();
}

void RemoteScreen::showMoreMenu(const QPoint& globalPos) {
    QMenu menu(this);
    QAction* channels = menu.addAction(QStringLiteral("Каналы"));
    QAction* screen = menu.addAction(QStringLiteral("Экран ТВ"));
    menu.addSeparator();
    QAction* refresh = menu.addAction(QStringLiteral("Обновить"));

    QAction* chosen = menu.exec(globalPos);
    if (chosen == channels) {
        openChannelsPage();
    } else if (chosen == screen) {
        openScreenPage();
    } else if (chosen == refresh) {
        refreshInfo();
        if (m_pages->currentIndex() == kChannelsPage)
            refreshChannelList();
        else if (m_pages->currentIndex() == kScreenPage)
            refreshScreenImage();
    }
}

void RemoteScreen::sendCommand(Cmd cmd) {
    auto* client = m_container->getClient();
    if (!client) {
        m_lastAttemptFailed = true;
        m_errorBanner->setText("Сессия не установлена");
        updateErrorBannerStyle();
        m_errorBanner->setVisible(true);
        recomputeStatus();
        return;
    }

    connectClientSignals(client);
    client->sendCommand(cmd);

    switch (cmd) {
        case Cmd::VolumeUp:
        case Cmd::VolumeDown:
        case Cmd::MuteToggle:
            scheduleInfoRefresh(kRefreshAfterVolumeMs);
            break;

        case Cmd::ChannelUp:
        case Cmd::ChannelDown:
        case Cmd::PreviousChannel:
        case Cmd::FavoriteChannel:
        case Cmd::LiveTv:
            scheduleInfoRefresh(kRefreshAfterChannelMs);
            break;

        case Cmd::ExternalInput:
            scheduleInfoRefresh(kRefreshAfterInputMs);
            break;

        case Cmd::Power:
            scheduleInfoRefresh(kRefreshAfterPowerMs);
            break;

        default:
            if (cmd >= Cmd::Number0 && cmd <= Cmd::Number9)
                scheduleInfoRefresh(kRefreshAfterDigitMs);
            break;
    }
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
    if (!m_errorBanner) return;
    const auto& p = ThemeManager::instance().palette();
    m_errorBanner->setStyleSheet(
        QStringLiteral("QLabel { background-color: %1; color: %2; "
                       "border-radius: 10px; padding: 10px; }")
            .arg(p.errorContainer.name(), p.onErrorContainer.name()));
}