# King of the Hill

Prototipo digital del juego de mesa, en C++ con raylib. De 1 a 4 jugadores en
la misma computadora —los lugares que sobran los juega la CPU—: exploración de
la montaña con eventos de D20, las 96 bendiciones y maldiciones del diseño con
sus efectos reales, y torneo PvP al mejor de 3 tiradas con batalla final.

**Integrantes:** Federico Fernandez Soto · Salvador Rosafioriti · Valentin Reyes

## Compilar

Abrí `KingOfTheHill.slnx` y compilá en **x64**. No hay nada que configurar:
raylib y las fuentes viajan dentro del repo y el `.vcxproj` las busca con rutas
relativas, así que compila igual en cualquier PC con Visual Studio.

```
King-of-the-Hill/
├── KingOfTheHill.slnx
├── KingOfTheHill.vcxproj          // includes, libs y salidas, todo relativo
├── src/                           // codigo fuente
├── assets/fonts/                  // Work Sans (Regular y Bold) + su licencia
├── assets/audio/                  // musica y efectos (sintetizados para el juego)
├── lib/raylib/
│   ├── include/                   // raylib.h, raymath.h, rlgl.h
│   └── lib/x64/raylib.lib         // raylib 6.0 estatica (msvc)
├── bin/x64/<Config>/              // ejecutable + assets (se genera)
└── obj/x64/<Config>/              // intermedios (se generan)
```

- `Debug` compila **con consola**; `Release`, **sin consola** (subsistema
  Windows con `EntryPointSymbol = mainCRTStartup`).
- `raylib.lib` es la estática: el `.exe` no necesita DLL, sólo la carpeta
  `assets`, que el evento post-build copia al lado del ejecutable.
- Sólo x64. Para Win32 hay que poner la `raylib.lib` de 32 bits en
  `lib\raylib\lib\Win32`.
- En Debug se omite la biblioteca `MSVCRT` para que no choquen dos runtimes
  (la raylib oficial viene compilada contra el runtime release).

## Controles

| Fase | Teclas |
|---|---|
| Menú | `1`…`4` cuántos juegan · `ENTER` empezar |
| Elegir camino | `1` fácil (gris) · `2` normal (rojo) · `3` difícil (púrpura) |
| Tirar el D20 | `ESPACIO` |
| Elegir carta | `1` · `2` · `3` |
| Combate | `ESPACIO` por tirada |
| Elegir (cartas que lo piden) | `1` … `5` |
| Pantalla final | `R` jugar de nuevo · `ESC` salir |
| En cualquier momento | `F11` o `Alt+Enter` pantalla completa · `M` silenciar · `ESC` salir (o volver a ventana) |

## Reglas

- **Exploración**: 6 etapas. Cada jugador elige libremente uno de los tres
  caminos y tira un D20 contra la dificultad de la casilla (fácil 8, normal 12,
  difícil 16, con +1 cada dos etapas).
- **Cartas**: se ofrecen 3 del mazo correspondiente, el jugador guarda 1 y las
  otras 2 vuelven al mazo. El camino define el mazo: fácil da comunes, normal
  peligrosas y difícil épicas.
- **Críticos**: 20 natural gana solo, 1 natural pierde solo; en ambos casos se
  ignoran los modificadores.
- **Torneo**: iniciativa de los 4; se cruzan los dos más altos y los dos más
  bajos; después juegan los dos perdedores (3er puesto) y por último los dos
  ganadores. Cada combate es **al mejor de 3 tiradas**, tirada por tirada.
- **Recompensa de combate**: el ganador le roba una bendición al perdedor (es lo
  que da sentido a la carta *Corona rota*, que la bloquea).
- **Pantalla final**: corona al rey de la colina y muestra las cuatro
  posiciones. Con `R` arranca otra partida.

## Jugadores y CPU

En el menú se elige con `1`…`4` cuántas personas juegan; los lugares que quedan
libres los toma la CPU, que juega con las mismas reglas y las mismas cartas (no
hace trampa ni ve nada que un jugador no vea). Las fichas de CPU están marcadas
en el panel de jugadores y en los combates.

La CPU piensa medio segundo antes de cada acción para que se pueda seguir lo
que hace, y sus decisiones están en `src/Cpu.cpp`:

- **Camino**: si junta más maldiciones que bendiciones va por el fácil; en las
  dos últimas etapas va al difícil para buscar cartas épicas; si le está yendo
  bien también arriesga. Una de cada cinco veces elige al azar, para que no sea
  siempre igual.
- **Cartas**: puntúa las tres que le ofrecen y se queda con la mejor (o con la
  maldición menos dañina). El puntaje por tipo de efecto está en `ScoreCard`.
- **Elecciones** (Dado Cambiado, Apuesta controlada, Predicción…): toma la
  opción de mayor valor.

Es una CPU de prototipo: alcanza para que una partida de 1 jugador tenga
sentido, pero no planifica a futuro ni mira la mano de los rivales.

## Las 96 cartas

Están implementadas **las 96**, con su nombre y su texto tal como figuran en
`King of the hill cartas - estado.xlsx`. El catálogo vive en `src/Cards.cpp`:
una fila por carta con su tipo de efecto y sus parámetros. El motor que las
aplica es `src/Effects.cpp`.

La mayoría se activan solas en la tirada que indica su texto. Las que dicen
"elegí" o "podés" abren una pantalla de elección con teclas `1`…`5`:
*Dado Cambiado* (5 variantes), *Fortuna*, *Doble o nada*, *Apuesta controlada*,
*Predicción* y *Destino alterado*.

Orden de resolución de una tirada:

1. **Dados**: cuántos se tiran (`Doble o nada`, `Fortuna`, `Dado Cambiado`),
   bloqueos (`Agotamiento`, `Ancla`) y repeticiones (`Mala racha`).
2. **Conversiones** de un valor por otro (`Maldición del veinte`,
   `Talón de Aquiles`).
3. **Críticos**: 20 natural y 1 natural, que ignoran todo lo demás. Como la
   conversión pasa antes, `Maldición del veinte` sí te arruina un 20.
4. **Restricciones** sobre tus bendiciones (`Cadenas`, `Inercia`, `Bloqueo`,
   `Despojo`).
5. **Cartas del rival que tuercen las tuyas** (`Twisted`, `Negación`).
6. **Cartas que modifican lo que recibís** (`Escudo`, `Eco negativo`).
7. **Castigos por usar bendiciones** (`Peso muerto`, `Fractura`,
   `Desconcentración`, `Riesgo calculado`, `Fractura del destino`).
8. **Sumas y restas**: umbrales, paridad, Fibonacci, historial del combate y
   bonos planos.
9. **Multiplicar y dividir** (`Multiplicar`, `Dividir`, `Juego Seguro`).
10. **Topes** (`Tropiezo`, `Temor`, `Tormenta`, `Punto muerto`) y piso en 1.

Quién gana cada tirada también es configurable: `Justicia Divina` y
`Reverso del destino` hacen ganar al más bajo, `Redistribución de las riquezas`
al más cercano a 10, `Trampa de oso` le pone un mínimo al rival y `En empate`
convierte tus empates en derrotas.

### Supuestos que conviene revisar jugando

Tres cartas quedaron abiertas a interpretación y las resolví así:

- **Apuesta controlada**: la elección es *apostar o no*. Si apostás, 10 o menos
  suma 5 y 11 o más resta 2.
- **Despojo**: como no hay UI para que el rival elija, el juego le bloquea
  automáticamente tu mejor bendición.
- **Corona rota**: implica que ganar un combate da recompensa, cosa que el
  documento no detallaba. Implementé la recompensa como "el ganador roba una
  bendición al azar del perdedor".

Además, **Dado Cambiado** deja el resultado entre 1 y 5 (o 1 y 2): es lo que
dice el texto, pero contra dificultades de 8 a 18 es casi siempre una tirada
perdida. Vale la pena testearla.

## Archivos

| Archivo | Qué hace |
|---|---|
| `src/main.cpp` | ventana y loop principal |
| `src/Types.h/.cpp` | constantes de balance, `CardInstance`, `Player` |
| `src/Dice.h/.cpp` | el D20 y el RNG |
| `src/Cards.h/.cpp` | catálogo de las 96 cartas y los 6 mazos |
| `src/Effects.h/.cpp` | motor de efectos: cómo se resuelve cada tirada |
| `src/Board.h/.cpp` | geometría del tablero, dificultades y dibujo |
| `src/Combat.h/.cpp` | iniciativa y torneo al mejor de 3 |
| `src/Game.h/.cpp` | máquina de estados de la partida y UI |
| `src/Cpu.h/.cpp` | las decisiones de los jugadores controlados por la máquina |
| `src/Ui.h/.cpp` | fuentes y dibujo de todo el texto |
| `src/Theme.h` | la paleta del folleto en un solo lugar |
| `src/Fx.h/.cpp` | fondo, montañas, nieve, paneles, corona y curvas de animación |
| `src/Audio.h/.cpp` | música, efectos y mute |
| `src/Assets.h/.cpp` | dónde buscar los archivos de `assets/` |

Para agregar o cambiar una carta alcanza con tocar su fila en `src/Cards.cpp`.
Si el efecto no entra en los tipos existentes, se agrega al `enum EffectKind` y
su caso en `Effects.cpp`.

Balance concentrado en: `Types.h` (etapas y jugadores), `Board.cpp`
(`GetEventDifficulty`) y el catálogo de `Cards.cpp`.

## Presentación

La paleta sale del folleto del juego: magenta, azul noche y crema, con dorado
para la corona. Está toda en `src/Theme.h`, así que cambiar el look es tocar un
archivo.

- **Fondo** (`src/Fx.cpp`): degradado de cielo nocturno, resplandor magenta sobre
  el horizonte, tres cadenas de montañas y nieve cayendo. Todo dibujado por
  código, sin imágenes.
- **Dado**: al tirar, el número gira medio segundo antes de quedarse quieto, y
  recién ahí entra el sonido del resultado. Durante ese rato no se acepta input,
  así que no se puede saltear la tirada sin querer.
- **Cartel** de ¡ÉXITO! o FALLO que entra con un rebote y se va solo.
- **Tablero**: casillas con relieve y borde del color del camino, el camino como
  una ruta gruesa, y la ficha del jugador en turno flota con un anillo que late.
- **Combate**: tres fichas arriba a la derecha que se van pintando con el color
  de quien ganó cada tirada.
- **Menú y pantalla final**: banda magenta con las curvas del folleto, el título
  con su corona y las posiciones finales en un panel.

Nada de esto usa imágenes: son primitivas de raylib, así que no agrega peso al
repo ni dependencias.

## Pantalla completa y escalado

`F11` (o `Alt+Enter`) pasa a pantalla completa y vuelve; `ESC` también sale de
pantalla completa antes de cerrar el juego. La ventana además se puede
redimensionar a mano.

El juego se dibuja **siempre sobre un lienzo de 1280x720** (`RenderTexture2D`) y
recién al final se escala a la ventana, centrado y conservando la proporción,
con bandas negras si hace falta. Por eso no hay que recalcular ninguna posición:
la interfaz se ve igual en 1280x720 que en 4K. El filtrado es bilineal, así que
al agrandar no queda pixelado.

Lo único que hay que respetar si se agregan pantallas nuevas: usar las
constantes `SCREEN_WIDTH` y `SCREEN_HEIGHT` de `Types.h` y **no** llamar a
`GetScreenWidth()`, que dentro del lienzo devuelve el tamaño de la ventana real
y descoloca el dibujo.

## Audio

La música y los efectos están en `assets/audio` y son **sintetizados a medida
para este proyecto** (no hay samples de terceros, así que no hay problema de
licencia para publicarlo en itch).

| Archivo | Cuándo suena |
|---|---|
| `dice.wav` | al tirar el D20 (exploración, iniciativa y combate) |
| `success.wav` / `fail.wav` | medio segundo después del dado, con el resultado |
| `crit.wav` | 20 natural |
| `card.wav` | al quedarte con una carta |
| `select.wav` | menús, elección de camino y elecciones de cartas |
| `hit.wav` | al comparar las dos tiradas de un combate |
| `victory.wav` | al coronar al rey de la colina |
| `music_explore.ogg` | loop de la fase de exploración (23 s) |
| `music_combat.ogg` | loop del torneo (27 s) |

`src/Audio.cpp` concentra todo: carga, mute con `M`, cambio de pista y una cola
de efectos con retardo (por eso el resultado de la tirada suena *después* del
dado y no encima). Si los archivos no están, el juego arranca igual en silencio.

El estilo es chiptune porque los generé por síntesis. Si consiguen un pack CC0
con mejor producción, alcanza con reemplazar los archivos respetando los
nombres: el código no cambia.

## Fuentes

Todo el texto se dibuja con **Work Sans** (`assets/fonts/`), incluida en el
repo bajo SIL Open Font License 1.1. `src/Ui.cpp` la carga una sola vez con
`LoadFontEx` a 64 px y la busca en la carpeta de trabajo, al lado del ejecutable
y tres niveles arriba; si no la encuentra, usa la fuente por defecto de raylib.

## Para la entrega

En el zip de código fuente van `src/`, `assets/`, `lib/`, el `.slnx` y el
`.vcxproj`. `bin/` y `obj/` se generan al compilar y quedan afuera.
