#pragma once
#include <QAbstractButton>

class KeypadKey : public QAbstractButton {
  Q_OBJECT
public:
  /** digit >= 0 — цифра, digit < 0 — стрелка «назад» */
  explicit KeypadKey(int digit, QWidget *parent = nullptr);

  QSize sizeHint() const override { return {80, 64}; }

protected:
  void paintEvent(QPaintEvent *) override;

private:
  int m_digit;
};