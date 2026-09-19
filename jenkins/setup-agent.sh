#!/usr/bin/env bash
# Запускается один раз на Jenkins-агенте с sudo.
# Скачивает linuxdeploy-инструменты как AppImage и создаёт wrapper-скрипты.

set -euo pipefail

TOOLS_DIR="${TOOLS_DIR:-/opt/lgremote-build-tools}"

echo "=== apt packages ==="
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    wget \
    file \
    qt6-base-dev \
    qt6-svg-dev \
    libgl1-mesa-dev

echo "=== tools directory: $TOOLS_DIR ==="
sudo mkdir -p "$TOOLS_DIR"

fetch_appimage () {
    local url="$1"
    local target="$2"

    if [ -f "${TOOLS_DIR}/${target}.AppImage" ]; then
        echo "  -> ${target}.AppImage уже есть, пропускаем"
        return
    fi

    echo "  -> скачиваем ${target}.AppImage"
    sudo wget -q --show-progress \
        -O "${TOOLS_DIR}/${target}.AppImage" "$url"
    sudo chmod +x "${TOOLS_DIR}/${target}.AppImage"
}

fetch_appimage \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "linuxdeploy"

fetch_appimage \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "linuxdeploy-plugin-qt"

# Создаём wrapper-скрипты в /usr/local/bin, которые прозрачно
# запускают AppImage с APPIMAGE_EXTRACT_AND_RUN=1
echo "=== создаём wrapper-скрипты ==="

for tool in linuxdeploy linuxdeploy-plugin-qt; do
    wrapper="/usr/local/bin/${tool}"
    sudo tee "$wrapper" > /dev/null <<EOF
#!/bin/sh
exec env APPIMAGE_EXTRACT_AND_RUN=1 ${TOOLS_DIR}/${tool}.AppImage "\$@"
EOF
    sudo chmod +x "$wrapper"
    echo "  -> $wrapper"
done

echo
echo "=== Проверка ==="
which linuxdeploy
which linuxdeploy-plugin-qt
linuxdeploy --version
echo
echo "Готово."