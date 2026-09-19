#include "ui/widgets/StatusBar.hpp"
#include "theme/Theme.hpp"

#include <QMouseEvent>
#include <QPainter>

StatusBar::StatusBar(QWidget *parent) : QWidget(parent) {
  setFixedHeight(40);
  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

void StatusBar::setIp(const QString &ip) {
  m_ip = ip;
  update();
}
void StatusBar::setStatus(ConnectionStatus s) {
  m_status = s;
  update();
}

void StatusBar::paintEvent(QPaintEvent *) {
  const auto &p = ThemeManager::instance().palette();
  QPainter g(this);
  g.setRenderHint(QPainter::Antialiasing);

  QColor dot = p.offline;
  if (m_status == ConnectionStatus::Online)
    dot = p.online;
  else if (m_status == ConnectionStatus::Stale)
    dot = p.stale;

  g.setBrush(dot);
  g.setPen(Qt::NoPen);
  g.drawEllipse(QPointF(8, height() / 2.0), 4, 4);

  QFont f = font();
  f.setPointSizeF(10.0);
  g.setFont(f);
  g.setPen(p.onSurfaceVariant);

  QRect ipRect(20, 0, width() - 100, height());
  g.drawText(ipRect, Qt::AlignVCenter | Qt::AlignLeft, m_ip);

  const QString settingsText = QStringLiteral("Настройки");
  const QFontMetrics fm(f);
  const int textWidth = fm.horizontalAdvance(settingsText) + 16;
  m_settingsRect = QRect(width() - textWidth - 8, 0, textWidth, height());

  g.drawText(m_settingsRect, Qt::AlignCenter, settingsText);
}

void StatusBar::mousePressEvent(QMouseEvent *e) {
  if (m_settingsRect.contains(e->pos())) {
    emit settingsClicked();
  }
}