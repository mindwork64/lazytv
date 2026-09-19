# LG Лентяйка 1.1.0

**Первый релиз с публичным API.** Ядро проекта теперь оформлено как
переиспользуемая C++/Qt6-библиотека, а само приложение собирается
поверх неё.

---

## Что нового

### 📚 Библиотека для разработчиков

Протокол LG NetCast вынесен в отдельную библиотеку, которую можно
использовать в своих приложениях:

```cmake
find_package(LgRemote REQUIRED)
target_link_libraries(my_app PRIVATE LgRemote::Core)
```

```cpp
#include <lgremote/client.hpp>

lgremote::Client client("192.168.1.42");
client.requestPairingKey();
client.sendCommand(lgremote::Client::Command::Power);
```

Два уровня API:

- **`LgRemote::Core`** — только протокол (класс `lgremote::Client`)
- **`LgRemote::Client`** — то же + JSON-хранилище сессии и управление
  жизненным циклом клиента

Пример использования: [`examples/minimal_client/`](examples/minimal_client/).

### 🏗 Установка библиотеки

```bash
cmake --install build --prefix /usr/local
```

Устанавливаются заголовки, статические библиотеки и CMake-конфиги,
так что `find_package(LgRemote)` работает из коробки.

### 🔧 Внутренние изменения

- Публичный API в `namespace lgremote`
- Заголовки переехали в `include/lgremote/`
- Приватные `.hpp` удалены из `src/net/` и `src/data/`
- CMake: три цели — `lgremote-core`, `lgremote`, `lgremote-app`
- Экспорт имён: `LgRemote::Core`, `LgRemote::Client`

---

## Что осталось как было

- Все функции приложения не изменились
- Сопряжение, D-Pad, рокеры, цифровая клавиатура, настройки — как в 1.0.0
- AppImage собирается тем же способом
- Jenkins-пайплайн работает без изменений

---

## Скачать

| Файл | Описание |
|---|---|
| `lgremote-1.1.0-x86_64.AppImage` | Самодостаточный бинарник (35 МБ) |
| `lgremote-1.1.0-x86_64.AppImage.sha256` | Контрольная сумма |

```bash
chmod +x lgremote-1.1.0-x86_64.AppImage
./lgremote-1.1.0-x86_64.AppImage
```

---

## Обновление с 1.0.0

Просто запустите новый AppImage. Файл конфигурации
`~/.config/lgremote/config.json` совместим — сессия сохранится,
код сопряжения вводить не нужно.

---

## Совместимость

- Linux x86_64 (Ubuntu 20.04+, Debian 11+, Fedora 36+)
- Qt 6.4+ для сборки из исходников
- Телевизоры LG с поддержкой NetCast (2010–2014)

Полное описание — в [README](https://github.com/mindwork64/lgremote-qt#readme).