#!/usr/bin/env bash
set -euo pipefail

header=$1
executable=$2
license=$3
udev_rule=$4
output=$5

stage=$(mktemp -d)
temporary=
trap 'rm -rf -- "$stage"; if [[ -n "$temporary" ]]; then rm -f -- "$temporary"; fi' EXIT
temporary=$(mktemp -- "${output}.XXXXXX")

install -m 0755 -- "$executable" "$stage/daz-shifter-desktop"
install -m 0644 -- "$license" "$stage/LICENSE"
install -m 0644 -- "$udev_rule" "$stage/70-daz-shifter.rules"

{
  cat -- "$header"
  tar -C "$stage" -czf - daz-shifter-desktop LICENSE 70-daz-shifter.rules
} >"$temporary"
chmod 0755 -- "$temporary"
mv -fT -- "$temporary" "$output"
