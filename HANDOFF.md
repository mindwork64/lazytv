# HANDOFF — LazyTV

**Дата:** 2026-10-30
**Ветка:** `main` = `f94ae17`, синхронизирована с `origin/main`, дерево чистое.
**Версия:** 1.1.3. Тег `v1.1.3` — на коммите `948e139` (HEAD на 1 коммит
впереди: `docs: add agent working rules`).

## Быстрый старт

1. [AGENTS.md](AGENTS.md) — правила работы агентов (tracked).
2. [apidocs/PROJECT.md](apidocs/PROJECT.md) — инфраструктура, сборка, релиз,
   правила; [apidocs/ARCHITECTURE.md](apidocs/ARCHITECTURE.md),
   [apidocs/ADR.md](apidocs/ADR.md) — архитектура и решения.
   **Важно:** `apidocs/` в `.gitignore` и не отслеживается git.
3. Код — по необходимости и фрагментами: `include/lazytv/`, `src/net/`,
   `src/data/`, `src/ui/`.

## Проект

LazyTV — desktop-пульт для LG NetCast (2010–2014). C++20, Qt 6 Widgets
(`Core`, `Gui`, `Widgets`, `Network`, `Svg`), CMake 3.21+, MIT.
Три цели: `lazytv-core` (протокол), `lazytv` (+ `SessionStore`,
`AppContainer`), `lazytv-app` (GUI, OUTPUT_NAME `lazytv`).
Публичный API — `namespace lazytv`, заголовки в `include/lazytv/`.

Ключевое: `Client` (QTcpSocket, HTTP/1.1, сигналы `pairingKeyResult`,
`pairingConfirmResult`, `commandResult`, `sessionExpired`); `SessionStore`
(`ip`, `session`, `pairingKey`, `themeMode`, `hotkeysEnabled`);
`AppContainer::reauth()` — тихое восстановление сессии по ключу
сопряжения; `HotkeyHandler` — клавиатура как кнопки ТВ.

## Правила работы (сжато)

- Веб-чат DeepSeek — полные файлы; **harness — только точечные правки**,
  целые файлы не выводить.
- Большие файлы читать фрагментами (`grep`, `sed -n`, offset/limit).
- Минимум итераций: сразу финальный вариант с кратким пояснением.
- Не менять имена классов/targets и архитектуру ради локальной задачи;
  новые зависимости — по согласованию.
- Стиль новых правок: 4 пробела, `m_` для полей, `kCamelCase` для констант,
  `#pragma once`, комментарии/UI по-русски. В существующих 2-space файлах
  сохранять локальный стиль.
- Коммиты: Conventional Commits (`feat:`, `fix:`, `docs:`, `chore:`),
  релиз — `Release X.Y.Z`.

## Окружение

- cmake 3.28.3, g++ 13.3.0, Qt 6.4.2, make (Unix Makefiles).
- `build/` настроена в `RelWithDebInfo`; dry-run не показывает компиляции,
  бинарник отдаёт `lazytv 1.1.3`.
- Автотестов нет (`ctest` — 0), линтеров и `-Wall/-Wextra` нет,
  `.clang-format`/`.editorconfig` отсутствуют.

## Сделано в предыдущей сессии

- Проведён обзор проекта и требований к коду.
- Добавлен `AGENTS.md` с правилами агентов, закоммичен и запушен
  (`f94ae17`).
- В `apidocs/PROJECT.md` обновлён раздел «Правила работы с ассистентом»
  (разделение веб-чат/harness, чтение фрагментами, минимум итераций) —
  локально, в git не попадает.

## Известные расхождения и долги

- **Отступы смешаны:** 2 пробела — `MainWindow`, `Theme`, `SettingsScreen`,
  виджеты, `session_store.hpp`, `app_container.hpp`, `AppContainer.cpp`,
  `SessionStore.cpp`; 4 пробела — `client.hpp`, `NetCastClient.cpp`,
  `RemoteScreen`, `PairingScreen`, `HotkeyHandler`, `main.cpp`.
- Указатели оформлены и `T*`, и `T *`.
- **Документация рассинхронизирована:** в `PROJECT.md` остались `1.1.2`
  (пример `--version` и раздел релиза), пример конфига без `pairingKey`
  и `hotkeysEnabled`; скриншоты `docs/screenshot-*.png` отсутствуют.
- В README ссылка на AppImage 1.1.3, локально лежит `1.1.2`.
- `apidocs/` игнорируется git: новые файлы там требуют `git add -f`.

## Возможные следующие шаги

1. Синхронизировать `PROJECT.md` с 1.1.3 и фактическим конфигом.
2. Решить вопрос стиля: либо `.clang-format` + отдельное переформатирование,
   либо закрепить правило «новый код — 4 пробела, старое не трогать».
3. Добавить тесты и `-Wall/-Wextra` (сейчас проверка только сборкой).
4. AppStream-метаданные, скриншоты, актуализация release-артефактов.

## Команды

```bash
# сборка
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake \
    -DQt6_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6 \
    -DQt6Svg_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6Svg
cmake --build build -j$(nproc)

# smoke-тест
QT_QPA_PLATFORM=offscreen ./build/lazytv --version

# состояние
git status --short --branch
git log -1 --stat
```

## Стартовый промпт для нового чата

> Продолжаем проект LazyTV. Сначала прочитай `HANDOFF.md` и `AGENTS.md`,
> затем при необходимости `apidocs/PROJECT.md` (фрагментами). Работаем из
> harness: только точечные правки, большие файлы читаем фрагментами,
> минимум итераций. Перед изменениями проверь `git status` и сборку.
> Текущая задача: <вписать>.
