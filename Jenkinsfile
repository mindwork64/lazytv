pipeline {
    agent any

    options {
        timestamps()
        ansiColor('xterm')
        buildDiscarder(logRotator(numToKeepStr: '20'))
        disableConcurrentBuilds()
    }

    environment {
        QT_PREFIX   = '/usr/lib/x86_64-linux-gnu/cmake'
        APP_VERSION = '1.0.0'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
                sh 'git rev-parse --short HEAD > .git-sha'
            }
        }

        stage('Verify env') {
            steps {
                sh '''
                    set -eu
                    for tool in cmake qmake6 file linuxdeploy linuxdeploy-plugin-qt; do
                        command -v $tool >/dev/null || {
                            echo "MISSING: $tool"
                            echo "Запустите jenkins/setup-agent.sh на агенте."
                            exit 1
                        }
                    done
                    echo "cmake:   $(cmake --version | head -1)"
                    echo "qt6:     $(qmake6 -query QT_VERSION)"
                    echo "gcc:     $(g++ --version | head -1)"
                '''
            }
        }

        stage('Configure') {
            steps {
                sh '''
                    set -eu
                    rm -rf build
                    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
                        -DCMAKE_PREFIX_PATH=${QT_PREFIX} \
                        -DQt6_DIR=${QT_PREFIX}/Qt6 \
                        -DQt6Svg_DIR=${QT_PREFIX}/Qt6Svg
                '''
            }
        }

        stage('Build') {
            steps {
                sh '''
                    set -eu
                    cmake --build build -j$(nproc)

                    ls -la build/lgremote
                    file build/lgremote | tee /tmp/lgremote-type

                    if ! grep -q "dynamically linked" /tmp/lgremote-type; then
                        echo "Бинарник статический или повреждён — прерываем."
                        exit 1
                    fi

                    size=$(stat -c%s build/lgremote)
                    if [ "$size" -lt 100000 ]; then
                        echo "Подозрительно маленький бинарник: ${size} байт"
                        exit 1
                    fi
                '''
            }
        }

        stage('Package AppImage') {
            steps {
                sh '''
                    set -eu

                    rm -rf AppDir
                    mkdir -p AppDir/usr/bin
                    cp build/lgremote AppDir/usr/bin/lgremote
                    chmod +x AppDir/usr/bin/lgremote

                    cat > lgremote.desktop <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=LgRemote
Comment=LG NetCast TV remote
Exec=lgremote
Icon=lgremote
Terminal=false
Categories=Utility;AudioVideo;
DESKTOP

                    cp resources/ic_launcher.svg lgremote.svg

                    export EXTRA_QT_MODULES="svg;"
                    export EXTRA_QT_PLUGINS="svg;"
                    export EXTRA_PLATFORM_PLUGINS="libqxcb.so;"

                    linuxdeploy \
                        --appdir AppDir \
                        --plugin qt \
                        --output appimage \
                        --desktop-file=lgremote.desktop \
                        --icon-file=lgremote.svg

                    mv -v LgRemote*.AppImage \
                          "lgremote-${APP_VERSION}-x86_64.AppImage"

                    sha256sum "lgremote-${APP_VERSION}-x86_64.AppImage" \
                        > "lgremote-${APP_VERSION}-x86_64.AppImage.sha256"

                    ls -la *.AppImage *.sha256
                '''
            }
        }

        stage('Smoke test') {
            steps {
                sh '''
                    set -eu
                    ./lgremote-${APP_VERSION}-x86_64.AppImage --appimage-extract > /dev/null
                    test -f squashfs-root/AppRun -o -L squashfs-root/AppRun
                    file squashfs-root/usr/bin/lgremote
                    readelf -d squashfs-root/usr/bin/lgremote | grep -E "RPATH|RUNPATH" || true
                    rm -rf squashfs-root
                '''
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: 'lgremote-*.AppImage,lgremote-*.sha256',
                             fingerprint: true,
                             onlyIfSuccessful: true
        }
        cleanup {
            sh 'rm -rf build AppDir lgremote.desktop lgremote.svg'
        }
    }
}