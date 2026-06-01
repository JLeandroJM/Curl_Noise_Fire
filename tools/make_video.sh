#!/bin/bash
# =============================================================================
# make_video.sh - Ensambla los PNG RGBA en un video.
#   ./make_video.sh output/BuildingFire BuildingFire.mp4 30
#       $1 = carpeta de frames
#       $2 = video de salida (def: out.mp4)
#       $3 = fps (def: 30)
#
# Genera dos cosas:
#   - <salida>          : video sobre fondo NEGRO (preview rapido)
#   - <salida>.mov      : video con alpha (ProRes 4444) para edicion/composicion
# Requiere ffmpeg.
# =============================================================================
set -e

FRAMES="${1:?carpeta de frames requerida}"
OUT="${2:-out.mp4}"
FPS="${3:-30}"

# Detecta el patron de nombre (Escena_00000.png)
PATTERN=$(ls "${FRAMES}"/*_00000.png 2>/dev/null | head -n1 | sed 's/00000/%05d/')
if [ -z "${PATTERN}" ]; then
    echo "No encontre frames *_00000.png en ${FRAMES}"; exit 1
fi

echo "Patron: ${PATTERN}"

# 1) Preview sobre negro (descarta alpha)
ffmpeg -y -framerate "${FPS}" -i "${PATTERN}" \
    -c:v libx264 -pix_fmt yuv420p -crf 18 "${OUT}"

# 2) Con canal alpha (ProRes 4444) para componer en un editor
ffmpeg -y -framerate "${FPS}" -i "${PATTERN}" \
    -c:v prores_ks -profile:v 4444 -pix_fmt yuva444p10le "${OUT%.mp4}.mov"

echo "Generados: ${OUT}  y  ${OUT%.mp4}.mov (con alpha)"
