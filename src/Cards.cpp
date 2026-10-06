#include "Cards.h"
#include "Dice.h"

namespace ReyesCuadra
{
    static const CardDef CARD_CATALOG[] =
    {
        { "Ying y Yang",
          "En tu próximo combate, restás 1 a todas tus tiradas. En el combate siguiente, sumás 3 a todas tus tiradas.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, -1, 3, 0, -1, false, 0, 0 },

        { "Twisted",
          "Durante tu próximo combate, si tu rival usa una Bendición que suma un número fijo a una tirada, esa suma se convierte en una resta por el mismo valor.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::TwistRival, EffectScope::NextCombat, 0, 0, 0, -1, true, 0, 0 },

        { "Justicia Divina",
          "Durante tu próximo combate, en cada tirada gana quien obtiene el número más bajo.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::WinRule, EffectScope::NextCombat, 0, 0, 0, -1, false, 0, 0 },

        { "Redistribución de las riquezas",
          "Durante tu próximo combate, en cada tirada gana quien obtiene el resultado más cercano a 10.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::WinRule, EffectScope::NextCombat, 1, 10, 0, -1, false, 0, 0 },

        { "Multiplicar",
          "En tu próximo lanzamiento, duplicás el resultado. El resultado final no puede superar 20.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::MultiplyCapped, EffectScope::NextRoll, 2, 20, 0, -1, false, 0, 0 },

        { "Juego Seguro",
          "En tu próximo lanzamiento, si sacás entre 1 y 9, triplicás el resultado. Si sacás entre 11 y 20, dividís el resultado entre 2 y redondeás hacia arriba. Si sacás 10, no cambia.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::SafePlay, EffectScope::NextRoll, 9, 3, 11, -1, false, 0, 0 },

        { "Trampa de oso",
          "Durante tu próximo combate, una tirada del rival solo puede ganar si obtiene al menos 10.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::WinRule, EffectScope::NextCombat, 2, 10, 0, -1, false, 0, 0 },

        { "Trampa de oso +",
          "Durante tu próximo combate, una tirada del rival solo puede ganar si obtiene al menos 15.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::WinRule, EffectScope::NextCombat, 2, 15, 0, -1, false, 0, 0 },

        { "Dos caminos",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 3. Si es par, restás 3.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 3, -3, -1, false, 0, 0 },

        { "Dos caminos",
          "En tu próximo lanzamiento, si el resultado es par, sumás 3. Si es impar, restás 3.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 3, -3, -1, false, 0, 0 },

        { "Fibonacci",
          "En tu próximo lanzamiento, si sacás un número de Fibonacci (1, 2, 3, 5, 8 o 13), sumás el número anterior de la secuencia. Los demás resultados no cambian.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::FibonacciBonus, EffectScope::NextRoll, 1, 0, 0, -1, false, 0, 0 },

        { "No tan rápido +++",
          "En el próximo lanzamiento de tu rival, cualquier resultado mayor a 15 se convierte en 15.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::MaxResult, EffectScope::NextCombatRoll, 15, 0, 0, -1, true, 0, 0 },

        { "No tan rápido",
          "En el próximo lanzamiento de tu rival, restale 3 al resultado final.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -3, 0, 0, -1, true, 0, 0 },

        { "Remordimiento",
          "Robá una Bendición al azar de otro jugador.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::HandAction, EffectScope::OnAcquire, 0, 0, 0, -1, false, 0, 0 },

        { "Yang y Ying",
          "En tu próximo combate, sumás 1 a todas tus tiradas. En el combate siguiente, restás 3 a todas tus tiradas.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, 1, -3, 0, -1, false, 0, 0 },

        { "Condena",
          "A partir de ahora, cada rival contra el que combatas recibe +1 a todas sus tiradas durante ese combate.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::Permanent, 1, 0, 0, -1, true, 0, 0 },

        { "Condena +",
          "A partir de ahora, cada rival contra el que combatas recibe +2 a todas sus tiradas durante ese combate.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::Permanent, 2, 0, 0, -1, true, 0, 0 },

        { "Condena ++",
          "A partir de ahora, cada rival contra el que combatas recibe +3 a todas sus tiradas durante ese combate.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::Permanent, 3, 0, 0, -1, true, 0, 0 },

        { "Ying y Yang ++",
          "En tu próximo combate, restás 5 a todas tus tiradas. En el combate siguiente, sumás 10 a todas tus tiradas.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, -5, 10, 0, -1, false, 0, 0 },

        { "Twisted ++",
          "Durante tu próximo combate, si tu rival usa una Bendición que suma un número fijo a una tirada, esa suma no tiene efecto.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::TwistRival, EffectScope::NextCombat, 1, 0, 0, -1, true, 0, 0 },

        { "Dado Cambiado 5 + +",
          "En tu próximo lanzamiento, tenés 5 tiradas en total. Convertí cada resultado del D20 a un valor de 1 a 5: 1-4 = 1, 5-8 = 2, 9-12 = 3, 13-16 = 4 y 17-20 = 5. Elegí uno de los cinco resultados.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 5, 5, -1, false, 0, 0 },

        { "Fortuna +",
          "A partir de ahora, en cada lanzamiento tirás 3 veces el D20 y elegís uno de los resultados.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::DiceControl, EffectScope::Permanent, 0, 3, 0, -1, false, 0, 0 },

        { "Dos caminos + +",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 10. Si es par, restás 10.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 10, -10, -1, false, 0, 0 },

        { "Dos caminos + +",
          "En tu próximo lanzamiento, si el resultado es par, sumás 10. Si es impar, restás 10.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 10, -10, -1, false, 0, 0 },

        { "Fibonacci Riesgoso",
          "En tu próximo lanzamiento, si sacás un número de Fibonacci (1, 2, 3, 5, 8 o 13), sumás los dos números anteriores de la secuencia. Si no es Fibonacci, restás el número de Fibonacci anterior más cercano.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::FibonacciBonus, EffectScope::NextRoll, 2, 0, 0, -1, false, 0, 0 },

        { "No tan rápido ++",
          "En el próximo lanzamiento de tu rival, restale 10 al resultado final.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -10, 0, 0, -1, true, 0, 0 },

        { "Yang y Ying --",
          "En tu próximo combate, sumás 5 a todas tus tiradas. En el combate siguiente, restás 10 a todas tus tiradas.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, 5, -10, 0, -1, false, 0, 0 },

        { "Dado Cambiado 5 - -",
          "En tu próximo lanzamiento, tenés 2 tiradas en total. Convertí cada resultado del D20 a 1-5: 1-4 = 1, 5-8 = 2, 9-12 = 3, 13-16 = 4 y 17-20 = 5. Elegí uno de los dos resultados.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 2, 5, -1, false, 0, 0 },

        { "Mala suerte",
          "En tu próximo combate, tu cantidad total de tiradas se reduce en 1. Nunca podés quedar con menos de 1 tirada.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::CombatRolls, EffectScope::NextCombat, -1, 0, 0, -1, false, 0, 0 },

        { "Dividir -",
          "En tu próximo lanzamiento, dividí el resultado entre 2 y redondeá hacia arriba. Este efecto dura tus próximos 2 turnos.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::DivideRoundUp, EffectScope::NextRoll, 0, 0, 0, -1, false, 2, 0 },

        { "Ying y Yang +",
          "En tu próximo combate, restás 3 a todas tus tiradas. En el combate siguiente, sumás 5 a todas tus tiradas.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, -3, 5, 0, -1, false, 0, 0 },

        { "Twisted +",
          "En tu próximo combate, si tu rival usa una Bendición que multiplica una tirada, el resultado se divide en lugar de multiplicarse.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::TwistRival, EffectScope::NextCombat, 2, 0, 0, -1, true, 0, 0 },

        { "Dado Cambiado 2 +",
          "En tu próximo lanzamiento, tenés 3 tiradas en total. Convertí cada resultado del D20 a 1 o 2: 1-10 = 1 y 11-20 = 2. Elegí uno de los tres resultados.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 3, 2, -1, false, 0, 0 },

        { "Dado Cambiado 5 +",
          "En tu próximo lanzamiento, tenés 4 tiradas en total. Convertí cada resultado del D20 a 1-5: 1-4 = 1, 5-8 = 2, 9-12 = 3, 13-16 = 4 y 17-20 = 5. Elegí uno de los cuatro resultados.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 4, 5, -1, false, 0, 0 },

        { "Fortuna",
          "A partir de ahora, en cada lanzamiento tirás 2 veces el D20 y elegís uno de los resultados.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::Permanent, 0, 2, 0, -1, false, 0, 0 },

        { "Multiplicar +",
          "En tu próximo lanzamiento, duplicás el resultado. El resultado final no puede superar 25.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::MultiplyCapped, EffectScope::NextRoll, 2, 25, 0, -1, false, 0, 0 },

        { "Multiplicar ++",
          "En tu próximo lanzamiento, duplicás el resultado. El resultado final no puede superar 30.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::MultiplyCapped, EffectScope::NextRoll, 2, 30, 0, -1, false, 0, 0 },

        { "El apostador",
          "En tu próximo lanzamiento, tirás el D20 una segunda vez y lanzás una moneda. Si sale cara, dividís el segundo resultado entre 2 y redondeás hacia arriba. Si sale cruz, duplicás el segundo resultado.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextRoll, 4, 0, 0, -1, false, 0, 0 },

        { "Dos caminos +",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 5. Si es par, restás 5.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 5, -5, -1, false, 0, 0 },

        { "Dos caminos +",
          "En tu próximo lanzamiento, si el resultado es par, sumás 5. Si es impar, restás 5.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 5, -5, -1, false, 0, 0 },

        { "No tan rápido +",
          "En el próximo lanzamiento de tu rival, restale 5 al resultado final.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -5, 0, 0, -1, true, 0, 0 },

        { "Remordimiento",
          "Robá una Bendición específica que elijas de otro jugador.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::HandAction, EffectScope::OnAcquire, 1, 0, 0, -1, false, 0, 0 },

        { "Yang y Ying -",
          "En tu próximo combate, restás 5 a todas tus tiradas. En el combate siguiente, sumás 10 a todas tus tiradas.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::TwoPhaseFlat, EffectScope::NextTwoCombats, -5, 10, 0, -1, false, 0, 0 },

        { "Dado Cambiado 5 -",
          "En tu próximo lanzamiento, tenés 2 tiradas en total. Convertí cada resultado del D20 a 1-5: 1-4 = 1, 5-8 = 2, 9-12 = 3, 13-16 = 4 y 17-20 = 5. Elegí uno de los dos resultados.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 2, 5, -1, false, 0, 0 },

        { "Dado Cambiado 2 -",
          "En tu próximo lanzamiento, tenés 2 tiradas en total. Convertí cada resultado del D20 a 1 o 2: 1-10 = 1 y 11-20 = 2. Elegí uno de los dos resultados.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextRoll, 1, 2, 2, -1, false, 0, 0 },

        { "Dividir",
          "En tu próximo lanzamiento, dividí el resultado entre 2 y redondeá hacia arriba.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DivideRoundUp, EffectScope::NextRoll, 0, 0, 0, -1, false, 0, 0 },

        { "Impulso",
          "En tu próximo lanzamiento, sumás 2 al resultado final.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextRoll, 2, 0, 0, -1, false, 0, 0 },

        { "Escudo",
          "En tu próximo combate, la primera penalización que recibas se reduce en 3.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::ModifyIncoming, EffectScope::NextCombat, 0, 3, 0, -1, false, 0, 0 },

        { "Revancha",
          "Si perdés tu próximo combate, podés repetir una de las 3 tiradas de ese combate. Debés conservar el nuevo resultado.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::PlayerChoice, EffectScope::NextCombat, 4, 0, 0, -1, false, 0, 0 },

        { "Paso firme",
          "En tu próximo lanzamiento, si sacás entre 1 y 9, sumás 4. Si sacás 10 o más, el resultado no cambia.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 1, 9, 4, -1, false, 0, 0 },

        { "Apuesta controlada",
          "Antes de tu próximo lanzamiento, elegí una opción: si sacás 10 o menos, sumás 5; si sacás 11 o más, restás 2.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::PlayerChoice, EffectScope::NextRoll, 0, 10, 5, -1, false, 0, 0 },

        { "Reserva",
          "En tu próximo combate, una vez podés guardar el resultado de una de tus tiradas y usar exactamente ese mismo resultado en tu siguiente tirada.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::PlayerChoice, EffectScope::NextCombat, 3, 0, 0, -1, false, 1, 0 },

        { "Destino alterado",
          "Una vez durante tu próximo combate, después de tirar, podés sumar 4 o restar 4 a tu resultado.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::PlayerChoice, EffectScope::NextCombat, 2, 4, 0, -1, false, 1, 0 },

        { "Golpe perfecto",
          "En tu próximo combate, si sacás 18, 19 o 20 en una tirada, sumás 3 al resultado.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ThresholdBonus, EffectScope::NextCombat, 18, 20, 3, -1, false, 0, 0 },

        { "Doble o nada",
          "En tu próxima tirada de combate, tirás 2 D20 y elegís el resultado que prefieras.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::DiceControl, EffectScope::NextCombatRoll, 0, 2, 0, -1, false, 0, 0 },

        { "Negación",
          "Una vez durante tu próximo combate, podés anular una Bendición que tu rival haya usado en esa misma tirada.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::TwistRival, EffectScope::NextCombat, 3, 0, 0, -1, true, 1, 0 },

        { "Última oportunidad",
          "Si después de las primeras 2 tiradas de tu próximo combate vas perdiendo, sumás 6 a tu tercera tirada.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::HistoryBonus, EffectScope::NextCombat, 2, 2, 6, 2, false, 0, 0 },

        { "Predicción",
          "Antes de tu próximo lanzamiento, elegí impar o par. Si acertás, sumás 7. Si fallás, restás 2.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::PlayerChoice, EffectScope::NextRoll, 1, 7, -2, -1, false, 0, 0 },

        { "Cadena de victorias",
          "Si ganás una tirada durante tu próximo combate, sumás 3 a tu siguiente tirada.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::HistoryBonus, EffectScope::NextCombat, 1, 0, 3, -1, false, 0, 0 },

        { "Descanso",
          "Al obtener esta carta, podés descartar una Maldición Común que tengas.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::HandAction, EffectScope::OnAcquire, 2, 0, 0, -1, false, 0, 0 },

        { "Fatiga",
          "En tu próximo combate, restás 2 a tu primera tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, 0, false, 0, 0 },

        { "Tropiezo",
          "En tu próximo lanzamiento, el resultado final no puede ser mayor que 15.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::MaxResult, EffectScope::NextRoll, 15, 0, 0, -1, false, 0, 0 },

        { "Duda",
          "En tu próximo lanzamiento, si el resultado es par, restás 3.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, -3, 0, -1, false, 0, 0 },

        { "Peso muerto",
          "En tu próximo combate, la primera vez que uses una Bendición, restás 2 a esa misma tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::BlessingPenalty, EffectScope::NextCombat, 0, 0, -2, -1, false, 0, 0 },

        { "Desgaste",
          "En tu próximo combate, restás 2 a tu segunda tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, 1, false, 0, 0 },

        { "Piedra en el camino",
          "En tu próximo lanzamiento de exploración, restás 3 al resultado.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextExplorationRoll, -3, 0, 0, -1, false, 0, 0 },

        { "Mal paso",
          "En tu próximo lanzamiento, si sacás 5 o menos, restás 3 adicionales.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 1, 5, -3, -1, false, 0, 0 },

        { "Pulso inestable",
          "En tu próximo lanzamiento, si sacás 16 o más, restás 4.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 16, 20, -4, -1, false, 0, 0 },

        { "Desánimo",
          "En tu próximo combate, si perdés la primera tirada, restás 2 a tu segunda tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::HistoryBonus, EffectScope::NextCombat, 0, 0, -2, 1, false, 0, 0 },

        { "Talón de Aquiles",
          "En tu próximo combate, si sacás exactamente 10, ese resultado se convierte en 5.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ValueConvert, EffectScope::NextCombat, 10, 5, 0, -1, false, 0, 0 },

        { "En empate",
          "Durante tu próximo combate, si una de tus tiradas empata con la de tu rival, esa tirada cuenta como perdida para vos.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::WinRule, EffectScope::NextCombat, 3, 0, 0, -1, false, 0, 0 },

        { "Cansancio",
          "En tu próxima ronda de exploración, el primer lanzamiento que realices recibe -2.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextExplorationRoll, -2, 0, 0, -1, false, 0, 0 },

        { "Herida abierta",
          "En tu próximo combate, restás 4 a tu primera tirada.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombat, -4, 0, 0, 0, false, 0, 0 },

        { "Agotamiento",
          "En tu próximo combate, no podés usar efectos que te otorguen tiradas adicionales.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextCombat, 2, 0, 0, -1, false, 0, 0 },

        { "Lastre",
          "En tu próximo combate, restás 2 a todas tus tiradas.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, -1, false, 0, 0 },

        { "Temor",
          "En tu próximo combate, cada resultado de 15 o más se convierte en 14.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::MaxResult, EffectScope::NextCombat, 14, 0, 0, -1, false, 0, 0 },

        { "Desconcentración",
          "En tu próximo combate, si usás 2 o más Bendiciones, restás 5 a tu tercera tirada.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::BlessingPenalty, EffectScope::NextCombat, 2, 2, -5, 2, false, 0, 0 },

        { "Bloqueo",
          "En tu próximo combate, no podés usar la misma Bendición más de una vez.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::BlessingRestriction, EffectScope::NextCombat, 2, 0, 0, -1, false, 0, 0 },

        { "Mala racha",
          "En tu próximo combate, cada vez que saques 18, 19 o 20, debés volver a tirar y quedarte con el segundo resultado.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DiceControl, EffectScope::NextCombat, 3, 18, 0, -1, false, 0, 0 },

        { "Inercia",
          "En tu próximo combate, no podés modificar el resultado de tu primera tirada usando Bendiciones.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::BlessingRestriction, EffectScope::NextCombat, 1, 0, 0, 0, false, 0, 0 },

        { "Carga",
          "Si tenés 3 o más Bendiciones al recibir esta carta, restás 3 a todas tus tiradas del próximo combate.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombat, -3, 3, 0, -1, false, 0, 1 },

        { "Riesgo calculado",
          "En tu próximo combate, la primera Bendición que uses también hace que restes 2 a esa misma tirada.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::BlessingPenalty, EffectScope::NextCombat, 0, 0, -2, -1, false, 0, 0 },

        { "Fractura",
          "Después de usar una Bendición en tu próximo combate, restás 2 a tu siguiente tirada.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::BlessingPenalty, EffectScope::NextCombat, 1, 0, -2, -1, false, 0, 0 },

        { "Marcha pesada",
          "En tu próxima ronda de exploración, restás 2 a todos tus lanzamientos.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextExplorationRoll, -2, 0, 0, -1, false, 0, 0 },

        { "Tormenta",
          "En tu próximo combate, tu primera tirada no puede superar 10 después de aplicar todos los efectos.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::MaxResult, EffectScope::NextCombat, 10, 0, 0, 0, false, 0, 0 },

        { "Ancla",
          "En tu próximo combate, no podés obtener tiradas adicionales, sin importar de dónde provengan.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::DiceControl, EffectScope::NextCombat, 2, 0, 0, -1, false, 0, 0 },

        { "Maldición del veinte",
          "En tu próximo combate, cada vez que saques 20, ese resultado se convierte en 1.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::ValueConvert, EffectScope::NextCombat, 20, 1, 0, -1, false, 0, 0 },

        { "Reverso del destino",
          "En la primera tirada de tu próximo combate, gana quien obtenga el número más bajo.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::WinRule, EffectScope::NextCombat, 0, 0, 0, 0, false, 0, 0 },

        { "Impuesto",
          "En tu próximo combate, restás 3 al resultado final de cada una de tus 3 tiradas.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::FlatBonus, EffectScope::NextCombat, -3, 0, 0, -1, false, 0, 0 },

        { "Cadenas",
          "En tu próximo combate, solo podés usar 1 Bendición por tirada.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::BlessingRestriction, EffectScope::NextCombat, 0, 1, 0, -1, false, 0, 0 },

        { "Despojo",
          "Antes de tu próximo combate, tu rival elige una de tus Bendiciones. No podés usarla durante ese combate.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::BlessingRestriction, EffectScope::NextCombat, 3, 0, 0, -1, false, 0, 0 },

        { "Eco negativo",
          "En tu próximo combate, todo bonificador positivo que recibas se reduce a la mitad y se redondea hacia abajo.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::ModifyIncoming, EffectScope::NextCombat, 1, 0, 0, -1, false, 0, 0 },

        { "Punto muerto",
          "En tu próximo combate, tu tercera tirada no puede superar 8 después de aplicar todos los efectos.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::MaxResult, EffectScope::NextCombat, 8, 0, 0, 2, false, 0, 0 },

        { "Marea inversa",
          "En tu próximo combate, si después de las primeras 2 tiradas vas ganando, restás 5 a tu tercera tirada.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::HistoryBonus, EffectScope::NextCombat, 3, 2, -5, 2, false, 0, 0 },

        { "Corona rota",
          "Si ganás tu próximo combate, no podés robar una Bendición ni entregar una Maldición como recompensa.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::NoRewardOnWin, EffectScope::NextCombat, 0, 0, 0, -1, false, 0, 0 },

        { "Fractura del destino",
          "En tu próximo combate, la primera vez que modifiques una tirada usando una Bendición, restás 3 al resultado final.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::BlessingPenalty, EffectScope::NextCombat, 0, 0, -3, -1, false, 0, 0 },
    };

    static constexpr int CARD_CATALOG_COUNT =
        static_cast<int>(sizeof(CARD_CATALOG) / sizeof(CARD_CATALOG[0]));

    int GetCardDefCount()
    {
        return CARD_CATALOG_COUNT;
    }

    const CardDef& GetCardDef(int defId)
    {
        if (defId < 0 || defId >= CARD_CATALOG_COUNT) return CARD_CATALOG[0];
        return CARD_CATALOG[defId];
    }

    int CountCards(const Player& player, CardKind kind)
    {
        int count = 0;
        for (const CardInstance& card : player.cards)
        {
            if (GetCardDef(card.defId).kind == kind) ++count;
        }
        return count;
    }

    int CountPendingCards(const Player& player)
    {
        int count = 0;
        for (const CardInstance& card : player.cards)
        {
            if (!card.spent) ++count;
        }
        return count;
    }

    static Deck& GetDeck(DeckSet& set, CardKind kind, CardLevel level)
    {
        return set.decks[static_cast<int>(kind)][static_cast<int>(level)];
    }

    void ShuffleDeck(Deck& deck)
    {
        for (int i = static_cast<int>(deck.available.size()) - 1; i > 0; --i)
        {
            int j = RandomInt(0, i);
            int temp = deck.available[i];
            deck.available[i] = deck.available[j];
            deck.available[j] = temp;
        }
    }

    void InitDeckSet(DeckSet& set)
    {
        for (int kindIndex = 0; kindIndex < 2; ++kindIndex)
        {
            for (int levelIndex = 0; levelIndex < 3; ++levelIndex)
            {
                Deck& deck = set.decks[kindIndex][levelIndex];
                deck.available.clear();
                deck.taken.clear();
            }
        }

        for (int defId = 0; defId < CARD_CATALOG_COUNT; ++defId)
        {
            const CardDef& def = CARD_CATALOG[defId];
            GetDeck(set, def.kind, def.level).available.push_back(defId);
        }

        for (int kindIndex = 0; kindIndex < 2; ++kindIndex)
        {
            for (int levelIndex = 0; levelIndex < 3; ++levelIndex)
            {
                ShuffleDeck(set.decks[kindIndex][levelIndex]);
            }
        }
    }

    int DrawCardOptions(DeckSet& set, CardKind kind, CardLevel level, int options[])
    {
        Deck& deck = GetDeck(set, kind, level);

        if (static_cast<int>(deck.available.size()) < CARD_OPTIONS)
        {
            for (int defId : deck.taken) deck.available.push_back(defId);
            deck.taken.clear();
            ShuffleDeck(deck);
        }

        int offered = 0;
        while (offered < CARD_OPTIONS && !deck.available.empty())
        {
            options[offered] = deck.available.back();
            deck.available.pop_back();
            ++offered;
        }

        return offered;
    }

    void ReturnCardOptions(DeckSet& set, CardKind kind, CardLevel level,
                           const int options[], int optionCount, int chosenIndex)
    {
        Deck& deck = GetDeck(set, kind, level);

        for (int i = 0; i < optionCount; ++i)
        {
            if (i == chosenIndex) deck.taken.push_back(options[i]);
            else                  deck.available.push_back(options[i]);
        }

        ShuffleDeck(deck);
    }

    CardLevel GetCardLevelForPath(PathKind path)
    {
        switch (path)
        {
            case PathKind::Easy:   return CardLevel::Weak;
            case PathKind::Normal: return CardLevel::Normal;
            case PathKind::Hard:   return CardLevel::Epic;
        }
        return CardLevel::Weak;
    }
}
