#pragma once

#include <QElapsedTimer>
#include <QObject>

#include <lazytv/client.hpp>

#include <optional>

class QKeyEvent;

/**
 * Разбор клавиатурных событий пульта в команды LG NetCast.
 *
 * Команды не отправляет: сообщает о них сигналом commandRequested(),
 * чтобы вызывающая сторона (RemoteScreen) использовала общий путь
 * sendCommand() с его обработкой ошибок и индикацией состояния.
 *
 * Клавиши с модификаторами Ctrl/Alt/Meta не перехватываются — они
 * остаются системе. Shift и цифровой блок клавиатуры не мешают разбору.
 */
class HotkeyHandler : public QObject {
    Q_OBJECT
public:
    using Cmd = lazytv::Client::Command;

    explicit HotkeyHandler(QObject* parent = nullptr);

    /** Горячие клавиши включены (настройка «Горячие клавиши»). */
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool v) { m_enabled = v; }

    /**
     * Обрабатывает нажатие. true — клавиша распознана и поглощена
     * (событие нужно принять); false — передать дальше по цепочке.
     */
    bool handle(const QKeyEvent* event);

signals:
    /** Распознана клавиша-команда. */
    void commandRequested(lazytv::Client::Command cmd);
    /** Tab — переключение страниц пульта (MAIN ↔ NUMBERS). */
    void switchPage();

private:
    /** Команда для клавиши или std::nullopt, если клавиша не наша. */
    static std::optional<Cmd> mapKey(const QKeyEvent* event);
    /** Разрешён ли автоповтор для команды. */
    static bool isRepeatable(Cmd cmd);

    bool m_enabled = true;
    QElapsedTimer m_repeatThrottle;
};
