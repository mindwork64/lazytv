#include "ui/HotkeyHandler.hpp"

#include <QKeyEvent>

namespace {

using Cmd = lazytv::Client::Command;

/** Модификаторы, при которых клавиши не перехватываются. */
constexpr Qt::KeyboardModifiers kPassthroughModifiers =
    Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

} // namespace

HotkeyHandler::HotkeyHandler(QObject* parent) : QObject(parent) {}

std::optional<HotkeyHandler::Cmd> HotkeyHandler::mapKey(const QKeyEvent* event) {
    const int key = event->key();

    // Цифры: основной ряд и NumPad (key() один и тот же).
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return lazytv::Client::digit(key - Qt::Key_0);

    switch (key) {
        case Qt::Key_Up:    return Cmd::Up;
        case Qt::Key_Down:  return Cmd::Down;
        case Qt::Key_Left:  return Cmd::Left;
        case Qt::Key_Right: return Cmd::Right;

        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space: return Cmd::Ok;

        case Qt::Key_Escape:    return Cmd::Back;
        case Qt::Key_Backspace: return Cmd::Exit;

        case Qt::Key_P: return Cmd::Power;
        case Qt::Key_M: return Cmd::MuteToggle;
        case Qt::Key_H: return Cmd::HomeMenu;
        case Qt::Key_I: return Cmd::Info;

        // «+» приходит как Key_Plus (Shift+= или NumPad), «=» — как Key_Equal.
        case Qt::Key_Plus:
        case Qt::Key_Equal:  return Cmd::VolumeUp;
        case Qt::Key_Minus:  return Cmd::VolumeDown;

        case Qt::Key_PageUp:   return Cmd::ChannelUp;
        case Qt::Key_PageDown: return Cmd::ChannelDown;

        default: return std::nullopt;
    }
}

bool HotkeyHandler::isRepeatable(Cmd cmd) {
    switch (cmd) {
        case Cmd::Up:
        case Cmd::Down:
        case Cmd::Left:
        case Cmd::Right:
        case Cmd::VolumeUp:
        case Cmd::VolumeDown:
        case Cmd::ChannelUp:
        case Cmd::ChannelDown:
            return true;
        default:
            return false;
    }
}

bool HotkeyHandler::handle(const QKeyEvent* event) {
    if (!m_enabled)
        return false;
    if (event->modifiers() & kPassthroughModifiers)
        return false;

    const int key = event->key();
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        if (event->isAutoRepeat())
            return true;
        emit switchPage();
        return true;
    }

    const auto cmd = mapKey(event);
    if (!cmd)
        return false;

    if (event->isAutoRepeat()) {
        // Power, Mute, Home, Exit, Info, цифры — строго по одной команде.
        if (!isRepeatable(*cmd))
            return true;
        // Удержание стрелок и рокеров: не чаще, чем клиент успевает
        // отправить (иначе его очередь растёт и команды «догоняют» ТВ
        // уже после отпускания клавиши).
        if (m_repeatThrottle.isValid() &&
            m_repeatThrottle.elapsed() < lazytv::Client::kMinCommandIntervalMs) {
            return true;
        }
    }

    m_repeatThrottle.restart();
    emit commandRequested(*cmd);
    return true;
}
