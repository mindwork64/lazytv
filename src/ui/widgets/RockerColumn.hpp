#pragma once
#include <QTimer>
#include <QWidget>

class RockerColumn : public QWidget {
  Q_OBJECT
public:
  explicit RockerColumn(const QString &label, QWidget *parent = nullptr);

signals:
  void plus();
  void minus();

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;

private:
  enum class Half { None, Plus, Minus };
  QString m_label;
  Half m_pressed = Half::None;
  QTimer m_repeatTimer;

  QRectF m_plusRect;
  QRectF m_minusRect;
  QRectF m_labelRect;

  void recompute();
  void fireOnce(Half);
};