#!/usr/bin/env bash

error() {
	echo "$@"
	exit 1
}

if [[ -z $1 ]]; then
	error "Error: nothing to copy"
fi

if [[ -n $WAYLAND_DISLPLAY ]]; then
	command=(wl-copy)
	if [[ $X_SELECTION == primary ]]; then
		command+=( --primary )
	fi
elif [[ -n $DISPLAY ]]; then
	command=(xclip -selection clipboard)
else
	error "Error: X11 or Wayland display were not detected"
fi

echo "$1" | "${command[@]}" || error "Error: failed to copy data to clipboard"
echo "Password copied to clipboard."