#pragma once
#include <QColor>
#include <QObject>
#include <QPixmap>
#include <QSize>

struct Palette {
  QColor background;
  QColor surface;
  QColor surfaceVariant;
  QColor onSurface;
  QColor onSurfaceVariant;
  QColor outline;
  QColor primary;
  QColor error;
  QColor errorContainer;
  QColor onErrorContainer;
  QColor online{0x34, 0xC7, 0x59};
  QColor stale{0xE5, 0xA0, 0x0D};
  QColor offline{0x8E, 0x8E, 0x93};
};

class ThemeManager : public QObject {
  Q_OBJECT
public:
  static ThemeManager &instance();

  void apply(int mode);
  int mode() const { return m_mode; }
  const Palette &palette() const { return m_palette; }

signals:
  void changed();

private:
  ThemeManager() = default;
  Palette m_palette;
  int m_mode = 1;

  static Palette darkPalette();
  static Palette lightPalette();
  static bool detectSystemDark();
  static QString buildStyleSheet(const Palette &p);
};

/** Рендерит SVG в QPixmap с заданным цветом (через CompositionMode_SourceIn).
 */
QPixmap tintedSvg(const QString &path, const QColor &color, const QSize &size);