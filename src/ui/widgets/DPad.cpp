#include "ui/widgets/DPad.hpp"
#include "theme/Theme.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

DPad::DPad(QWidget *parent) : QWidget(parent) {
  setMinimumSize(200, 200);
  m_flashTimer.setSingleShot(true);
  connect(&m_flashTimer, &QTimer::timeout, this, [this]() {
    m_flashOn = false;
    update();
  });
  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

void DPad::flash(Part part, int ms) {
  m_flashPart = part;
  m_flashOn = true;
  update();
  m_flashTimer.start(ms);
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
  const QPixmap pmLit = tintedSvg(":/icons/triangle.svg", p.onPrimary, ts);

  auto drawTriangle = [&](const QRectF &box, qreal angle, bool lit) {
    if (lit) {
      g.setBrush(p.primary);
      g.setPen(Qt::NoPen);
      g.drawEllipse(box.center(), box.width() * 0.55, box.height() * 0.55);
    }
    g.save();
    g.translate(box.center());
    g.rotate(angle);
    const QPixmap &icon = lit ? pmLit : pm;
    g.drawPixmap(-ts.width() / 2, -ts.height() / 2, icon);
    g.restore();
  };

  const bool litUp    = m_flashOn && m_flashPart == Part::Up;
  const bool litDown  = m_flashOn && m_flashPart == Part::Down;
  const bool litLeft  = m_flashOn && m_flashPart == Part::Left;
  const bool litRight = m_flashOn && m_flashPart == Part::Right;
  const bool litOk    = m_flashOn && m_flashPart == Part::Center;

  drawTriangle(m_up, 0, litUp);
  drawTriangle(m_right, 90, litRight);
  drawTriangle(m_down, 180, litDown);
  drawTriangle(m_left, 270, litLeft);

  // Центральная точка: при подсветке «нажимается» — точка вырастает.
  g.setPen(Qt::NoPen);
  if (litOk) {
    g.setBrush(p.primary);
    g.drawEllipse(m_center.center(), m_center.width() * 0.95,
                  m_center.height() * 0.95);
    g.setBrush(p.onPrimary);
  } else {
    g.setBrush(p.onSurface);
  }
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