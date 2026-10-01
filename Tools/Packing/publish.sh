#!/usr/bin/env bash
# Publish the game folder to the update server, one version up. Runs on the VPS.
#
#   ./publish.sh patch      1.0.4 -> 1.0.5    a fix, a changed file
#   ./publish.sh minor      1.0.4 -> 1.1.0    new content
#   ./publish.sh major      1.0.4 -> 2.0.0
#   ./publish.sh 1.2.3      exactly that version
#   ./publish.sh            show the published version and do nothing
#
# The number it counts up from is the one in the published manifest, so there
# is no version file to keep in step. The login server is carried over from the
# same manifest; name another with SERVER=ip:port ./publish.sh patch
#
# It refuses when the uploaded Archive/ would take back a patch players already
# have (usually someone else's change missing from it). To go back on purpose:
#   ALLOW_REVERT=1 ./publish.sh patch
set -euo pipefail

GAME="${GAME:-/root/Documents/Game}"
OUT="${OUT:-/var/www/darkeden}"
MANIFEST="$OUT/manifest.txt"

field() { [ -f "$MANIFEST" ] && awk -v key="$1" '$1 == key { print $2; exit }' "$MANIFEST" || true; }

current="$(field version)"
server="${SERVER:-$(field server)}"

if [ $# -eq 0 ]; then
    echo "published: ${current:-nothing yet}    login server: ${server:-none}"
    echo "usage: $0 major|minor|patch|<x.y.z>"
    exit 0
fi

# nothing published yet counts as 1.0.0, so the first "patch" gives 1.0.1
IFS=. read -r major minor patch <<< "${current:-1.0.0}"
major="${major:-1}"; minor="${minor:-0}"; patch="${patch:-0}"

case "$1" in
    major) major=$((major + 1)); minor=0; patch=0 ;;
    minor) minor=$((minor + 1)); patch=0 ;;
    patch) patch=$((patch + 1)) ;;
    *)
        if [[ ! "$1" =~ ^[0-9]+[.][0-9]+[.][0-9]+$ ]]; then
            echo "usage: $0 major|minor|patch|<x.y.z>" >&2
            exit 2
        fi
        IFS=. read -r major minor patch <<< "$1"
        ;;
esac
next="$major.$minor.$patch"

if [ -z "$server" ]; then
    echo "No login server: the manifest has none yet. Run it once as" >&2
    echo "  SERVER=<ip>:9909 $0 $1" >&2
    exit 2
fi

echo "publishing ${current:-nothing} -> $next   (login server $server)"

# nginx reads these as www-data; uploads tend to arrive owner-only
chmod -R a+rX "$GAME"
python3 "$GAME/make_manifest.py" "$GAME" "$OUT" --version "$next" --server "$server" \
    ${ALLOW_REVERT:+--allow-revert}

echo "published $next"
