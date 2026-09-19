#include "ui/widgets/IconButton.hpp"
#include "theme/Theme.hpp"

#include <QPainter>

IconButton::IconButton(QString svgPath, QString label, QWidget *parent)
    : QAbstractButton(parent), m_svg(std::move(svgPath)),
      m_label(std::move(label)) {
  setCursor(Qt::PointingHandCursor);
  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

QSize IconButton::sizeHint() const { return {72, 72}; }

void IconButton::setLabelVisible(bool v) {
  m_labelVisible = v;
  update();
}

void IconButton::paintEvent(QPaintEvent *) {
  const auto &p = ThemeManager::instance().palette();
  QPainter g(this);
  g.setRenderHint(QPainter::Antialiasing);

  const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);

  // Фон
  g.setBrush(p.surface);
  g.setPen(QPen(p.outline, 1));
  g.drawRoundedRect(r, 14, 14);

  // Иконка
  const QSize iconSize(24, 24);
  const QPixmap pm = tintedSvg(m_svg, p.onSurface, iconSize);

  int iconY = m_labelVisible ? (height() - iconSize.height() - 16) / 2
                             : (height() - iconSize.height()) / 2;
  g.drawPixmap((width() - iconSize.width()) / 2, iconY, pm);

  // Подпись
  if (m_labelVisible) {
    QFont f = font();
    f.setPointSizeF(8.0);
    g.setFont(f);
    g.setPen(p.onSurfaceVariant);
    g.drawText(QRect(0, iconY + iconSize.height() + 2, width(), 14),
               Qt::AlignHCenter | Qt::AlignTop, m_label);
  }
}