#include "ui/widgets/RockerColumn.hpp"
#include "theme/Theme.hpp"

#include <QMouseEvent>
#include <QPainter>

RockerColumn::RockerColumn(const QString &label, QWidget *parent)
    : QWidget(parent), m_label(label) {
  setMinimumSize(80, 160);
  setCursor(Qt::PointingHandCursor);

  m_repeatTimer.setInterval(200);
  connect(&m_repeatTimer, &QTimer::timeout, this, [this]() {
    if (m_pressed == Half::Plus)
      emit plus();
    if (m_pressed == Half::Minus)
      emit minus();
  });

  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

void RockerColumn::recompute() {
  const QRectF r = rect();
  const qreal labelH = 18;
  const qreal halfH = (r.height() - labelH) / 2.0;

  m_plusRect = QRectF(r.left(), r.top(), r.width(), halfH);
  m_labelRect = QRectF(r.left(), r.top() + halfH, r.width(), labelH);
  m_minusRect = QRectF(r.left(), r.top() + halfH + labelH, r.width(), halfH);
}

void RockerColumn::paintEvent(QPaintEvent *) {
  recompute();
  const auto &p = ThemeManager::instance().palette();

  QPainter g(this);
  g.setRenderHint(QPainter::Antialiasing);

  g.setBrush(p.surface);
  g.setPen(QPen(p.outline, 1));
  g.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 14, 14);

  QFont big = font();
  big.setPointSize(20);
  big.setWeight(QFont::Light);
  g.setFont(big);
  g.setPen(p.onSurface);
  g.drawText(m_plusRect, Qt::AlignCenter, "+");
  g.drawText(m_minusRect, Qt::AlignCenter, "−");

  QFont small = font();
  small.setPointSize(9);
  g.setFont(small);
  g.setPen(p.onSurfaceVariant);
  g.drawText(m_labelRect, Qt::AlignCenter, m_label);
}

void RockerColumn::fireOnce(Half h) {
  if (h == Half::Plus)
    emit plus();
  if (h == Half::Minus)
    emit minus();
}

void RockerColumn::mousePressEvent(QMouseEvent *e) {
  const QPointF pos = e->position();
  Half h = Half::None;
  if (m_plusRect.contains(pos))
    h = Half::Plus;
  else if (m_minusRect.contains(pos))
    h = Half::Minus;

  if (h != Half::None) {
    m_pressed = h;
    fireOnce(h);
    m_repeatTimer.start();
  }
}

void RockerColumn::mouseReleaseEvent(QMouseEvent *) {
  m_pressed = Half::None;
  m_repeatTimer.stop();
}