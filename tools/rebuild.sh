#!/usr/bin/env bash
# Refresh the artifacts on this branch: upstream v0.150.1 + ../source/skiko-wayland-egl.patch.
# Needs JDK 25, a C/C++ toolchain, pkg-config and the dbus/fontconfig/GL/EGL/X11 dev headers.
set -euo pipefail
VERSION="0.150.1"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT
git clone --depth 1 --branch v0.150.1 https://github.com/JetBrains/skiko "$WORK/skiko"
cd "$WORK/skiko" && git apply "$HERE/source/skiko-wayland-egl.patch"
./gradlew --no-daemon -Pskiko.native.linux.enabled=true -Pdeploy.release=true -Pdeploy.version="$VERSION" \
    :skiko:publishKotlinMultiplatformPublicationToMavenLocal :skiko:publishLinuxX64PublicationToMavenLocal
rm -rf "$HERE/org/jetbrains/skiko"; cp -r "$HOME/.m2/repository/org/jetbrains/skiko" "$HERE/org/jetbrains/"
# The published root metadata must list EVERY upstream variant: this repository owns the
# upstream coordinates, so a root module that only declares the variants hosted here shadows
# Maven Central and breaks all other targets (ios/js/wasm/awt/linuxArm64) with
# "no matching variant" - Gradle never falls back to the next repository for metadata.
# Non-hosted variants are re-emitted with absolute available-at URLs to Maven Central.
python3 "$HERE/tools/merge-upstream-variants.py" "$HERE"
echo "Refreshed - commit and push this branch."
