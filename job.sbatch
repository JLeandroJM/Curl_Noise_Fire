#!/bin/bash
# =============================================================================
# job.sbatch - Renderiza una escena de fuego en Khipu (Slurm + GPU NVIDIA).
# Uso:
#   sbatch job.sbatch BuildingFire 30
#         $1 = nombre de escena (def: BuildingFire)
#         $2 = fps (def: 30)
# Salida: frames PNG en  output/<Escena>/
# =============================================================================
#SBATCH --job-name=fire-cuda
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --gres=gpu:1
#SBATCH --time=01:00:00
#SBATCH --output=logs/fire-%j.out
#SBATCH --error=logs/fire-%j.err

set -e
mkdir -p logs

SCENE="${1:-BuildingFire}"
FPS="${2:-30}"

# --- Modulos: ajustar segun 'module avail' en Khipu ---
module purge
# Cargar el toolkit de CUDA disponible en el cluster (ver: module avail cuda)
module load cuda || module load cuda/12.4 || true

echo "Nodo:        $(hostname)"
echo "GPU(s):"
nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader || true
echo "nvcc:        $(command -v nvcc) ; $(nvcc --version | tail -n1)"

# --- Compilar SIEMPRE (incremental: solo recompila lo que cambió) ---
# Importante: tras editar el codigo hay que recompilar, si no se usa el binario viejo.
BUILD_DIR=build
echo "== Compilando (incremental) =="
cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j4

# --- Render ---
OUTDIR="output/${SCENE}"
mkdir -p "${OUTDIR}"

echo "== Renderizando ${SCENE} @ ${FPS}fps =="
srun "${BUILD_DIR}/fire_cuda" \
    --scene "${SCENE}" \
    --width 1920 --height 1080 \
    --fps "${FPS}" \
    --outdir "${OUTDIR}"

echo "== Frames en ${OUTDIR}/ =="
ls "${OUTDIR}" | head
echo "Total frames: $(ls "${OUTDIR}" | wc -l)"
