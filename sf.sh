#!/usr/bin/env bash
# switchfish: one entry point for building, running and shipping.
set -euo pipefail
cd "$(dirname "$0")"

IMAGE=switchfish-dev
NRO=build/switch/switchfish.nro
RYUBING_VERSION=1.3.354
RYUBING=tools/ryubing/Ryujinx.app/Contents/MacOS/Ryujinx
EMU_SD=tools/ryubing/portable/sdcard/switch/switchfish
BREW_DEPS="cmake ninja pkgconf sdl2 sdl2_ttf libopenmpt jpeg-turbo libpng libvorbis mpg123"

usage() {
	cat <<EOF
Usage: ./sf.sh <command>

  setup           once after cloning: Docker image, Homebrew packages, Ryubing
  build [host]    build for this Mac (build/host/switchfish)
  build switch    build the NRO in Docker ($NRO)
  run [args]      build and run on the Mac with the files in gameData/ (or --data <dir>)
  emu [--gui]     build the NRO and play it in Ryubing; click = touch.
                  --gui opens Ryubing's own window, which needs prod.keys from your Switch
  widen           regenerate the widescreen tank sides in port/romfs/widescreen/ from your
                  backdrops with Codex (an image model; costs API credit). Only the new parts are
                  stored; check the result with git diff, undo with git checkout.
  send <ip> [args] build the NRO and send it over wifi to hbmenu's netloader (press Y on the
                  Switch): it lands in sdmc:/switch/switchfish/, starts with [args] (--level 3-2)
                  and its log shows here until Ctrl-C
  upstream        clone or update the reference repos in upstream/
  clean           remove build/

BUILD_TYPE=Debug ./sf.sh build ... picks the CMake build type.
EOF
}

need_docker() {
	if docker info >/dev/null 2>&1; then
		return
	fi
	if [ "$(uname)" = Darwin ] && [ -d /Applications/Docker.app ]; then
		echo "Starting Docker Desktop..."
		open -a Docker
		for _ in $(seq 60); do
			docker info >/dev/null 2>&1 && return
			sleep 2
		done
	fi
	echo "Docker isn't running; start it and try again."
	exit 1
}

need_image() {
	need_docker
	docker image inspect "$IMAGE" >/dev/null 2>&1 || docker build -t "$IMAGE" docker
}

build_host() {
	cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
	cmake --build build/host
	ln -sf build/host/compile_commands.json compile_commands.json
}

need_pyenv() {
	[ -x tools/pyenv/bin/python ] || { python3 -m venv tools/pyenv && tools/pyenv/bin/pip -q install numpy pillow; }
}

build_switch() {
	need_image
	docker run --rm -v "$PWD":/src -w /src -e BUILD_TYPE="${BUILD_TYPE:-Release}" "$IMAGE" bash -c '
		cmake -S . -B build/switch -G Ninja -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
			-DCMAKE_BUILD_TYPE=$BUILD_TYPE &&
		cmake --build build/switch'
	ls -la "$NRO"
}

install_ryubing() {
	[ -x "$RYUBING" ] && return
	echo "Installing Ryubing $RYUBING_VERSION in tools/ryubing/"
	# portable/ next to the app keeps its config and SD card in here instead of ~/Library
	mkdir -p tools/ryubing/portable
	curl -fsSL "https://git.ryujinx.app/Ryubing/Canary/releases/download/$RYUBING_VERSION/ryujinx-canary-$RYUBING_VERSION-macos_universal.app.tar.gz" |
		tar xz -C tools/ryubing
}

setup() {
	need_docker
	docker build -t "$IMAGE" docker
	if [ "$(uname)" = Darwin ]; then
		brew install $BREW_DEPS
		install_ryubing
	fi
}

# The emulator's SD card is tools/ryubing/portable/sdcard; the game looks in sdmc:/switch/switchfish.
emu() {
	local gui=false
	if [ "${1:-}" = --gui ]; then
		gui=true
		shift
	fi
	install_ryubing
	build_switch
	mkdir -p "$EMU_SD"
	if [ -f gameData/properties/resources.xml ]; then
		rsync -a gameData/ "$EMU_SD/"
	else
		echo "No game files in gameData/: the game will say where to put them."
	fi
	cp "$NRO" "$EMU_SD/"
	rm -f "$EMU_SD/log.txt"
	touch "$EMU_SD/debug.txt"	# the game only writes log.txt with this
	# Without --no-gui Ryubing won't load anything until it has prod.keys; homebrew doesn't need them.
	if $gui; then
		"$RYUBING" "$@" "$PWD/$EMU_SD/switchfish.nro" || true
	else
		"$RYUBING" --no-gui "$@" "$PWD/$EMU_SD/switchfish.nro" || true
	fi
	if [ -f "$EMU_SD/log.txt" ]; then
		echo "--- $EMU_SD/log.txt"
		cat "$EMU_SD/log.txt"
	fi
}

# Over wifi to hbmenu's netloader: the NRO lands in sdmc:/switch/switchfish/ (so hbmenu has it afterwards)
# and starts with the given arguments; its log shows here until Ctrl-C. The Switch connects back to
# nxlink's server on port 28771, hence the published port.
send() {
	[ -n "${1:-}" ] || { echo "Usage: ./sf.sh send <switch-ip> [game arguments, e.g. --level 3-2]"; exit 1; }
	local ip=$1 tty=
	shift
	build_switch
	[ -t 1 ] && tty=-it
	docker run --rm $tty -p 28771:28771 -v "$PWD":/src -w /src -e SWITCH_IP="$ip" -e GAME_ARGS="$*" "$IMAGE" bash -c '
		$DEVKITPRO/tools/bin/nxlink -a "$SWITCH_IP" -p switchfish/switchfish.nro -s ${GAME_ARGS:+--args "$GAME_ARGS"} '"$NRO"
}

# Runs a command, killing it (and its children) after $1 seconds.
with_timeout() {
	local secs=$1 pid killer rc
	shift
	"$@" &
	pid=$!
	(sleep "$secs"; pkill -P "$pid"; kill "$pid") 2>/dev/null &
	killer=$!
	wait "$pid"
	rc=$?
	kill "$killer" 2>/dev/null
	return $rc
}

# Each backdrop's middle goes to Codex with its sides transparent, plus the whole original as a reference
# for the glass walls; the game keeps the original middle pixel exact and only uses the new parts, so the pack
# stores only those (fit.py) and they can live in the repo.
widen() {
	[ -f gameData/properties/resources.xml ] || { echo "Put your game files in gameData/ first."; exit 1; }
	need_pyenv
	local work py=tools/pyenv/bin/python
	work=$(mktemp -d)
	for i in 1 2 3 4 5 6; do
		src=$(find gameData/images -maxdepth 1 -iname "aquarium$i.jpg" | head -1)
		"$py" -I scripts/widescreen/canvas.py "$src" "$work/canvas$i.png"
		# Codex sometimes hangs trying to edit files in its read-only sandbox
		with_timeout 300 ~/.claude/skills/codex-consult/codex-run.sh exec "$(cat scripts/widescreen/prompt.txt)" -i "$work/canvas$i.png" "$src" > "$work/codex$i.txt" &
	done
	wait
	for i in 1 2 3 4 5 6; do
		src=$(find gameData/images -maxdepth 1 -iname "aquarium$i.jpg" | head -1)
		draft=$(grep -oE '/[^]`( )]+/generated_images/[^]`( )]+\.png' "$work/codex$i.txt" | head -1 || true)
		if [ -z "$draft" ]; then
			# Codex sometimes answers without an image; one more try
			with_timeout 300 ~/.claude/skills/codex-consult/codex-run.sh exec "$(cat scripts/widescreen/prompt.txt)" -i "$work/canvas$i.png" "$src" > "$work/codex$i.txt" || true
			draft=$(grep -oE '/[^]`( )]+/generated_images/[^]`( )]+\.png' "$work/codex$i.txt" | head -1 || true)
		fi
		if [ -z "$draft" ]; then
			echo "aquarium$i: no image from Codex, see $work/codex$i.txt"
			continue
		fi
		"$py" -I scripts/widescreen/fit.py "$draft" "$src" "port/romfs/widescreen/aquarium$i.png"
	done
	# the main menu and the title screen: no glass walls, the whole original is the middle
	for screen in selectorback:menu titlescreen:title; do
		name=${screen%%:*} prompt=scripts/widescreen/prompt-${screen##*:}.txt
		src=$(find gameData/images -maxdepth 1 -iname "$name.jpg" | head -1)
		[ -n "$src" ] || { echo "$name: not in gameData/images"; continue; }
		"$py" -I scripts/widescreen/canvas.py "$src" "$work/$name.png" --full
		draft=
		for try in 1 2; do
			with_timeout 300 ~/.claude/skills/codex-consult/codex-run.sh exec "$(cat "$prompt")" -i "$work/$name.png" "$src" > "$work/codex-$name.txt" || true
			draft=$(grep -oE '/[^]`( )]+/generated_images/[^]`( )]+\.png' "$work/codex-$name.txt" | head -1 || true)
			[ -n "$draft" ] && break
		done
		if [ -n "$draft" ]; then
			"$py" -I scripts/widescreen/fit.py "$draft" "$src" "port/romfs/widescreen/$name.png" --full
		else
			echo "$name: no image from Codex, see $work/codex-$name.txt"
		fi
	done
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
	emu) shift; emu "$@" ;;
	widen) widen ;;
	send) shift; send "$@" ;;
	upstream) upstream ;;
	clean) rm -rf build ;;
	*) usage ;;
esac
