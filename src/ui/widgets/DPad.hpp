#pragma once
#include <QWidget>

class DPad : public QWidget {
  Q_OBJECT
public:
  explicit DPad(QWidget *parent = nullptr);

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

  void recompute();
};