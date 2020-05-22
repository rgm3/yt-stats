#!/usr/bin/env bash

# Given an image of the correct size, quantize down to 3-color black-white-red
# and output two 1-bit bitmaps of the dithered red and black channels.

set -euo pipefail
IFS=$'\n\t'

# This file approximates the colors you'd see when looking at a BWR ePaper display.
# https://learn.adafruit.com/preparing-graphics-for-e-ink-displays?view=all
readonly PALETTE_FILE="eink-3color.png"
readonly WIDTH=296
readonly HEIGHT=128

function main {
  local fn
  fn="${1:-}"

  [[ -z $fn ]] && { >&2 echo "Usage: $0 image${WIDTH}x${HEIGHT}.{png,bmp,jpg,...}"; exit 1; }
  [[ -f $fn ]] || { >&2 echo "File not found $fn"; exit 1; }


  local size
  size=$(identify -format "%wx%h" "$fn")
  [[ $size != "${WIDTH}x${HEIGHT}" ]] && { >&2 echo "Warning: expected ${WIDTH}x${HEIGHT} input image"; }

  local shortfn="${fn%%.*}"
  quantize_dither "$fn" "85%"  # > input_dithered.bmp


  # For gotchas like "Can't BMP3 when PNG has gama" see https://www.imagemagick.org/Usage/formats/#bmp
  convert "${shortfn}"_dithered.bmp -depth 1 -colors 3 -fill white \
    +opaque black -monochrome BMP3:"${shortfn}"_black.bmp

  convert "${shortfn}"_dithered.bmp -depth 1 -colors 3 -fill white \
    +opaque red   -monochrome BMP3:"${shortfn}"_red.bmp

  write_h_file "${shortfn}"_red.bmp
  write_h_file "${shortfn}"_black.bmp
}

# Write "input_red.h" file with bitmap data as a C array
function write_h_file {
  local fn="$1"
  local outfile="${1%%.*}".h
  local includeguard
  includeguard="_$(echo "${outfile}" | tr 'a-z.' 'A-Z_')"

  cat <<EOHFILE > "${outfile}"
// $fn
#ifndef ${includeguard}
#define ${includeguard}
const unsigned char ${fn%%.*} [] PROGMEM = {
$(convert "${fn}" -monochrome +negate gray:- | xxd -i)
};
#endif /* ${includeguard} */
EOHFILE
}

# Produce a good 3-color approximation of what the image will look like
# when displayed on the e-ink module.
function quantize_dither {
  local fn diffusion_pct
  fn="$1"
  diffusion_pct="$2"

  convert "$fn" \
    -dither FloydSteinberg \
    -define dither:diffusion-amount="${diffusion_pct}" \
    -remap "${PALETTE_FILE}" \
    -type truecolor \
  "${fn%%.*}"_dithered.bmp
}

main "$@"
