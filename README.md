# Simulación de Fuego con Curl Noise — Port CUDA (headless) para Khipu

Este proyecto es una implementación del simulador de fuego **enteramente en CUDA**, pensada para correr en el cluster **Khipu** de UTEC (Slurm + GPU NVIDIA, **sin pantalla**). Es **autocontenido**: todas sus dependencias están en `external/` (glm, stb).

**No se usa OpenGL/EGL**: la GPU hace tanto la **simulación** (emisión + integración con curl noise) como el **rasterizado** (splatting de partículas → buffer HDR → bloom → tonemap). La salida son **secuencias de PNG RGBA** (color premultiplicado por alpha), listas para componer sobre metraje real con OpenCV.

```
   ┌── Simulación CUDA ──┐   ┌── Render CUDA (splatting) ──┐   ┌─ Export ─┐
   │ emitKernel          │   │ splat → HDR (emisivo+humo)  │   │ PNG RGBA │
   │ updateKernel (curl) │ → │ resolve → bloom → tonemap   │ → │ por frame│
   └─────────────────────┘   └─────────────────────────────┘   └──────────┘
                                                                     │
                                       composite.py (OpenCV)  ◄──────┘
                                       sobre metraje real de UTEC → video final
```

## ¿Qué produce?
Por cada escena, una carpeta con un PNG por frame: `output/<Escena>/<Escena>_00000.png`, ...
Eso es el **producto crudo**. Después:
1. (Opcional) `make_video.sh` arma un `.mp4`/`.mov` con alpha.
2. `composite.py` superpone el fuego sobre el metraje real → **video entregable**.

---

## Qué se hizo (resumen)
- **Port completo a CUDA** del simulador de fuego (antes OpenGL): emisión + integración con **curl noise** (simplex 3D/4D portado fielmente del GLSL) corriendo en GPU, headless.
- **Rasterizador tiled tipo 2D Gaussian Splatting** (adaptado y mejorado a partir de `rasrerizado_cuda`): proyección a gaussianas 2D → asignación a tiles → orden por (tile, profundidad) con thrust → blend front-to-back en memoria compartida. Fallback simple con `--simple-raster`.
- **Pipeline de imagen**: bloom + tonemap ACES + sRGB → exporta **PNG RGBA premultiplicado** por frame.
- **Composición** sobre metraje real con OpenCV (`tools/composite.py`), fuera del cluster.
- Proyecto **autocontenido** (glm y stb vendorizados en `external/`), pensado para **Slurm + Apptainer en Khipu**.
- Se corrigió un bug heredado (la combustión de materiales nunca arrancaba por `maxLifetime=10000`).

## Requisitos
- **CUDA Toolkit** ≥ 11.4 (en Khipu vía `module load cuda`, o usando el contenedor Apptainer).
- **CMake** ≥ 3.18.
- Para componer: Python 3 con `opencv-python` y `numpy` (corre en tu Mac/PC, no necesita GPU).

---

## Compilar y correr en local (PC con GPU NVIDIA)

```bash
# desde la raiz del repo
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# Render de una escena (sin ventana, escribe PNGs):
./build/fire_cuda --scene BuildingFire --width 1280 --height 720 --fps 30 --outdir output/BuildingFire
```

### Probar en una GPU modesta (ej. GTX 1650, 4 GB, sm_75)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_ARCHITECTURES=75
cmake --build build -j4
# parametros bajos para validar que corre:
./build/fire_cuda --scene WallFire --lightweight --width 1280 --height 720 --duration 3 --outdir test_out
```

> En Mac (Apple Silicon) **no se puede compilar/ejecutar** esto: CUDA es solo NVIDIA. Compílalo en Khipu o en un PC con GPU NVIDIA.

### Opciones principales
| Flag | Descripción | Default |
|------|-------------|---------|
| `--scene <s>` | `PaperFire`/`WallFire`/`TreeFire`/`StructuralFire`/`BuildingFire` o índice 0–4 | 0 |
| `--width/--height` | Resolución | 1920×1080 |
| `--fps` | Frames por segundo | 30 |
| `--duration <s>` | Sobreescribe la duración de la escena | (la de la escena) |
| `--outdir <ruta>` | Carpeta de salida | `output` |
| `--lightweight` | Menos partículas (pruebas rápidas) | off |
| `--show-geometry` | Dibuja materiales estáticos (mesa, muro…) para **preview**. No usar para composición. | off |
| `--wind <f>` | Viento en +X | 0 |
| `--glow/--exposure/--bloom/--smoke` | Ajustes de look | ver `--help` |

---

## Correr en Khipu (Slurm) — paso a paso

> Según la doc de Khipu: hay un **nodo de acceso** (solo para compilar y enviar jobs) y nodos de cómputo con GPU gestionados por **Slurm**. Nunca corras el render directo en el nodo de acceso: siempre vía `sbatch`/`srun`.

### 0. Acceso por SSH (una sola vez)
```bash
# configura tu llave SSH (ver docs.khipu.utec.edu.pe/primeros-pasos/configurar-llaves-ssh)
ssh-keygen -t ed25519                       # si no tienes llave
ssh-copy-id <usuario>@khipu.utec.edu.pe     # o registra la llave segun su guia
ssh <usuario>@khipu.utec.edu.pe             # entrar al nodo de acceso
```

### 1. Subir el código al nodo de acceso
```bash
# desde tu máquina (la composición OpenCV NO va a Khipu, queda local)
scp -r Curl_Noise_Fire <usuario>@khipu.utec.edu.pe:~/
# (o: git clone <tu-repo> dentro de Khipu)
```

### 2. Opción A — módulos del sistema
```bash
cd ~/Curl_Noise_Fire
sbatch job.sbatch BuildingFire 30      # escena, fps
squeue -u $USER                        # ver estado
# logs en logs/fire-<jobid>.out
```
`job.sbatch` pide 1 GPU (`--gres=gpu:1`), compila si hace falta y renderiza a `output/<Escena>/`.
Ajusta `module load cuda/<version>` según lo que muestre `module avail cuda` en Khipu.

### 3. Opción B — contenedor Apptainer (recomendado si los módulos dan problemas)
Construye la imagen una vez (en una máquina donde seas root o con `--fakeroot`):
```bash
apptainer build fire.sif apptainer/fire.def
```
Y en el job de Slurm, en vez de `module load`, usa:
```bash
apptainer exec --nv fire.sif bash -lc '
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build -j4 &&
  build/fire_cuda --scene BuildingFire --outdir output/BuildingFire'
```
El flag **`--nv`** expone la GPU NVIDIA del nodo dentro del contenedor.

### GPUs de Khipu (referencia)
| Nodo | GPU | VRAM | Arch (sm_) |
|------|-----|------|-----------|
| g001 | Tesla T4 | 16 GB | 75 |
| ag001 | 2× A100 | 40 GB | 80 |
| g002 / ds001 | RTX A6000 | 48 GB | 86 |

El binario se compila para `sm_75;80;86` (cubre todas). Para fijar una sola:
`cmake -S . -B build -DCMAKE_CUDA_ARCHITECTURES=80`.

### 4. Monitorear el job
```bash
squeue -u $USER                 # cola / estado (PD=pendiente, R=corriendo)
sacct -j <jobid>                # estadísticas (memoria, CPU, estado final)
tail -f logs/fire-<jobid>.out   # progreso del render (frames)
scancel <jobid>                 # cancelar si hace falta
```

### 5. Traer los frames a tu máquina (para componer)
```bash
# desde tu máquina
scp -r <usuario>@khipu.utec.edu.pe:~/Curl_Noise_Fire/output/BuildingFire ./output/
# luego, en local, componer sobre el metraje real (ver sección siguiente):
python tools/composite.py --frames output/BuildingFire --bg metraje_utec.mp4 --out incendio_utec.mp4 --fps 30 --loop-bg
```

> Resumen del flujo Khipu: **acceso SSH → subir → `sbatch job.sbatch <Escena> <fps>` → `squeue`/logs → `scp` de los PNG → componer con OpenCV en local.**

---

## Componer sobre metraje real (entregable final)

Trae los PNG de Khipu a tu máquina y compón con OpenCV:
```bash
pip install opencv-python numpy

# sobre un video real del edificio:
python tools/composite.py --frames output/BuildingFire \
    --bg metraje_utec.mp4 --out incendio_utec.mp4 --fps 30 --loop-bg

# sobre una foto fija:
python tools/composite.py --frames output/WallFire \
    --bg foto_pared.jpg --out pared.mp4 --fps 30
```
Los PNG llevan **alpha premultiplicado**, así que la composición es
`out = fuego_rgb + fondo * (1 − alpha)` (ya implementada en el script).

Para un preview rápido sobre negro o un `.mov` con alpha para editor de video:
```bash
tools/make_video.sh output/BuildingFire BuildingFire.mp4 30
```

---

## Cómo está organizado

```
.
├── include/
│   ├── vec_math.cuh       Operadores float3/float4 (host+device)
│   ├── SimplexNoise.cuh   Simplex 3D/4D portado del GLSL (Ashima/Gustavson)
│   ├── CurlNoise.cuh      Curl noise: v = ∇×Ψ por diferencias finitas
│   ├── gpu_types.cuh      GpuParticle/GpuEmitter (layout 64B = al host)
│   ├── FireEngine.h       API de host del motor
│   ├── Scenes.h           Fábrica de las 5 escenas
│   ├── Particle.h         struct Particle + EmitterConfig + CurlNoiseParams
│   ├── Camera.h           Cámara con rutas de keyframes
│   └── Scene.h            Interfaz Scene + GeometryUtils
├── src/
│   ├── FireEngine.cu      Kernels (emit/update/splat/bloom/tonemap) + host
│   ├── Scenes.cpp         Incluye las 5 escenas (sin duplicar)
│   ├── main.cpp           CLI headless + export PNG
│   ├── stb_impl.cpp       Implementación de stb_image_write
│   ├── Camera.cpp
│   └── scenes/            5 escenas + SceneUtils (geometría de partículas)
├── external/             glm + stb (vendorizados, sin internet)
├── job.sbatch            Script Slurm para Khipu
├── apptainer/fire.def    Contenedor CUDA
└── tools/                composite.py (OpenCV) + make_video.sh (ffmpeg)
```

El proyecto es **autocontenido**. La estructura `Particle` del host (64 B) se copia binariamente a `GpuParticle` en la GPU (mismo layout).

---

## Notas técnicas y limitaciones (honestas)
- **Rasterizador tiled (2DGS)**: el render por defecto usa rasterizado *tiled* (estilo Gaussian Splatting, adaptado de `rasrerizado_cuda`): cada partícula se proyecta a una gaussiana 2D, se asigna a tiles, se **ordena por (tile, profundidad)** con thrust y se compone **front-to-back** (`C += T·αᵢ·colorᵢ; T·=(1−αᵢ)`) en memoria compartida. Es más correcto y rápido que el splat aditivo. El alpha final = `1−T`. Hay un fallback más simple con `--simple-raster`.
- **Curl noise**: portado fielmente del GLSL. Es lo más costoso (simplex 4D × diferencias finitas × octavas). Si va lento, baja `octaves` en la escena o usa `--lightweight`.
- **Humo y fuego** comparten el mismo blend front-to-back: el fuego (emisivo brillante) suma luz y el humo (gris, opaco) ocluye, con orden correcto.
- **Combustión de materiales**: se corrigió un bug del diseño original (`maxLifetime=10000` impedía que el material ardiera); ahora se usa una duración de quemado acotada.
- **Geometría estática** (mesa, muro, fachada): solo se dibuja con `--show-geometry`, pensada como preview. Para la composición sobre metraje real **no** se renderiza (el fondo real ya la aporta).
- **Determinismo**: `dt` fijo (`1/fps`) y semilla basada en tiempo → frames reproducibles para video.
- **No hay colisión real con obstáculos todavía** (las partículas pueden atravesar muros). Es la siguiente mejora natural (usar un SDF + `curlNoiseWithBoundary`).
