# LazyTV 1.1.0

**Первое издание под новым именем LazyTV.**

Приложение переименовано из `lgremote` в `lazytv`, чтобы избежать
использования торговой марки LG в названии. Функциональность
не изменилась.

---

## Что нового

### 🏷 Новое имя

- Пакет: `lgremote` → `lazytv`
- Приложение в меню: `LgRemote` → `LazyTV`
- Команда: `lgremote` → `lazytv`
- PPA: `ppa:mindwork64/lgremote` → `ppa:mindwork64/lazytv`
- Namespace: `lgremote::` → `lazytv::`
- CMake-цели: `LgRemote::Core` → `LazyTV::Core`, `LgRemote::Client` → `LazyTV::Client`
- Заголовки: `#include <lgremote/...>` → `#include <lazytv/...>`
- Конфиг: `~/.config/lgremote/` → `~/.config/lazytv/`

### 📦 Установка

**Ubuntu 24.04:**

```bash
sudo add-apt-repository ppa:mindwork64/lazytv
sudo apt update
sudo apt install lazytv
```

**Другие дистрибутивы:** скачайте `lazytv-1.1.0-x86_64.AppImage`
со страницы релиза.

### ⚠️ Важно при обновлении

- Старый пакет `lgremote` необходимо удалить: `sudo apt remove lgremote`
- Старый конфиг `~/.config/lgremote/` переносится вручную:

  ```bash
  mv ~/.config/lgremote ~/.config/lazytv
  ```

  Или, если не важна сохранённая сессия, просто подключитесь заново.

---

## Что осталось как было

- Весь функционал пульта без изменений
- Сопряжение, D-Pad, рокеры, цифровая клавиатура, настройки
- Все три способа установки: PPA, AppImage, из исходников
- Использование как библиотеки через `find_package(LazyTV)`

---

## Скачать

| Файл | Описание |
| --- | --- |
| `lazytv-1.1.0-x86_64.AppImage` | Самодостаточный бинарник (35 МБ) |
| `lazytv-1.1.0-x86_64.AppImage.sha256` | Контрольная сумма |

```bash
chmod +x lazytv-1.1.0-x86_64.AppImage
./lazytv-1.1.0-x86_64.AppImage
```

---

## Совместимость

- Linux x86_64 (Ubuntu 20.04+, Debian 11+, Fedora 36+)
- Qt 6.4+ для сборки из исходников
- Телевизоры LG с поддержкой NetCast (2010–2014)

Полное описание — в [README](https://github.com/mindwork64/lazytv#readme).
