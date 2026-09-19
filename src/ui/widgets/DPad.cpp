#include "ui/widgets/DPad.hpp"
#include "theme/Theme.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

DPad::DPad(QWidget *parent) : QWidget(parent) {
  setMinimumSize(200, 200);
  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

void DPad::recompute() {
  const QRectF r = rect();
  const qreal s = qMin(r.width(), r.height());
  const QPointF c = r.center();

  m_center = QRectF(c.x() - s * 0.12, c.y() - s * 0.12, s * 0.24, s * 0.24);

  const qreal t = s * 0.27;
  m_up = QRectF(c.x() - t / 2, r.top(), t, t);
  m_down = QRectF(c.x() - t / 2, r.bottom() - t, t, t);
  m_left = QRectF(r.left(), c.y() - t / 2, t, t);
  m_right = QRectF(r.right() - t, c.y() - t / 2, t, t);
}

void DPad::paintEvent(QPaintEvent *) {
  recompute();
  const auto &p = ThemeManager::instance().palette();

  QPainter g(this);
  g.setRenderHint(QPainter::Antialiasing);

  // Внешний круг
  g.setBrush(p.surface);
  g.setPen(QPen(p.primary, 2));
  g.drawEllipse(rect().adjusted(1, 1, -1, -1));

  // Треугольники
  const QSize ts(18, 18);
  const QPixmap pm = tintedSvg(":/icons/triangle.svg", p.onSurface, ts);

  auto drawTriangle = [&](const QRectF &box, qreal angle) {
    g.save();
    g.translate(box.center());
    g.rotate(angle);
    g.drawPixmap(-ts.width() / 2, -ts.height() / 2, pm);
    g.restore();
  };

  drawTriangle(m_up, 0);
  drawTriangle(m_right, 90);
  drawTriangle(m_down, 180);
  drawTriangle(m_left, 270);

  // Центральная точка
  g.setBrush(p.onSurface);
  g.setPen(Qt::NoPen);
  g.drawEllipse(m_center);
}

void DPad::mousePressEvent(QMouseEvent *e) {
  const QPointF pos = e->position();
  if (m_up.contains(pos)) {
    emit up();
    m_pressed = &m_up;
  } else if (m_down.contains(pos)) {
    emit down();
    m_pressed = &m_down;
  } else if (m_left.contains(pos)) {
    emit left();
    m_pressed = &m_left;
  } else if (m_right.contains(pos)) {
    emit right();
    m_pressed = &m_right;
  } else if (m_center.contains(pos)) {
    emit ok();
    m_pressed = &m_center;
  }
}

void DPad::mouseReleaseEvent(QMouseEvent *) { m_pressed = nullptr; }