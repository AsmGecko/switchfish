#!/usr/bin/env bash
# switchfish: one entry point for building, running and shipping.
set -euo pipefail
cd "$(dirname "$0")"

IMAGE=switchfish-dev
NRO=build/switch/switchfish.nro
RYUBING_VERSION=1.3.354
RYUBING=tools/ryubing/Ryujinx.app/Contents/MacOS/Ryujinx
EMU_SD=tools/ryubing/portable/sdcard/switch/switchfish

usage() {
	cat <<EOF
Usage: ./sf.sh <command>

  setup           build the Docker image with the Switch toolchain
  build [host]    build for this Mac/Linux box (build/host/switchfish)
  build switch    build the NRO in Docker ($NRO)
  run [args]      build and run the host build (needs gameData/ or --data)
  emu             build the NRO and start it in Ryubing (installs it in tools/ on first use)
  send <ip>       send the NRO to a Switch running hbmenu's netloader (Y)
  upstream        clone or update the reference repos in upstream/
  clean           remove build/
EOF
}

build_host() {
	cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
	cmake --build build/host
}

build_switch() {
	docker image inspect "$IMAGE" >/dev/null 2>&1 || setup
	docker run --rm -v "$PWD":/src -w /src "$IMAGE" bash -c '
		cmake -S . -B build/switch -G Ninja \
			-DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
			-DCMAKE_BUILD_TYPE='"${BUILD_TYPE:-Release}"' &&
		cmake --build build/switch'
	ls -la "$NRO"
}

setup() {
	docker build -t "$IMAGE" docker
}

install_ryubing() {
	[ -x "$RYUBING" ] && return
	echo "Installing Ryubing $RYUBING_VERSION in tools/ryubing/"
	mkdir -p tools/ryubing/portable
	curl -fsSL "https://git.ryujinx.app/Ryubing/Canary/releases/download/$RYUBING_VERSION/ryujinx-canary-$RYUBING_VERSION-macos_universal.app.tar.gz" |
		tar xz -C tools/ryubing
}

# The emulator's SD card is tools/ryubing/portable/sdcard; the game looks in sdmc:/switch/switchfish.
emu() {
	install_ryubing
	build_switch
	mkdir -p "$EMU_SD"
	if [ -f gameData/properties/resources.xml ]; then
		rsync -a gameData/ "$EMU_SD/"
	else
		echo "No game files in gameData/ yet: the NRO will start and log that it found no data."
	fi
	cp "$NRO" "$EMU_SD/"
	rm -f "$EMU_SD/log.txt"
	"$RYUBING" "$PWD/$EMU_SD/switchfish.nro" "$@" || true
	[ -f "$EMU_SD/log.txt" ] && { echo "--- $EMU_SD/log.txt"; cat "$EMU_SD/log.txt"; }
}

upstream() {
	mkdir -p upstream
	while read -r dir url branch; do
		[ -z "$dir" ] && continue
		if [ -d "upstream/$dir/.git" ]; then
			git -C "upstream/$dir" pull --ff-only -q
		else
			git clone -q -b "$branch" "$url" "upstream/$dir"
		fi
		echo "$dir: $(git -C "upstream/$dir" log -1 --format='%h %cs %s')"
	done < docs/upstream.txt
}

case "${1:-}" in
	setup) setup ;;
	build)
		case "${2:-host}" in
			host) build_host ;;
			switch) build_switch ;;
			*) usage; exit 1 ;;
		esac ;;
	run) shift; build_host; ./build/host/switchfish "$@" ;;
	send)
		[ -n "${2:-}" ] || { echo "Usage: ./sf.sh send <switch-ip>"; exit 1; }
		docker run --rm -v "$PWD":/src -w /src "$IMAGE" bash -c "\$DEVKITPRO/tools/bin/nxlink -a $2 $NRO" ;;
	emu) shift; emu "$@" ;;
	upstream) upstream ;;
	clean) rm -rf build ;;
	*) usage ;;
esac
