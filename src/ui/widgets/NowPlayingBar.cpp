#include "ui/widgets/NowPlayingBar.hpp"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "theme/Theme.hpp"

namespace {

constexpr int kBarHeight = 64;
constexpr int kMoreButtonSize = 28;

} // namespace

NowPlayingBar::NowPlayingBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(kBarHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_StyledBackground, false);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(14, 8, 10, 8);
    root->setSpacing(10);

    auto* textColumn = new QVBoxLayout;
    textColumn->setContentsMargins(0, 0, 0, 0);
    textColumn->setSpacing(0);

    m_channelLabel = new QLabel(this);
    m_channelLabel->setTextFormat(Qt::PlainText);
    QFont channelFont = m_channelLabel->font();
    channelFont.setPointSizeF(12.0);
    channelFont.setBold(true);
    m_channelLabel->setFont(channelFont);
    m_channelLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_channelLabel->setMinimumWidth(0);

    m_programLabel = new QLabel(this);
    m_programLabel->setTextFormat(Qt::PlainText);
    QFont programFont = m_programLabel->font();
    programFont.setPointSizeF(9.5);
    m_programLabel->setFont(programFont);
    m_programLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_programLabel->setMinimumWidth(0);

    textColumn->addWidget(m_channelLabel);
    textColumn->addWidget(m_programLabel);

    m_volumeLabel = new QLabel(this);
    QFont volumeFont = m_volumeLabel->font();
    volumeFont.setPointSizeF(9.5);
    volumeFont.setBold(true);
    m_volumeLabel->setFont(volumeFont);
    m_volumeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_volumeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    m_moreButton = new QPushButton(QStringLiteral("…"), this);
    m_moreButton->setFixedSize(kMoreButtonSize, kMoreButtonSize);
    m_moreButton->setFocusPolicy(Qt::NoFocus);
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setToolTip(QStringLiteral("Ещё"));
    connect(m_moreButton, &QPushButton::clicked, this, [this] {
        emit menuRequested(
            m_moreButton->mapToGlobal(QPoint(0, m_moreButton->height())));
    });

    root->addLayout(textColumn, 1);
    root->addWidget(m_volumeLabel);
    root->addWidget(m_moreButton);

    connect(&ThemeManager::instance(), &ThemeManager::changed,
            this, &NowPlayingBar::updateStyle);

    updateStyle();
    setUnavailable();
}

void NowPlayingBar::setChannel(const lazytv::ChannelInfo& channel) {
    const QString number = channel.number();
    const QString name = channel.name();

    if (!number.isEmpty() && !name.isEmpty())
        m_channelText = number + QStringLiteral("  ") + name;
    else
        m_channelText = number.isEmpty() ? name : number;

    if (m_channelText.isEmpty())
        m_channelText = QStringLiteral("Канал не определён");

    m_programText = channel.progName;
    applyElidedText();
}

void NowPlayingBar::setVolume(const lazytv::VolumeInfo& volume) {
    m_volumeMuted = volume.muted;
    m_volumeText = volume.muted
                       ? QStringLiteral("MUTE")
                       : QStringLiteral("VOL %1").arg(volume.level);
    m_volumeLabel->setText(m_volumeText);
    applyVolumeStyle();
}

void NowPlayingBar::setUnavailable(const QString& text) {
    m_channelText = text.isEmpty()
                        ? QStringLiteral("Нет данных с телевизора")
                        : text;
    m_programText.clear();
    m_volumeMuted = false;
    m_volumeText = QStringLiteral("VOL —");
    m_volumeLabel->setText(m_volumeText);
    applyVolumeStyle();
    applyElidedText();
}

void NowPlayingBar::applyElidedText() {
    const int channelWidth = qMax(0, m_channelLabel->width() - 2);
    const int programWidth = qMax(0, m_programLabel->width() - 2);

    m_channelLabel->setText(QFontMetrics(m_channelLabel->font())
                                .elidedText(m_channelText, Qt::ElideRight,
                                            channelWidth));
    m_programLabel->setText(QFontMetrics(m_programLabel->font())
                                .elidedText(m_programText, Qt::ElideRight,
                                            programWidth));

    QString tooltip = m_channelText;
    if (!m_programText.isEmpty())
        tooltip += QLatin1Char('\n') + m_programText;
    m_channelLabel->setToolTip(tooltip);
    m_programLabel->setToolTip(m_programText);
}

void NowPlayingBar::paintEvent(QPaintEvent* /*event*/) {
    const auto& p = ThemeManager::instance().palette();

    QPainter g(this);
    g.setRenderHint(QPainter::Antialiasing);
    g.setPen(QPen(p.outline, 1));
    g.setBrush(p.surface);
    g.drawRoundedRect(rect().adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
}

void NowPlayingBar::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    applyElidedText();
}

void NowPlayingBar::applyVolumeStyle() {
    const auto& p = ThemeManager::instance().palette();
    const QColor color = m_volumeMuted ? p.error : p.onSurfaceVariant;
    m_volumeLabel->setStyleSheet(
        QStringLiteral("QLabel { color: %1; background: transparent; }")
            .arg(color.name()));
}

void NowPlayingBar::updateStyle() {
    const auto& p = ThemeManager::instance().palette();

    m_channelLabel->setStyleSheet(
        QStringLiteral("QLabel { color: %1; background: transparent; }")
            .arg(p.onSurface.name()));
    m_programLabel->setStyleSheet(
        QStringLiteral("QLabel { color: %1; background: transparent; }")
            .arg(p.onSurfaceVariant.name()));
    m_moreButton->setStyleSheet(
        QStringLiteral("QPushButton { background: transparent; border: none; "
                       "border-radius: 6px; color: %1; font-size: 18px; "
                       "padding: 0; }"
                       "QPushButton:hover { background: %2; }")
            .arg(p.onSurfaceVariant.name(), p.surfaceVariant.name()));

    applyVolumeStyle();
    update();
}
