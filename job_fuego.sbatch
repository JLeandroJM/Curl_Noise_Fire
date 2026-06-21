#!/bin/bash
#SBATCH --job-name=fire_sim
#SBATCH --partition=data-science
#SBATCH --gres=shard:4
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=02:00:00
#SBATCH --output=fire_sim_%j.out
#SBATCH --error=fire_sim_%j.err
# Para forzar un nodo libre, descomenta (ej. la Tesla T4):
# #SBATCH --nodelist=g001

set -e
cd $HOME/Curl_Noise_Fire

# Escena por argumento:  sbatch job_fuego.sbatch Test5 "--solid --psize 1.5"
# $1 = escena (def Test5). $2 = flags extra de render (def: --solid para preview tipo Jean).
# Para la composición final (RGBA transparente) usa: sbatch job_fuego.sbatch BuildingFire ""
SCENE="${1:-Chair}"
EXTRA="${2:---solid --no-smoke}"

echo "=== Nodo: $(hostname) | Escena: ${SCENE} | Extra: ${EXTRA} ==="
echo "=== Ejecutando dentro del Contenedor Apptainer ==="

# OJO: comillas DOBLES para que ${SCENE}/${EXTRA} se expandan antes de entrar al contenedor.
apptainer exec --nv fire.sif bash -lc "
  rm -rf build/ &&
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build -j4 &&
  build/fire_cuda --scene ${SCENE} --width 1920 --height 1080 --fps 30 ${EXTRA} --outdir output/${SCENE}
"

echo "=== Render Terminado: output/${SCENE} ==="
