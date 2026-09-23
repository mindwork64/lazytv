#pragma once

#include <QWidget>

namespace lazytv { class AppContainer; class Client; }

class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;
class QTimer;

class PairingScreen : public QWidget {
    Q_OBJECT
public:
    explicit PairingScreen(lazytv::AppContainer* container, QWidget* parent = nullptr);

signals:
    void connected();

private:
    void requestKey();
    void confirm();
    void setLoading(bool v);
    void setInfo(const QString&);
    void setError(const QString&);

    lazytv::AppContainer* m_container;
    QLineEdit*   m_ip;
    QLineEdit*   m_key;
    QPushButton* m_reqBtn;
    QPushButton* m_confirmBtn;
    QProgressBar* m_progress;
    QLabel* m_infoLabel;
    QLabel* m_errorLabel;
    lazytv::Client* m_activeClient = nullptr;

    /** Блокирует «Запросить код» на 60 секунд после успешного запроса. */
    QTimer* m_reqCooldown = nullptr;
};