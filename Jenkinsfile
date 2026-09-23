pipeline {
    agent any

    options {
        timestamps()
        ansiColor('xterm')
        buildDiscarder(logRotator(numToKeepStr: '30'))
        disableConcurrentBuilds()
    }

    environment {
        QT_PREFIX  = '/usr/lib/x86_64-linux-gnu/cmake'
        BUILD_TYPE = 'RelWithDebInfo'
    }

    triggers {
        // Опрос Git раз в 5 минут. Если у вас настроен webhook —
        // эту секцию можно удалить.
        pollSCM('H/5 * * * *')
    }

    stages {
        stage('Checkout') {
            // Собираем только main — всё остальное игнорируем.
            when { branch 'main' }
            steps {
                checkout scm
                sh 'git rev-parse --short HEAD > .git-sha'
            }
        }

        stage('Verify env') {
            when { branch 'main' }
            steps {
                sh '''
                    set -eu

                    for tool in cmake qmake6 file; do
                        if ! command -v "$tool" >/dev/null; then
                            echo "MISSING: $tool"
                            exit 1
                        fi
                    done

                    echo "cmake:   $(cmake --version | head -1)"
                    echo "qt6:     $(qmake6 -query QT_VERSION)"
                    echo "gcc:     $(g++ --version | head -1)"
                '''
            }
        }

        stage('Configure') {
            when { branch 'main' }
            steps {
                sh '''
                    set -eu
                    rm -rf build
                    cmake -B build -S . -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
                        -DCMAKE_PREFIX_PATH=${QT_PREFIX} \
                        -DQt6_DIR=${QT_PREFIX}/Qt6 \
                        -DQt6Svg_DIR=${QT_PREFIX}/Qt6Svg
                '''
            }
        }

        stage('Build') {
            when { branch 'main' }
            steps {
                sh '''
                    set -eu
                    cmake --build build -j$(nproc)

                    ls -la build/lazytv
                    file build/lazytv | tee /tmp/lazytv-type

                    if ! grep -q "dynamically linked" /tmp/lazytv-type; then
                        echo "Бинарник статический или повреждён — прерываем."
                        exit 1
                    fi

                    size=$(stat -c%s build/lazytv)
                    if [ "$size" -lt 100000 ]; then
                        echo "Подозрительно маленький бинарник: ${size} байт"
                        exit 1
                    fi
                '''
            }
        }

        stage('Smoke test') {
            when { branch 'main' }
            steps {
                sh '''
                    set -eu

                    # --version работает без дисплея — можно проверять в headless.
                    QT_QPA_PLATFORM=offscreen ./build/lazytv --version \
                        | tee /tmp/lazytv-version.txt

                    if ! grep -qE "^lazytv [0-9]+\\.[0-9]+\\.[0-9]+" /tmp/lazytv-version.txt; then
                        echo "Не удалось получить версию из --version"
                        exit 1
                    fi
                '''
            }
        }
    }

    post {
        success {
            when { branch 'main' }
            archiveArtifacts artifacts: 'build/lazytv',
                             fingerprint: true,
                             onlyIfSuccessful: true
        }
        cleanup {
            sh 'rm -rf build'
        }
    }
}