#pragma once
#include <QTimer>
#include <QWidget>

class RockerColumn : public QWidget {
  Q_OBJECT
public:
  /** Половина рокера — для подсветки от горячих клавиш. */
  enum class Half { None, Plus, Minus };

  explicit RockerColumn(const QString &label, QWidget *parent = nullptr);

  /** Подсветить половину рокера (см. RemoteScreen::flashButton). */
  void flash(Half half, int ms = 120);

signals:
  void plus();
  void minus();

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

private:
  QString m_label;
  Half m_pressed = Half::None;
  QTimer m_repeatTimer;

  bool m_flashOn = false;
  Half m_flashPart = Half::None;
  QTimer m_flashTimer;

  QRectF m_plusRect;
  QRectF m_minusRect;
  QRectF m_labelRect;

  void recompute();
  void fireOnce(Half);
};