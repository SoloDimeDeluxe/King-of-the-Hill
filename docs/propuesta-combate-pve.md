# Propuesta: combate contra enemigos (PvE)

Borrador para que el grupo lo apruebe o lo corrija.

> **Estado: programado de forma provisional.** Está implementado en
> `src/Enemy.cpp` y `src/Game.cpp` para poder probarlo jugando mientras el grupo
> decide. Lo que se jugó son estas reglas con los números afinados (ver
> *Ajustes del prototipo*, al final) y las cuatro decisiones abiertas resueltas
> por defecto. Nada de eso está cerrado: cambiar `MakeEnemy` cambia el juego.

Responde las tres preguntas que el documento de diseño dejó abiertas:

> *¿Cómo funcionan los enemigos, los turnos, los ataques y los cálculos de daño?*
> *¿De qué sirve pegarle al enemigo en diferentes partes?*
> *¿Cómo funcionan las tiradas?*

La propuesta respeta dos cosas que ya están escritas en el documento: que las
tiradas se resuelven con el D20 contra una dificultad, y que **las partes menos
importantes del enemigo son las que deciden si el jugador supera la prueba**.

---

## 1. Qué es un enemigo

Un enemigo es un conjunto de **partes**. Cada parte tiene tres números:

| Parte | Dificultad | Heridas | Para qué sirve |
|---|---|---|---|
| Cola / patas | baja (camino −2) | 1 | derribarla **acerca a ganar** |
| Cuerpo | normal (camino +0) | 2 | derribarlo **acerca a ganar** |
| Cabeza | alta (camino +4) | 2 | **no hace falta para ganar**: da la recompensa grande |

La "dificultad del camino" es la que ya usa el juego: 8 en el fácil, 12 en el
normal, 16 en el difícil, +1 cada dos etapas. O sea que en el camino normal de
la etapa 1 la cola pide 10, el cuerpo 12 y la cabeza 16.

**Heridas** es cuántos aciertos hacen falta para derribar esa parte.

## 2. Cómo se gana el combate

Se gana al derribar **cola y cuerpo**. La cabeza es opcional.

Esto es lo que pide el documento: las partes menos importantes son las que
deciden la prueba, y la cabeza es la apuesta. Pegarle a la cabeza no te acerca a
ganar el combate: te acerca a la recompensa.

- **Ganar el combate** → la recompensa normal que ya existe: elegir una
  bendición entre tres.
- **Derribar además la cabeza** → una bendición del nivel de arriba (en el
  camino fácil te dan una peligrosa, en el normal una épica).
- **Perder el combate** → una maldición, como cuando hoy se falla un evento.

## 3. Cómo es un turno

El combate dura **3 turnos**. En cada turno:

1. **Elegís la parte** que atacás, con `1`, `2` o `3`. La pantalla muestra la
   dificultad de cada una y cuántas heridas le quedan.
2. **Tirás el D20.** Se aplican las cartas exactamente igual que en el resto del
   juego: umbrales, paridad, multiplicadores, topes. El 20 natural es acierto
   automático y el 1 natural, fallo automático.
3. **Si igualás o superás la dificultad**, le sacás una herida a esa parte. Si
   la bajás a cero, la parte queda derribada y ya no se puede atacar.
4. **Si fallás**, el enemigo contraataca: recibís una maldición del nivel del
   camino, sin poder elegir entre tres.

Si al terminar los 3 turnos no derribaste cola y cuerpo, perdiste el combate.

**Por qué 3 turnos y no vida del jugador:** el documento no tiene barra de vida
en ningún lado, y meterla obliga a inventar curación, daño y muerte. Con 3
turnos el límite es el tiempo, no la vida, y el castigo por fallar son las
maldiciones, que es el recurso que el juego ya maneja. Si prefieren vida, es la
decisión **A** de abajo.

## 4. La cuenta, en un ejemplo

Camino normal, etapa 3. Cola 11, cuerpo 13, cabeza 17. El jugador tiene
*Impulso* (+2) sin usar.

- **Turno 1** — ataca la cola. Saca 9, +2 de Impulso = 11. Alcanza: cola
  derribada (1 herida).
- **Turno 2** — ataca el cuerpo. Saca 6. No alcanza: recibe una maldición normal.
- **Turno 3** — ataca el cuerpo otra vez. Saca 15. Acierta, pero al cuerpo le
  quedaba 1 de 2 heridas, así que queda en 1.

Perdió: le faltó derribar el cuerpo. Si en el turno 2 hubiera acertado, llegaba.

Ese es el ritmo buscado: tres turnos alcanzan **justo** si no fallás, y la
cabeza sólo la intenta el que va sobrado o el que ya está perdido.

## 5. Enemigos, mini jefe y jefe final

- **Enemigo común** (casillas de evento): las 3 partes de la tabla.
- **Mini jefe** (mitad del mapa): 4 partes (se suma un brazo o un ala de
  dificultad media, 2 heridas) y **4 turnos** en vez de 3.
- **Jefe final** (la cima): 4 partes, **5 turnos**, y hay que derribar tres de
  las cuatro. Es el que reparte la corona.

Los nombres y el aspecto de cada enemigo los pone el grupo; para el código sólo
hacen falta esos números.

---

## Lo que tienen que decidir

**A. ¿Vida del jugador?** La propuesta dice que no: el límite son los turnos y
el castigo son las maldiciones. La alternativa es darle 3 puntos de vida al
jugador y que cada fallo le saque uno. Agrega tensión y obliga a definir si la
vida se cura entre combates.

**B. ¿El combate PvE reemplaza al evento, o convive?** Hoy cada casilla es una
tirada contra una dificultad. Puede ser que **todas** las casillas pasen a ser
enemigos, o que se mezclen (unas de evento, otras de combate). Esto define el
ritmo de la partida y también la variedad del tablero.

**C. ¿Cuántas partes muestra la pantalla?** Tres entra cómodo con las teclas
`1`, `2` y `3`. Con cuatro ya hay que usar `4` o flechas. Si el jefe final tiene
cuatro, conviene que todos usen cuatro para que el control sea siempre igual.

**D. Las recompensas de arriba.** ¿Derribar la cabeza da una carta de nivel
superior, dos cartas, o algo distinto (plata, ítems)? El documento menciona
"batallas opcionales con recompensas afuera del personaje" y eso todavía no
tiene reglas.

Con esas cuatro respuestas el combate se programa derecho: el motor de tiradas y
de cartas ya está hecho y es el mismo que usaría el PvE.

---

## Ajustes del prototipo

Lo que está programado hoy, para que lo prueben y lo corrijan. Dos cosas
cambiaron respecto de la propuesta de arriba, y las dos por el mismo motivo:
pedir la dificultad del camino en **cada** parte hacía el combate casi
imposible. Con la cola en 10 y el cuerpo en 12, acertar las tres tiradas
seguidas que hacen falta pasa una de cada diez veces; en el camino difícil, una
de cada cien.

**1. Las partes piden menos que el evento.** Como hay que acertar varias veces,
cada parte baja respecto de la dificultad del camino en lugar de igualarla:

| Etapa | Enemigo | Partes (ajuste / heridas) | Turnos | Hay que derribar |
|---|---|---|---|---|
| 2 | Bestia del camino | Cola −4/1 · Cuerpo −2/1 · **Cabeza** +2/1 | 3 | 2 |
| 4 | Guardián de la ladera | Patas −5/1 · Cuerpo −3/1 · Ala −2/1 · **Cabeza** +2/1 | 5 | 3 |
| 6 | Señor de la cima | Cola −5/1 · Cuerpo −4/1 · Garra −3/1 · **Cabeza** +1/2 | 5 | 3 |

Con esto, sobre 2.400 combates simulados los tres se ganan alrededor del 90 % en
el camino fácil, 60 % en el normal y 22 % en el difícil. Queda cerca de la
tirada de evento (65 / 45 / 25) y un poco más generoso, que es lo justo porque
cada fallo ya cuesta una maldición.

**2. El combate no se corta al ganar.** Si derribaste lo que hacía falta y te
sobran turnos, podés cobrar la recompensa o seguir apostando por la cabeza. Así
la cabeza es una decisión real y no una carta que nunca se juega.

Las respuestas por defecto a las cuatro preguntas:

- **A. Vida del jugador:** no. El límite son los turnos.
- **B. Eventos o combates:** conviven. Etapas 1, 3 y 5 son eventos; 2, 4 y 6 son
  combates. Se cambia en `StageHasEnemy`.
- **C. Partes en pantalla:** hasta cuatro, siempre con las teclas `1`…`4`.
- **D. La cabeza:** da una bendición de un nivel más alto. En el camino difícil,
  que ya da épicas, no sube más.

Una cosa que quedó fuera a propósito: las tiradas del PvE pasan por el motor de
cartas como **tiradas de exploración**, no de combate. O sea que las cartas que
dicen "en tu próximo combate" no se gastan contra los enemigos, sólo en el
torneo final. Si el grupo quiere lo contrario es una línea en `StartEnemyRoll`.
