#pragma once
#include <QTimer>
#include <QWidget>

class DPad : public QWidget {
  Q_OBJECT
public:
  /** Часть крестовины — для подсветки от горячих клавиш. */
  enum class Part { Up, Down, Left, Right, Center };

  explicit DPad(QWidget *parent = nullptr);

  /** Подсветить часть крестовины (см. RemoteScreen::flashButton). */
  void flash(Part part, int ms = 120);

signals:
  void up();
  void down();
  void left();
  void right();
  void ok();

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

private:
  QRectF m_center;
  QRectF m_up, m_down, m_left, m_right;
  QRectF *m_pressed = nullptr;

  bool m_flashOn = false;
  Part m_flashPart = Part::Center;
  QTimer m_flashTimer;

  void recompute();
};