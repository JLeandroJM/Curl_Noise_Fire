#!/usr/bin/env python3
# =============================================================================
# composite.py - Compone los frames PNG RGBA (fuego/humo) sobre metraje real
# usando alpha premultiplicado:  out = fg_rgb + bg * (1 - alpha)
#
# Los PNG que produce fire_cuda son RGBA con color PREMULTIPLICADO por alpha,
# por eso la formula no vuelve a multiplicar fg por alpha.
#
# Ejemplos:
#   # Sobre un video real:
#   python composite.py --frames output/BuildingFire --bg metraje_utec.mp4 \
#       --out incendio_utec.mp4 --fps 30
#
#   # Sobre una imagen fija (se repite en todos los frames):
#   python composite.py --frames output/WallFire --bg foto_pared.jpg \
#       --out pared.mp4 --fps 30
#
# Requiere: pip install opencv-python numpy
# =============================================================================
import argparse
import glob
import os
import sys

import cv2
import numpy as np


def list_frames(folder):
    files = sorted(glob.glob(os.path.join(folder, "*.png")))
    if not files:
        sys.exit(f"No se encontraron PNG en {folder}")
    return files


def open_background(path):
    """Devuelve (tipo, handle). tipo: 'video' o 'image'."""
    ext = os.path.splitext(path)[1].lower()
    if ext in (".mp4", ".mov", ".avi", ".mkv"):
        cap = cv2.VideoCapture(path)
        if not cap.isOpened():
            sys.exit(f"No se pudo abrir el video de fondo: {path}")
        return "video", cap
    img = cv2.imread(path, cv2.IMREAD_COLOR)
    if img is None:
        sys.exit(f"No se pudo abrir la imagen de fondo: {path}")
    return "image", img


def main():
    ap = argparse.ArgumentParser(description="Compone fuego RGBA sobre metraje real.")
    ap.add_argument("--frames", required=True, help="Carpeta con los PNG RGBA")
    ap.add_argument("--bg", required=True, help="Fondo: video o imagen")
    ap.add_argument("--out", required=True, help="Video de salida (.mp4)")
    ap.add_argument("--fps", type=float, default=30.0)
    ap.add_argument("--loop-bg", action="store_true",
                    help="Si el video de fondo es mas corto, reiniciarlo")
    args = ap.parse_args()

    frames = list_frames(args.frames)
    bg_type, bg = open_background(args.bg)

    # Resolucion de salida = la de los frames de fuego.
    first = cv2.imread(frames[0], cv2.IMREAD_UNCHANGED)
    if first is None or first.shape[2] != 4:
        sys.exit("Los frames deben ser PNG RGBA (4 canales).")
    H, W = first.shape[:2]

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(args.out, fourcc, args.fps, (W, H))
    if not writer.isOpened():
        sys.exit(f"No se pudo abrir el writer: {args.out}")

    for i, fpath in enumerate(frames):
        fg = cv2.imread(fpath, cv2.IMREAD_UNCHANGED)  # BGRA (OpenCV)
        if fg is None:
            continue
        if fg.shape[:2] != (H, W):
            fg = cv2.resize(fg, (W, H))

        # Fondo de este frame
        if bg_type == "video":
            ok, bframe = bg.read()
            if not ok:
                if args.loop_bg:
                    bg.set(cv2.CAP_PROP_POS_FRAMES, 0)
                    ok, bframe = bg.read()
                if not ok:
                    break
            bframe = cv2.resize(bframe, (W, H))
        else:
            bframe = cv2.resize(bg, (W, H))

        # composicion premultiplicada: out = fg_rgb + bg*(1-a)
        bgr = fg[:, :, :3].astype(np.float32)        # ya premultiplicado
        alpha = fg[:, :, 3].astype(np.float32) / 255.0
        alpha = alpha[:, :, None]
        bg_f = bframe.astype(np.float32)
        out = bgr + bg_f * (1.0 - alpha)
        out = np.clip(out, 0, 255).astype(np.uint8)

        writer.write(out)
        if i % 30 == 0:
            print(f"\rcompuesto {i+1}/{len(frames)}", end="", flush=True)

    writer.release()
    if bg_type == "video":
        bg.release()
    print(f"\nGuardado: {args.out}")


if __name__ == "__main__":
    main()
