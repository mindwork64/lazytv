#!/usr/bin/env bash
#
# LazyTV: сборка source-пакета и загрузка в PPA ppa:mindwork64/lazytv.
#
# Повышение версии скрипт НЕ делает — это осознанное ручное действие.
# К моменту запуска в репозитории уже должны быть: новая версия в
# CMakeLists.txt, новая запись в debian/changelog, тег vX.Y.Z на HEAD
# и чистое рабочее дерево.
#
# Использование:
#   scripts/release-ppa.sh 1.1.1
#
# Для проверки самого скрипта каталоги можно переопределить:
#   LAZYTV_SRC_DIR=/tmp/sbx/apps/lazytv LAZYTV_APPS_DIR=/tmp/sbx/apps
#
set -euo pipefail

VERSION="${1:-}"
GPG_KEY="8A0110A813660BBB16A0D243128E73EB1731C57C"
PPA="lazytv"
SRC_DIR="${LAZYTV_SRC_DIR:-$HOME/apps/lazytv}"
APPS_DIR="${LAZYTV_APPS_DIR:-$HOME/apps}"
WORK_DIR="$APPS_DIR/lazytv-${VERSION}"

die() { echo "ОШИБКА: $*" >&2; exit 1; }
step() { echo; echo "==> $*"; }

[[ -n "$VERSION" ]] || die "укажите версию: scripts/release-ppa.sh 1.1.1"
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] \
    || die "версия должна быть в формате X.Y.Z (получено: '$VERSION')"

cd "$SRC_DIR"
[[ -d .git ]] || die "$SRC_DIR не является git-репозиторием"

step "1/8 Проверка синхронизации версии"
grep -q "project(lazytv VERSION ${VERSION} " CMakeLists.txt \
    || die "CMakeLists.txt не содержит 'project(lazytv VERSION ${VERSION} '"
CHANGELOG_VERSION="$(dpkg-parsechangelog -SVersion)"
[[ "$CHANGELOG_VERSION" == "${VERSION}-1" ]] \
    || die "debian/changelog: версия ${CHANGELOG_VERSION}, ожидалась ${VERSION}-1"
echo "    CMakeLists.txt : ${VERSION}"
echo "    changelog      : ${CHANGELOG_VERSION}"

step "2/8 Проверка рабочего дерева и тега"
if [[ -n "$(git status --porcelain)" ]]; then
    git status --short
    die "незакоммиченные правки: git archive взял бы старое содержимое,
    а debian/changelog — новое, версии в .dsc и orig.tar.gz разошлись бы"
fi
git rev-parse -q --verify "refs/tags/v${VERSION}" >/dev/null \
    || die "нет тега v${VERSION}: сначала
    git tag -a v${VERSION} -m 'LazyTV ${VERSION}' && git push origin v${VERSION}"
HEAD_SHA="$(git rev-parse HEAD)"
TAG_SHA="$(git rev-parse "refs/tags/v${VERSION}^{commit}")"
[[ "$HEAD_SHA" == "$TAG_SHA" ]] \
    || die "тег v${VERSION} указывает не на HEAD (${TAG_SHA::10} != ${HEAD_SHA::10})"
echo "    дерево чистое, тег v${VERSION} = HEAD (${HEAD_SHA::10})"

step "3/8 Проверка файлов релиза"
for f in debian/changelog debian/control debian/rules; do
    if [[ -n "$(tail -c1 "$f")" ]]; then
        die "нет перевода строки в конце $f
    починить: printf '\\n' >> $f"
    fi
done
NOTES="docs/release-notes-${VERSION}.md"
[[ -f "$NOTES" ]] || die "нет $NOTES (нужен для gh release create)"
echo "    debian/{changelog,control,rules} — newline в конце, $NOTES на месте"

step "4/8 Рабочая копия из тега"
rm -rf "$WORK_DIR"
rm -f "$APPS_DIR/lazytv_${VERSION}"*
mkdir -p "$WORK_DIR"
git archive --format=tar --prefix="lazytv-${VERSION}/" HEAD | tar -x -C "$APPS_DIR/"
grep -q "project(lazytv VERSION ${VERSION} " "$WORK_DIR/CMakeLists.txt" \
    || die "в извлечённом CMakeLists.txt нет версии ${VERSION}"
echo "    $WORK_DIR"

step "5/8 orig.tar.gz (без debian/)"
cd "$APPS_DIR"
tar -czf "lazytv_${VERSION}.orig.tar.gz" \
    --exclude="lazytv-${VERSION}/debian" \
    "lazytv-${VERSION}/"
[[ "$(tar -tzf "lazytv_${VERSION}.orig.tar.gz" | grep -c "^lazytv-${VERSION}/debian/")" == "0" ]] \
    || die "orig.tar.gz содержит debian/"
tar -tzf "lazytv_${VERSION}.orig.tar.gz" | grep -E 'lazytv\.desktop|ic_launcher' >/dev/null \
    || die "в orig.tar.gz нет lazytv.desktop / resources/ic_launcher.svg"
ls -lh "lazytv_${VERSION}.orig.tar.gz"

step "6/8 debuild -S -sa (спросит пароль GPG)"
cd "$WORK_DIR"
chmod +x debian/rules
rm -f debian/files
debuild -S -sa -k"$GPG_KEY"

step "7/8 lintian и сверка .dsc"
cd "$APPS_DIR"
lintian --fail-on error "lazytv_${VERSION}-1_source.changes" \
    || die "lintian нашёл ошибки (E:) — исправлять до dput"
grep -q "lazytv_${VERSION}.orig.tar.gz" "lazytv_${VERSION}-1.dsc" \
    || die "в .dsc нет контрольной суммы orig.tar.gz"
ls -la "lazytv_${VERSION}-1"*

step "8/8 dput"
dput "$PPA" "lazytv_${VERSION}-1_source.changes"

echo
echo "Загружено. Через 2-5 минут придёт Accepted (статус Pending),"
echo "затем Building и Successfully built — обычно 10-30 минут."
echo "Следить: https://launchpad.net/~mindwork64/+archive/ubuntu/lazytv"
echo
echo "Дальше: собрать AppImage в Jenkins job LazyTV-Release"
echo "(http://localhost:8080/job/LazyTV-Release/, параметры VERSION=${VERSION},"
echo "GIT_REF=v${VERSION}) и приложить к GitHub Release:"
echo "  gh release create v${VERSION} --title 'LazyTV ${VERSION}' \\"
echo "      --notes-file ${NOTES} \\"
echo "      lazytv-${VERSION}-x86_64.AppImage lazytv-${VERSION}-x86_64.AppImage.sha256"
