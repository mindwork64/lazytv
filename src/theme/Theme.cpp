#include "theme/Theme.hpp"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QProcess>
#include <QSvgRenderer>

ThemeManager &ThemeManager::instance() {
  static ThemeManager mgr;
  return mgr;
}

Palette ThemeManager::darkPalette() {
  return Palette{
      .background = QColor(0x1E, 0x1F, 0x22),
      .surface = QColor(0x2B, 0x2D, 0x30),
      .surfaceVariant = QColor(0x3C, 0x3F, 0x41),
      .onSurface = QColor(0xDF, 0xE1, 0xE5),
      .onSurfaceVariant = QColor(0xB4, 0xB8, 0xBF),
      .outline = QColor(0x4E, 0x51, 0x57),
      .primary = QColor(0xA5, 0x00, 0x34),
      .error = QColor(0xE5, 0xA0, 0x0D),
      .errorContainer = QColor(0x3A, 0x1A, 0x1A),
      .onErrorContainer = QColor(0xFF, 0xB4, 0xAB),
  };
}

Palette ThemeManager::lightPalette() {
  return Palette{
      .background = QColor(0xF7, 0xF8, 0xFA),
      .surface = QColor(0xFF, 0xFF, 0xFF),
      .surfaceVariant = QColor(0xEB, 0xEC, 0xF0),
      .onSurface = QColor(0x1E, 0x1F, 0x22),
      .onSurfaceVariant = QColor(0x6C, 0x70, 0x7E),
      .outline = QColor(0xD1, 0xD3, 0xD9),
      .primary = QColor(0xA5, 0x00, 0x34),
      .error = QColor(0xA5, 0x00, 0x34),
      .errorContainer = QColor(0xF5, 0xDA, 0xDD),
      .onErrorContainer = QColor(0x7A, 0x00, 0x28),
  };
}

bool ThemeManager::detectSystemDark() {
  QProcess p;
  p.start("gsettings", {"get", "org.gnome.desktop.interface", "color-scheme"});
  if (!p.waitForFinished(1000))
    return true;
  const QByteArray out = p.readAllStandardOutput().toLower();
  return out.contains("dark");
}

void ThemeManager::apply(int mode) {
  m_mode = mode;
  const bool dark = (mode == 1) || (mode == 0 && detectSystemDark());
  m_palette = dark ? darkPalette() : lightPalette();

  QPalette qp;
  qp.setColor(QPalette::Window, m_palette.background);
  qp.setColor(QPalette::WindowText, m_palette.onSurface);
  qp.setColor(QPalette::Base, m_palette.surface);
  qp.setColor(QPalette::AlternateBase, m_palette.surfaceVariant);
  qp.setColor(QPalette::Text, m_palette.onSurface);
  qp.setColor(QPalette::Button, m_palette.surface);
  qp.setColor(QPalette::ButtonText, m_palette.onSurface);
  qp.setColor(QPalette::Highlight, m_palette.primary);
  qp.setColor(QPalette::HighlightedText, Qt::white);
  qApp->setPalette(qp);

  qApp->setStyleSheet(buildStyleSheet(m_palette));
  emit changed();
}

QString ThemeManager::buildStyleSheet(const Palette &p) {
  return QStringLiteral(R"(
QWidget { background-color: %1; color: %2; }
QLineEdit {
    background-color: %3;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 8px 10px;
    color: %2;
    selection-background-color: %5;
}
QLineEdit:focus { border-color: %5; }
QPushButton {
    background-color: %3;
    border: 1px solid %4;
    border-radius: 8px;
    padding: 10px 16px;
    color: %2;
}
QPushButton:hover { background-color: %6; }
QPushButton:pressed { background-color: %4; }
QPushButton:disabled { color: %7; }
QLabel { background-color: transparent; color: %2; }
QRadioButton { color: %2; spacing: 8px; background-color: transparent; }
QRadioButton::indicator { width: 18px; height: 18px; }
QMainWindow { background-color: %1; }
)")
      .arg(p.background.name(), p.onSurface.name(), p.surface.name(),
           p.outline.name(), p.primary.name(), p.surfaceVariant.name(),
           p.onSurfaceVariant.name());
}

QPixmap tintedSvg(const QString &path, const QColor &color, const QSize &size) {
  QSvgRenderer renderer(path);
  QPixmap pm(size);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  renderer.render(&p);
  p.setCompositionMode(QPainter::CompositionMode_SourceIn);
  p.fillRect(pm.rect(), color);
  return pm;
}