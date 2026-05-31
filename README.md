# Simulación de Fuego Basada en Partículas con Curl Noise en GPU

## Equipo del Proyecto
- **Miembro 1**: (Nombre del estudiante)
- **Miembro 2**: (Nombre del estudiante)
- **Miembro 3**: (Nombre del estudiante)

Este proyecto implementa un sistema avanzado de simulación de fuego utilizando partículas, con aceleración por GPU (Compute Shaders de OpenGL). El núcleo del movimiento turbulento del fuego, humo y brasas se logra mediante la evaluación de **Curl Noise**, una técnica que garantiza campos vectoriales incompresibles (divergencia cero), lo cual imita el comportamiento de los fluidos de manera muy realista sin el costo computacional de resolver las ecuaciones completas de Navier-Stokes.

El proyecto permite interactuar con diferentes materiales (madera, papel, cemento, hojas de árbol) donde cada material tiene propiedades de ignición y velocidad de quemado distintas.

---

## Explicación Matemática del Curl Noise

El Curl Noise se basa en generar un campo vectorial incompresible aplicando el operador rotacional ($\nabla \times$) a un campo de potencial tridimensional generado por funciones de ruido (como el Perlin o Simplex Noise).

Sea $\vec{\Psi}(x,y,z)$ un campo de potencial vectorial tridimensional derivado de funciones de ruido:
$$ \vec{\Psi}(x,y,z) = ( \psi_x(x,y,z), \psi_y(x,y,z), \psi_z(x,y,z) ) $$

El campo de velocidades $\vec{v}(x,y,z)$ se obtiene aplicando el operador curl (rotacional) sobre $\vec{\Psi}$:
$$ \vec{v} = \nabla \times \vec{\Psi} $$

Desarrollando el rotacional en sus componentes cartesianas:
$$ \vec{v} = \left( \frac{\partial \psi_z}{\partial y} - \frac{\partial \psi_y}{\partial z}, \quad \frac{\partial \psi_x}{\partial z} - \frac{\partial \psi_z}{\partial x}, \quad \frac{\partial \psi_y}{\partial x} - \frac{\partial \psi_x}{\partial y} \right) $$

Una propiedad matemática fundamental del operador rotacional es que su divergencia siempre es exactamente cero ($\nabla \cdot (\nabla \times \vec{\Psi}) = 0$). Dado que en la dinámica de fluidos la ecuación de continuidad para un fluido incompresible es $\nabla \cdot \vec{v} = 0$, el campo de velocidades generado por el Curl Noise es intrínsecamente incompresible, evitando que las partículas converjan en puntos específicos (efecto de "pozo") o se dispersen artificialmente (efecto de "fuente").

Para calcular las derivadas espaciales de manera eficiente, empleamos diferencias finitas centrales:
$$ \frac{\partial \psi}{\partial x} \approx \frac{\psi(x + \epsilon, y, z) - \psi(x - \epsilon, y, z)}{2\epsilon} $$

---

## Instrucciones de Compilación

### Dependencias
- C++17
- CMake (>= 3.15)
- OpenGL (>= 4.3 para Compute Shaders)
- GLFW
- GLAD
- GLM

### Compilar en Linux (Servidor Khipu)

```bash
mkdir build
cd build
cmake ..
make -j4
```
*Nota para servidores sin pantalla (Headless):* Asegúrese de ejecutar bajo un entorno que provea contexto OpenGL, o exportar variables de entorno como `DISPLAY`. Si es estrictamente sin ventanas, el código de renderizado deberá utilizar un contexto off-screen (e.g., EGL).

### Compilar en Windows

Se recomienda utilizar Visual Studio (con soporte para C++) o MinGW-w64.
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## Instrucciones de Uso

El ejecutable principal acepta argumentos de línea de comandos para seleccionar la escena, controlar la carga gráfica y establecer parámetros de renderizado fuera de pantalla.

```bash
./fire-simulation [opciones]
```

**Opciones de Línea de Comandos:**
- `--scene <nombre>` : Ejecuta una escena específica. Opciones: `PaperFire`, `WallFire`, `TreeFire`, `StructuralFire`, `BuildingFire`. (Por defecto: ejecuta todas secuencialmente).
- `--lightweight` : Reduce drásticamente el número de partículas para GPUs de gama de entrada (como la GTX 1650).
- `--headless` : Renderiza frames directamente a disco sin abrir una ventana de visualización (ideal para el servidor Khipu).
- `--outdir <ruta>` : Define el directorio de salida donde se guardarán los frames si se utiliza `--headless` (Por defecto: `./output_frames/`).

**Ejemplo de uso:**
```bash
./fire-simulation --scene TreeFire --lightweight
```

---

## Descripciones de las Escenas

1. **PaperFire**: Una hoja de papel sobre un escritorio de madera. El fuego comienza en una esquina y se propaga lentamente de forma diagonal. Demuestra el consumo de material ligero.
2. **WallFire**: Un muro de cemento de grandes dimensiones con fuego lamiendo la superficie desde la base. El cemento no se quema, ilustrando el tratamiento de colisiones y límites físicos con el Curl Noise.
3. **TreeFire**: Un árbol compuesto por cilindros (tronco), líneas (ramas) y esferas de partículas (hojas). Muestra variaciones drásticas en la velocidad de propagación: las hojas se incendian y desaparecen rápidamente, mientras que la madera arde lento.
4. **StructuralFire**: Estructura de vigas, columnas y paredes interiores de cemento con una puerta de madera. Simulando el interior de un cuarto, el humo espeso y la alta turbulencia se abren paso buscando escapes de oxígeno a través de la entrada de madera.
5. **BuildingFire**: Escena final de mayor magnitud (fachada de UTEC). Representa un edificio con huecos para ventanas y un intenso fuego originado en las plantas bajas que envuelve progresivamente las estructuras superiores con una masiva columna de humo.

---

## Referencias

- Bridson, R., Hourihan, J., & Marcus, M. (2007). *Juggling: turbulent flow and curl noise*. In ACM SIGGRAPH 2007 courses (SIGGRAPH '07). Association for Computing Machinery, New York, NY, USA, 10–es. https://doi.org/10.1145/1281500.1281671
- Perlin, K. (2002). *Improving Noise*. ACM Trans. Graph., 21(3), 681–682.
