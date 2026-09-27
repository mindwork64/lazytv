#include "ui/widgets/Keypad.hpp"
#include "theme/Theme.hpp"

#include <QPainter>

KeypadKey::KeypadKey(int digit, QWidget *parent)
    : QAbstractButton(parent), m_digit(digit) {
  setCursor(Qt::PointingHandCursor);
  // См. IconButton: клавиатуру обрабатывает RemoteScreen.
  setFocusPolicy(Qt::NoFocus);
  setMinimumHeight(64);
  connect(&ThemeManager::instance(), &ThemeManager::changed, this,
          qOverload<>(&QWidget::update));
}

void KeypadKey::setFlash(bool v) {
  if (m_flash == v)
    return;
  m_flash = v;
  update();
}

void KeypadKey::paintEvent(QPaintEvent *) {
  const auto &p = ThemeManager::instance().palette();
  QPainter g(this);
  g.setRenderHint(QPainter::Antialiasing);

  const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
  g.setBrush(m_flash ? p.primary : p.surface);
  g.setPen(QPen(m_flash ? p.primary : p.outline, 1));
  g.drawRoundedRect(r, 16, 16);

  if (m_digit >= 0) {
    QFont f = font();
    f.setPointSize(22);
    f.setWeight(QFont::Light);
    g.setFont(f);
    g.setPen(m_flash ? p.onPrimary : p.onSurface);
    g.drawText(rect(), Qt::AlignCenter, QString::number(m_digit));
  } else {
    const QSize is(28, 28);
    const QPixmap pm = tintedSvg(":/icons/arrow_back.svg",
                                 m_flash ? p.onPrimary : p.onSurface, is);
    g.drawPixmap((width() - is.width()) / 2, (height() - is.height()) / 2, pm);
  }
}