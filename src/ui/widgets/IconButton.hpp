#pragma once
#include <QAbstractButton>
#include <QColor>

class IconButton : public QAbstractButton {
  Q_OBJECT
public:
  IconButton(QString svgPath, QString label, QWidget *parent = nullptr);

  QSize sizeHint() const override;

  void setLabelVisible(bool v);

  /** Подсветка от горячей клавиши: кнопка на короткое время «нажимается». */
  void setFlash(bool v);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_svg;
  QString m_label;
  bool m_labelVisible = true;
  bool m_flash = false;
};