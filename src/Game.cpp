#include "Game.h"
#include "Audio.h"
#include "Board.h"
#include "Cpu.h"
#include "Enemy.h"
#include "Fx.h"
#include "Theme.h"
#include "Dice.h"
#include "Ui.h"

namespace ReyesCuadra
{
    // El tablero es vertical y ocupa la mitad izquierda, así que los paneles
    // viven en una columna a la derecha.
    constexpr float PANEL_X = 620.0f;
    constexpr float PANEL_W = 620.0f;
    constexpr float PANEL_Y = 88.0f;
    constexpr float PANEL_H = 288.0f;

    constexpr float LIST_Y = 390.0f;
    constexpr float LIST_H = 280.0f;

    constexpr float OVERLAY_X = 260.0f;
    constexpr float OVERLAY_Y = 200.0f;
    constexpr float OVERLAY_W = 760.0f;
    constexpr float OVERLAY_H = 300.0f;

    static const Color PANEL_COLOR    = Color{ 24, 34, 60, 242 };
    static const Color PANEL_BORDER   = Color{ 86, 60, 102, 255 };
    static const Color BLESSING_COLOR = COL_BLESSING;
    static const Color CURSE_COLOR    = COL_CURSE;

    constexpr float REVEAL_TIME    = 0.55f;
    constexpr double CPU_THINK_TIME = 0.5;

    static const char* CREDITS_LINE =
        "Federico Fernandez Soto   ·   Salvador Rosafioriti   ·   Valentin Reyes";

    static void ShowBanner(GameState& game, const char* text, Color color)
    {
        game.bannerText  = text;
        game.bannerColor = color;
        game.bannerTime  = GetTime() + REVEAL_TIME;
    }

    static void AddLog(GameState& game, const char* text)
    {
        if (text == nullptr || text[0] == '\0') return;

        for (int i = 0; i < LOG_LINES - 1; ++i) game.log[i] = game.log[i + 1];
        game.log[LOG_LINES - 1] = text;
    }

    static void StartMatch(GameState& game)
    {
        static const Color PLAYER_COLORS[PLAYER_COUNT] =
        {
            Color{ 240, 200,  70, 255 },
            Color{ 235, 110,  80, 255 },
            Color{  95, 170, 235, 255 },
            Color{ 150, 220, 130, 255 }
        };

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            Player& player = game.players[i];

            player.id             = i;
            player.color          = PLAYER_COLORS[i];
            player.path           = PathKind::Normal;
            player.stage          = 0;
            player.initiativeRoll = 0;
            player.isCpu          = (i >= game.humanCount);
            player.visualStage    = 0.0f;
            player.visualLane     = static_cast<float>(static_cast<int>(PathKind::Normal));
            player.cards.clear();
            player.items.clear();
            player.hasMap       = false;
            player.extraWounds  = 0;
            player.bossBeaten   = false;
            player.bossParts    = 0;
            player.bossTurnsLeft = 0;

            // El humano juega con el personaje que eligió en el menú; la CPU
            // toma uno al azar para que no sean siempre iguales.
            player.character = player.isCpu
                             ? static_cast<CharacterId>(RandomInt(0, static_cast<int>(CharacterId::Count) - 1))
                             : game.menuCharacter;
        }

        InitDeckSet(game.deckSet);

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            Player& player = game.players[i];
            const CharacterDef& def = GetCharacterDef(player.character);

            for (int n = 0; n < def.startItems; ++n)
            {
                GiveItem(player, RollTreasureItem(player));
            }

            if (def.startCard)
            {
                int options[CARD_OPTIONS] = {};
                int count = DrawCardOptions(game.deckSet, CardKind::Blessing, CardLevel::Weak, options);

                if (count > 0)
                {
                    CardInstance card;
                    card.defId = options[0];

                    const CardDef& cardDef = GetCardDef(card.defId);
                    if (cardDef.scope == EffectScope::NextRoll ||
                        cardDef.scope == EffectScope::NextExplorationRoll ||
                        cardDef.scope == EffectScope::NextCombatRoll)
                    {
                        card.rollsLeft = (cardDef.uses > 0) ? cardDef.uses : 1;
                    }

                    player.cards.push_back(card);
                    ReturnCardOptions(game.deckSet, CardKind::Blessing, CardLevel::Weak, options, count, 0);
                }
            }
        }

        game.currentPlayer   = 0;
        game.currentStage    = 0;
        game.turnIndex       = 0;
        game.pathPreview     = PathKind::Normal;
        game.showCards       = false;
        game.cardsShown      = 0;

        for (int i = 0; i < PLAYER_COUNT; ++i) game.turnOrder[i] = i;
        game.eventDifficulty = 0;
        game.eventSuccess    = false;
        game.eventEffects.clear();
        game.eventSession    = RollSession{};
        game.cardOptionCount = 0;
        game.rewardBump      = 0;
        game.rollBonus       = 0;
        game.kingId          = -1;

        game.currentTile = TileKind::Event;
        game.tileNote.clear();

        game.enemy       = Enemy{};
        game.enemyMemory = CombatMemory{};
        game.enemyTarget = 0;
        game.enemyWon    = false;
        game.enemyNote.clear();

        for (int i = 0; i < LOG_LINES; ++i) game.log[i].clear();

        PlayTrack(MusicTrack::Explore);

        game.cpuSignature  = -1;
        game.cpuNextAction = 0.0;

        AddLog(game, TextFormat("Comienza la partida: vos y %i rivales de la CPU.",
                                PLAYER_COUNT - game.humanCount));
    }

    // El manual del juego de mesa: gana el dado más alto y sigue el de su
    // derecha. Eso define el orden de toda la exploración.
    static void RollTurnOrder(GameState& game)
    {
        int best = 0;

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            game.players[i].initiativeRoll = RollRawD20();

            if (game.players[i].initiativeRoll > game.players[best].initiativeRoll ||
                (game.players[i].initiativeRoll == game.players[best].initiativeRoll && RandomInt(0, 1) == 1))
            {
                best = i;
            }
        }

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            game.turnOrder[i] = (best + i) % PLAYER_COUNT;
        }

        game.turnIndex    = 0;
        game.currentPlayer = game.turnOrder[0];
        game.pathPreview   = game.players[game.currentPlayer].path;

        AddLog(game, TextFormat("Orden de turnos: J%i(%i)  J%i  J%i  J%i",
                                game.turnOrder[0] + 1, game.players[game.turnOrder[0]].initiativeRoll,
                                game.turnOrder[1] + 1, game.turnOrder[2] + 1, game.turnOrder[3] + 1));
    }

    static void FinishEventRoll(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        game.eventRoll    = game.eventSession.roll;
        game.eventEffects = game.eventSession.appliedNames;

        if (game.eventRoll.criticalHit)       game.eventSuccess = true;
        else if (game.eventRoll.criticalFail) game.eventSuccess = false;
        else                                  game.eventSuccess = (game.eventRoll.total >= game.eventDifficulty);

        game.revealUntil = GetTime() + REVEAL_TIME;
        game.resultTime  = game.revealUntil;

        ShowBanner(game, game.eventSuccess ? "¡ÉXITO!" : "FALLO",
                   game.eventSuccess ? COL_BLESSING : COL_CURSE);

        if (game.eventRoll.criticalHit)       PlaySfxDelayed(SoundId::Crit, 0.55f);
        else if (game.eventSuccess)           PlaySfxDelayed(SoundId::Success, 0.55f);
        else                                  PlaySfxDelayed(SoundId::Fail, 0.55f);

        AddLog(game, TextFormat("Jugador %i%s (%s): D20 %i -> %i vs %i -> %s",
                                game.currentPlayer + 1,
                                player.isCpu ? " [CPU]" : "",
                                GetPathName(player.path),
                                game.eventRoll.natural,
                                game.eventRoll.total,
                                game.eventDifficulty,
                                game.eventSuccess ? "ÉXITO" : "FALLO"));

        game.phase = GamePhase::EventResult;
    }

    static void StartEventRoll(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        RollRequest request;
        request.isCombat  = false;
        request.rollIndex = -1;

        game.eventDifficulty = GetEventDifficulty(player.path, game.currentStage);

        PlaySfx(SoundId::Dice);
        BeginRoll(game.eventSession, player, nullptr, request, nullptr);

        if (game.eventSession.choice.active) game.phase = GamePhase::EventChoice;
        else if (game.eventSession.done)     FinishEventRoll(game);
    }

    static void AdvanceEventChoice(GameState& game, int option)
    {
        PlaySfx(SoundId::Select);
        AdvanceRoll(game.eventSession, option);

        if (game.eventSession.choice.active) return;
        if (game.eventSession.done) FinishEventRoll(game);
    }

    // La corona ya no sale de un torneo entre jugadores: se la lleva quien
    // mejor le fue al jefe de la cima. Si nadie le ganó, no hay rey.
    static void CrownTheKing(GameState& game)
    {
        game.kingId = -1;
        int bestScore = -1;

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[i];
            if (!player.bossBeaten) continue;

            int score = player.bossParts * 100
                      + player.bossTurnsLeft * 10
                      + (PLAYER_COUNT - i);

            if (score > bestScore) { bestScore = score; game.kingId = i; }
        }

        StopTrack();

        if (game.kingId >= 0)
        {
            PlaySfx(SoundId::Victory);
            AddLog(game, TextFormat("Jugador %i se corona rey de la colina.", game.kingId + 1));
        }
        else
        {
            PlaySfx(SoundId::Fail);
            AddLog(game, "Nadie pudo con el jefe: la colina se queda sin rey.");
        }

        game.phase = GamePhase::Victory;
    }

    static void AdvanceTurn(GameState& game)
    {
        ++game.players[game.currentPlayer].stage;
        ++game.turnIndex;

        if (game.turnIndex >= PLAYER_COUNT)
        {
            game.turnIndex = 0;
            ++game.currentStage;
        }

        game.currentPlayer = game.turnOrder[game.turnIndex];
        game.pathPreview   = game.players[game.currentPlayer].path;
        game.rollBonus     = 0;

        PlayTrack(MusicTrack::Explore);

        if (game.currentStage >= STAGE_COUNT)
        {
            CrownTheKing(game);
            return;
        }

        game.phase = GamePhase::PathSelection;
    }

    // Saca de la mano la peor maldición que todavía no se usó. La comparten la
    // casilla de descanso y la poción.
    static bool ClearWorstCurse(Player& player, std::string& removedName)
    {
        int best      = -1;
        int bestValue = -1;

        for (int i = 0; i < static_cast<int>(player.cards.size()); ++i)
        {
            const CardInstance& card = player.cards[i];
            if (card.spent) continue;

            const CardDef& def = GetCardDef(card.defId);
            if (def.kind != CardKind::Curse) continue;

            int value = static_cast<int>(def.level);
            if (value > bestValue) { bestValue = value; best = i; }
        }

        if (best < 0) return false;

        removedName = GetCardDef(player.cards[best].defId).name;
        player.cards.erase(player.cards.begin() + best);
        return true;
    }

    static void OfferCards(GameState& game)
    {
        const Player& player = game.players[game.currentPlayer];

        game.offerKind  = game.eventSuccess ? CardKind::Blessing : CardKind::Curse;

        int level = static_cast<int>(GetCardLevelForPath(player.path)) + game.rewardBump;
        if (level < 0) level = 0;
        if (level > static_cast<int>(CardLevel::Epic)) level = static_cast<int>(CardLevel::Epic);

        game.offerLevel = static_cast<CardLevel>(level);
        game.rewardBump = 0;

        game.cardOptionCount = DrawCardOptions(game.deckSet, game.offerKind,
                                               game.offerLevel, game.cardOptions);

        game.phase = GamePhase::CardSelection;
    }

    // Una carta sin posibilidad de elegir: se usa para el contraataque del
    // enemigo, que según la propuesta no deja elegir entre tres.
    static void GiveForcedCard(GameState& game, CardKind kind, CardLevel level)
    {
        Player& player = game.players[game.currentPlayer];

        int options[CARD_OPTIONS] = {};
        int count = DrawCardOptions(game.deckSet, kind, level, options);
        if (count <= 0) return;

        int defId = options[0];
        const CardDef& def = GetCardDef(defId);

        CardInstance card;
        card.defId = defId;

        bool rollScoped = (def.scope == EffectScope::NextRoll ||
                           def.scope == EffectScope::NextExplorationRoll ||
                           def.scope == EffectScope::NextCombatRoll);

        if (rollScoped) card.rollsLeft = (def.uses > 0) ? def.uses : 1;

        if (def.condition == 1 && CountCards(player, CardKind::Blessing) < def.paramB)
        {
            card.spent = true;
        }

        player.cards.push_back(card);
        ReturnCardOptions(game.deckSet, kind, level, options, count, 0);

        std::string message;
        ApplyOnAcquire(player, game.players, PLAYER_COUNT, defId, message);
        AddLog(game, message.c_str());

        game.enemyNote = TextFormat("El enemigo contraataca: %s", def.name);
        AddLog(game, TextFormat("Jugador %i recibe %s", game.currentPlayer + 1, def.name));
    }

    static void TakeCard(GameState& game, int chosenIndex)
    {
        Player& player = game.players[game.currentPlayer];

        int defId = game.cardOptions[chosenIndex];
        const CardDef& def = GetCardDef(defId);

        CardInstance card;
        card.defId = defId;

        bool rollScoped = (def.scope == EffectScope::NextRoll ||
                           def.scope == EffectScope::NextExplorationRoll ||
                           def.scope == EffectScope::NextCombatRoll);

        if (rollScoped) card.rollsLeft = (def.uses > 0) ? def.uses : 1;

        if (def.condition == 1 && CountCards(player, CardKind::Blessing) < def.paramB)
        {
            card.spent = true;
        }

        player.cards.push_back(card);

        ReturnCardOptions(game.deckSet, game.offerKind, game.offerLevel,
                          game.cardOptions, game.cardOptionCount, chosenIndex);

        PlaySfx(SoundId::Card);
        game.bannerText  = TextFormat("+ %s", def.name);
        game.bannerColor = (def.kind == CardKind::Blessing) ? COL_BLESSING : COL_CURSE;
        game.bannerTime  = GetTime();
        AddLog(game, TextFormat("Jugador %i toma: %s", game.currentPlayer + 1, def.name));

        std::string message;
        ApplyOnAcquire(player, game.players, PLAYER_COUNT, defId, message);
        AddLog(game, message.c_str());

        AdvanceTurn(game);
    }

    // --------------------------------------------- casillas que no son combate

    static void ResolveTreasure(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        ItemId id = RollTreasureItem(player);

        if (GiveItem(player, id))
        {
            game.tileNote = TextFormat("Encontraste: %s", GetItemDef(id).name);
        }
        else
        {
            game.tileNote = "La mochila está llena: el cofre queda donde estaba.";
        }

        PlaySfx(SoundId::Card);
        ShowBanner(game, "TESORO", COL_GOLD);
        AddLog(game, TextFormat("Jugador %i abre un cofre. %s",
                                game.currentPlayer + 1, game.tileNote.c_str()));

        game.phase = GamePhase::TreasureResult;
    }

    static void ResolveRest(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        std::string removed;

        if (ClearWorstCurse(player, removed))
        {
            game.tileNote = TextFormat("Te sacaste de encima: %s", removed.c_str());
            ShowBanner(game, "DESCANSO", COL_BLESSING);
        }
        else
        {
            game.tileNote = "No tenías maldiciones pendientes: descansás y seguís.";
            ShowBanner(game, "DESCANSO", COL_CREAM);
        }

        PlaySfx(SoundId::Select);
        AddLog(game, TextFormat("Jugador %i descansa. %s",
                                game.currentPlayer + 1, game.tileNote.c_str()));

        game.phase = GamePhase::RestResult;
    }

    // ------------------------------------------------------- combate contra enemigos

    static void StartEnemyFight(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        game.enemy       = MakeEnemy(game.currentTile);
        game.enemyTarget = FirstAlivePart(game.enemy);
        game.enemyWon    = false;
        game.enemyNote.clear();

        // Los combates contra enemigos cuentan como combate para las cartas:
        // sin esto, todas las que dicen "en tu próximo combate" no se usarían
        // nunca, porque ya no hay torneo entre jugadores.
        StartCombatEffects(player, player, game.enemyMemory);
        PlayTrack(MusicTrack::Combat);

        AddLog(game, TextFormat("Jugador %i%s se enfrenta a %s",
                                game.currentPlayer + 1,
                                player.isCpu ? " [CPU]" : "",
                                game.enemy.name));

        game.phase = GamePhase::EnemyIntro;
    }

    static void FinishEnemyFight(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        game.enemyWon     = EnemyBeaten(game.enemy);
        game.eventSuccess = game.enemyWon;
        game.rewardBump   = (game.enemyWon && BonusPartDown(game.enemy)) ? 1 : 0;

        EndCombatEffects(player);

        if (game.enemy.rank == EnemyRank::Boss)
        {
            player.bossBeaten    = game.enemyWon;
            player.bossParts     = CountDownedCounting(game.enemy) + (BonusPartDown(game.enemy) ? 1 : 0);
            player.bossTurnsLeft = game.enemy.turnsLeft;
        }

        if (game.enemyWon)
        {
            ShowBanner(game, "ENEMIGO DERROTADO", COL_GOLD);
            PlaySfxDelayed(SoundId::Success, 0.3f);
        }
        else
        {
            ShowBanner(game, "TE QUEDASTE SIN TURNOS", COL_CURSE);
            PlaySfxDelayed(SoundId::Fail, 0.3f);
        }

        AddLog(game, TextFormat("Jugador %i %s a %s%s",
                                game.currentPlayer + 1,
                                game.enemyWon ? "derrota" : "pierde contra",
                                game.enemy.name,
                                (game.enemyWon && game.rewardBump > 0) ? " (con la cabeza derribada)" : ""));

        game.phase = GamePhase::EnemyOutcome;
    }

    // El combate no se corta al ganar: si sobran turnos se puede seguir
    // apostando por la parte de bonus, con el riesgo de comer otra maldición.
    static bool EnemyStillStanding(const Enemy& enemy)
    {
        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].woundsLeft > 0) return true;
        }

        return false;
    }

    static void NextEnemyTurn(GameState& game)
    {
        if (!EnemyStillStanding(game.enemy) || game.enemy.turnsLeft <= 0)
        {
            FinishEnemyFight(game);
            return;
        }

        game.enemyTarget = FirstAlivePart(game.enemy);
        game.phase       = GamePhase::EnemyTarget;
    }

    // Cierra lo que pasa después de un combate: la casilla opcional paga con
    // ítems, las demás con la carta de siempre.
    static void ResolveEnemyOutcome(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        if (game.currentTile == TileKind::Optional)
        {
            if (game.enemyWon)
            {
                if (BlocksCombatReward(player))
                {
                    AddLog(game, "Corona rota: el ganador no se lleva recompensa.");
                }
                else
                {
                    ItemId id = RollTreasureItem(player);

                    if (GiveItem(player, id))
                    {
                        AddLog(game, TextFormat("Jugador %i se lleva: %s",
                                                game.currentPlayer + 1, GetItemDef(id).name));
                        ShowBanner(game, GetItemDef(id).name, COL_GOLD);
                    }
                    else
                    {
                        AddLog(game, "La mochila estaba llena: la recompensa se pierde.");
                    }
                }

                AdvanceTurn(game);
                return;
            }

            OfferCards(game);
            return;
        }

        if (game.enemyWon && BlocksCombatReward(player))
        {
            AddLog(game, "Corona rota: el ganador no se lleva recompensa.");
            AdvanceTurn(game);
            return;
        }

        OfferCards(game);
    }

    static void FinishEnemyRoll(GameState& game)
    {
        Player&    player = game.players[game.currentPlayer];
        EnemyPart& part   = game.enemy.parts[game.enemyTarget];

        game.eventRoll    = game.eventSession.roll;
        game.eventEffects = game.eventSession.appliedNames;

        // El bono del personaje y el del amuleto se suman después de las
        // cartas, y un crítico los ignora igual que a todo lo demás.
        int bonus = GetCharacterBonus(player, true) + game.rollBonus;

        if (bonus != 0 && !game.eventRoll.criticalHit && !game.eventRoll.criticalFail)
        {
            game.eventRoll.total += bonus;
            if (game.eventRoll.total < 1) game.eventRoll.total = 1;
        }

        game.rollBonus = 0;

        bool hit;
        if (game.eventRoll.criticalHit)       hit = true;
        else if (game.eventRoll.criticalFail) hit = false;
        else                                  hit = (game.eventRoll.total >= game.eventDifficulty);

        game.eventSuccess = hit;
        game.enemyNote.clear();

        if (hit)
        {
            int damage = 1 + player.extraWounds;
            player.extraWounds = 0;

            part.woundsLeft -= damage;
            if (part.woundsLeft < 0) part.woundsLeft = 0;

            game.enemyNote = (part.woundsLeft <= 0)
                           ? TextFormat("%s derribada", part.name)
                           : TextFormat("A %s le quedan %i heridas", part.name, part.woundsLeft);

            ShowBanner(game, "¡ACERTASTE!", COL_BLESSING);
            PlaySfxDelayed(game.eventRoll.criticalHit ? SoundId::Crit : SoundId::Hit, 0.55f);
        }
        else
        {
            ShowBanner(game, "FALLASTE", COL_CURSE);
            PlaySfxDelayed(SoundId::Fail, 0.55f);
            GiveForcedCard(game, CardKind::Curse, GetCardLevelForPath(player.path));
        }

        // El historial del combate es lo que leen las cartas que reaccionan a
        // las tiradas anteriores.
        if (game.enemyMemory.rollsPlayed < MAX_COMBAT_ROLLS)
        {
            game.enemyMemory.outcomes[game.enemyMemory.rollsPlayed] = hit ? 1 : -1;
            ++game.enemyMemory.rollsPlayed;
        }

        if (hit) ++game.enemyMemory.wins;
        else     ++game.enemyMemory.losses;

        AddLog(game, TextFormat("J%i ataca %s: D20 %i -> %i vs %i -> %s",
                                game.currentPlayer + 1, part.name,
                                game.eventRoll.natural, game.eventRoll.total,
                                game.eventDifficulty, hit ? "ACIERTO" : "FALLO"));

        if (game.enemy.turnsLeft > 0) --game.enemy.turnsLeft;

        game.revealUntil = GetTime() + REVEAL_TIME;
        game.resultTime  = game.revealUntil;
        game.phase       = GamePhase::EnemyResult;
    }

    static void StartEnemyRoll(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        RollRequest request;
        request.isCombat  = true;
        request.rollIndex = game.enemyMemory.rollsPlayed;

        game.eventDifficulty = GetPartDifficulty(game.enemy, game.enemyTarget,
                                                 player.path, game.currentStage);

        PlaySfx(SoundId::Dice);
        BeginRoll(game.eventSession, player, nullptr, request, &game.enemyMemory);

        if (game.eventSession.choice.active) game.phase = GamePhase::EnemyChoice;
        else if (game.eventSession.done)     FinishEnemyRoll(game);
    }

    static void AdvanceEnemyChoice(GameState& game, int option)
    {
        PlaySfx(SoundId::Select);
        AdvanceRoll(game.eventSession, option);

        if (game.eventSession.choice.active) return;
        if (game.eventSession.done) FinishEnemyRoll(game);
    }

    // Reparte la casilla en la que cayó el jugador.
    static void EnterTile(GameState& game)
    {
        const Player& player = game.players[game.currentPlayer];

        game.currentTile = GetTileKind(player.path, game.currentStage);
        game.tileNote.clear();

        switch (game.currentTile)
        {
            case TileKind::Event:    game.phase = GamePhase::EventRoll;     break;
            case TileKind::Treasure: ResolveTreasure(game);                 break;
            case TileKind::Rest:     ResolveRest(game);                     break;
            case TileKind::Optional: game.phase = GamePhase::OptionalOffer; break;
            default:                 StartEnemyFight(game);                 break;
        }
    }

    // ------------------------------------------------------------------ ítems

    static bool UseItem(GameState& game, int slot)
    {
        Player& player = game.players[game.currentPlayer];

        if (slot < 0 || slot >= static_cast<int>(player.items.size())) return false;

        ItemId id = player.items[slot];
        const ItemDef& def = GetItemDef(id);

        switch (id)
        {
            case ItemId::Charm:
                game.rollBonus += 3;
                game.tileNote = "Amuleto listo: +3 a la tirada que viene.";
                break;

            case ItemId::Whetstone:
                player.extraWounds = 1;
                game.tileNote = "Piedra de afilar: el próximo golpe saca 2 heridas.";
                break;

            case ItemId::Potion:
            {
                std::string removed;

                if (!ClearWorstCurse(player, removed))
                {
                    game.tileNote = "No tenés maldiciones pendientes: la poción queda sin usar.";
                    return false;
                }

                game.tileNote = TextFormat("Poción: se va %s", removed.c_str());
            } break;

            default:
                return false;
        }

        RemoveItem(player, slot);
        PlaySfx(SoundId::Card);
        AddLog(game, TextFormat("Jugador %i usa %s", game.currentPlayer + 1, def.name));
        return true;
    }

    void InitGame(GameState& game)
    {
        StartMatch(game);
        game.phase = GamePhase::Menu;
    }

    static int CpuActorFor(const GameState& game)
    {
        switch (game.phase)
        {
            case GamePhase::PathSelection:
            case GamePhase::EventRoll:
            case GamePhase::EventChoice:
            case GamePhase::EventResult:
            case GamePhase::EnemyIntro:
            case GamePhase::EnemyTarget:
            case GamePhase::EnemyChoice:
            case GamePhase::EnemyResult:
            case GamePhase::EnemyOutcome:
            case GamePhase::OptionalOffer:
            case GamePhase::TreasureResult:
            case GamePhase::RestResult:
            case GamePhase::CardSelection:
                return game.currentPlayer;

            default: return -1;
        }
    }

    static bool IsCpuTurn(const GameState& game)
    {
        int actor = CpuActorFor(game);
        return (actor >= 0) && game.players[actor].isCpu;
    }

    static int PhaseSignature(const GameState& game)
    {
        return static_cast<int>(game.phase) * 100000
             + game.currentPlayer * 10000
             + game.enemy.turnsLeft * 1000
             + game.enemyTarget * 200
             + game.currentStage;
    }

    // La CPU gasta un ítem cuando la tirada que viene lo pide: el amuleto si
    // la dificultad le queda lejos, la piedra si está por rematar una parte y
    // la poción si arrastra maldiciones.
    static void CpuMaybeUseItem(GameState& game, bool isFight)
    {
        const Player& player = game.players[game.currentPlayer];
        if (player.items.empty()) return;

        int difficulty = isFight
                       ? GetPartDifficulty(game.enemy, game.enemyTarget, player.path, game.currentStage)
                       : GetEventDifficulty(player.path, game.currentStage);

        int wanted = CpuChooseItem(player, difficulty, isFight,
                                   isFight ? game.enemy.parts[game.enemyTarget].woundsLeft : 0);

        if (wanted >= 0) UseItem(game, wanted);
    }

    static void RunCpuTurn(GameState& game)
    {
        switch (game.phase)
        {
            case GamePhase::PathSelection:
            {
                Player& player = game.players[game.currentPlayer];
                PathKind chosen = CpuChoosePath(player, game.currentStage);

                PlaySfx(SoundId::Select);
                player.path      = chosen;
                game.pathPreview = chosen;
                AddLog(game, TextFormat("Jugador %i [CPU] toma el camino %s",
                                        game.currentPlayer + 1, GetPathName(chosen)));

                EnterTile(game);
            } break;

            case GamePhase::EventRoll:
            {
                CpuMaybeUseItem(game, false);
                StartEventRoll(game);
            } break;
            case GamePhase::EventChoice: AdvanceEventChoice(game, CpuChooseOption(game.eventSession.choice)); break;
            case GamePhase::EventResult: OfferCards(game); break;

            case GamePhase::EnemyIntro:  game.phase = GamePhase::EnemyTarget; break;

            case GamePhase::EnemyTarget:
            {
                game.enemyTarget = CpuChoosePart(game.players[game.currentPlayer],
                                                 game.enemy, game.currentStage);
                CpuMaybeUseItem(game, true);
                StartEnemyRoll(game);
            } break;

            case GamePhase::EnemyChoice:
                AdvanceEnemyChoice(game, CpuChooseOption(game.eventSession.choice));
                break;

            case GamePhase::EnemyResult:
            {
                if (EnemyBeaten(game.enemy) && game.enemy.turnsLeft > 0 &&
                    !CpuKeepsFighting(game.enemy))
                {
                    FinishEnemyFight(game);
                }
                else
                {
                    NextEnemyTurn(game);
                }
            } break;

            case GamePhase::EnemyOutcome: ResolveEnemyOutcome(game); break;

            case GamePhase::TreasureResult:
            case GamePhase::RestResult:
                AdvanceTurn(game);
                break;

            case GamePhase::OptionalOffer:
            {
                const Player& player = game.players[game.currentPlayer];

                if (CpuTakesOptionalFight(player))
                {
                    AddLog(game, TextFormat("Jugador %i [CPU] acepta la batalla opcional",
                                            game.currentPlayer + 1));
                    StartEnemyFight(game);
                }
                else
                {
                    AddLog(game, TextFormat("Jugador %i [CPU] pasa de largo",
                                            game.currentPlayer + 1));
                    AdvanceTurn(game);
                }
            } break;

            case GamePhase::CardSelection:
                TakeCard(game, CpuChooseCard(game.cardOptions, game.cardOptionCount));
                break;

            default: break;
        }
    }

    static int ReadOptionKey(int optionCount)
    {
        if (optionCount > 0 && IsKeyPressed(KEY_ONE))   return 0;
        if (optionCount > 1 && IsKeyPressed(KEY_TWO))   return 1;
        if (optionCount > 2 && IsKeyPressed(KEY_THREE)) return 2;
        if (optionCount > 3 && IsKeyPressed(KEY_FOUR))  return 3;
        if (optionCount > 4 && IsKeyPressed(KEY_FIVE))  return 4;
        return -1;
    }

    static void UpdateTokenMotion(GameState& game)
    {
        float speed = GetFrameTime() * 7.0f;
        if (speed > 1.0f) speed = 1.0f;

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            Player& player = game.players[i];

            float targetStage = static_cast<float>(player.stage);
            float targetLane  = static_cast<float>(static_cast<int>(player.path));

            player.visualStage += (targetStage - player.visualStage) * speed;
            player.visualLane  += (targetLane  - player.visualLane)  * speed;
        }
    }

    void UpdateGame(GameState& game)
    {
        UpdateTokenMotion(game);

        if (IsKeyPressed(KEY_M)) ToggleMute();

        bool inMatch = (game.phase != GamePhase::Menu && game.phase != GamePhase::Victory);

        if (inMatch && IsKeyPressed(KEY_TAB))
        {
            PlaySfx(SoundId::Select);
            game.showCards = !game.showCards;
            if (game.showCards) game.cardsShown = game.currentPlayer;
        }

        if (game.showCards)
        {
            for (int i = 0; i < PLAYER_COUNT; ++i)
            {
                if (IsKeyPressed(KEY_ONE + i))
                {
                    PlaySfx(SoundId::Select);
                    game.cardsShown = i;
                }
            }
            return;
        }

        if (GetTime() < game.revealUntil) return;

        // La mochila se abre con [I] en los momentos en los que un ítem sirve:
        // antes de tirar en un evento y antes de pegarle a un enemigo.
        bool canOpenItems = (game.phase == GamePhase::EventRoll ||
                             game.phase == GamePhase::EnemyTarget) &&
                            !game.players[game.currentPlayer].isCpu &&
                            !game.players[game.currentPlayer].items.empty();

        if (canOpenItems && IsKeyPressed(KEY_I))
        {
            PlaySfx(SoundId::Select);
            game.itemReturnPhase = game.phase;
            game.phase           = GamePhase::ItemMenu;
            return;
        }

        int signature = PhaseSignature(game);
        if (signature != game.cpuSignature)
        {
            game.cpuSignature  = signature;
            game.cpuNextAction = GetTime() + CPU_THINK_TIME;
        }

        if (IsCpuTurn(game))
        {
            if (GetTime() >= game.cpuNextAction) RunCpuTurn(game);
            return;
        }

        switch (game.phase)
        {
            case GamePhase::Menu:
            {
                if (IsKeyPressed(KEY_ENTER))
                {
                    PlaySfx(SoundId::Select);
                    game.phase = GamePhase::CharacterSelect;
                }
            } break;

            case GamePhase::CharacterSelect:
            {
                int option = ReadOptionKey(static_cast<int>(CharacterId::Count));

                if (option >= 0)
                {
                    PlaySfx(SoundId::Select);
                    game.menuCharacter = static_cast<CharacterId>(option);
                }

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    PlaySfx(SoundId::Card);
                    StartMatch(game);
                    game.phase = GamePhase::TurnOrder;
                }
            } break;

            case GamePhase::TurnOrder:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    PlaySfx(SoundId::Dice);
                    RollTurnOrder(game);
                    game.revealUntil = GetTime() + REVEAL_TIME;
                    game.phase = GamePhase::PathSelection;
                }
            } break;

            case GamePhase::PathSelection:
            {
                bool picked = false;

                if (IsKeyPressed(KEY_ONE))   { game.pathPreview = PathKind::Easy;   picked = true; }
                if (IsKeyPressed(KEY_TWO))   { game.pathPreview = PathKind::Normal; picked = true; }
                if (IsKeyPressed(KEY_THREE)) { game.pathPreview = PathKind::Hard;   picked = true; }

                if (picked)
                {
                    PlaySfx(SoundId::Select);
                    // La ficha se mueve al camino elegido, pero todavía se puede cambiar.
                    game.players[game.currentPlayer].path = game.pathPreview;
                }

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    PlaySfx(SoundId::Card);
                    game.players[game.currentPlayer].path = game.pathPreview;
                    EnterTile(game);
                }
            } break;

            case GamePhase::EventRoll:
            {
                if (IsKeyPressed(KEY_SPACE)) StartEventRoll(game);
            } break;

            case GamePhase::EnemyIntro:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    PlaySfx(SoundId::Select);
                    game.phase = GamePhase::EnemyTarget;
                }
            } break;

            case GamePhase::EnemyTarget:
            {
                int picked = ReadOptionKey(game.enemy.partCount);

                if (picked >= 0 && !IsPartDown(game.enemy, picked))
                {
                    PlaySfx(SoundId::Select);
                    game.enemyTarget = picked;
                }

                if (IsKeyPressed(KEY_SPACE)) StartEnemyRoll(game);
            } break;

            case GamePhase::EnemyChoice:
            {
                int option = ReadOptionKey(static_cast<int>(game.eventSession.choice.options.size()));
                if (option >= 0) AdvanceEnemyChoice(game, option);
            } break;

            case GamePhase::EnemyResult:
            {
                bool canRetire = EnemyBeaten(game.enemy) && game.enemy.turnsLeft > 0;

                if (canRetire && IsKeyPressed(KEY_ENTER))
                {
                    PlaySfx(SoundId::Select);
                    FinishEnemyFight(game);
                }
                else if (IsKeyPressed(KEY_SPACE))
                {
                    NextEnemyTurn(game);
                }
            } break;

            case GamePhase::EnemyOutcome:
            {
                if (IsKeyPressed(KEY_SPACE)) ResolveEnemyOutcome(game);
            } break;

            case GamePhase::EventChoice:
            {
                int option = ReadOptionKey(static_cast<int>(game.eventSession.choice.options.size()));
                if (option >= 0) AdvanceEventChoice(game, option);
            } break;

            case GamePhase::EventResult:
            {
                if (IsKeyPressed(KEY_SPACE)) OfferCards(game);
            } break;

            case GamePhase::CardSelection:
            {
                int option = ReadOptionKey(game.cardOptionCount);
                if (option >= 0) TakeCard(game, option);
            } break;

            case GamePhase::OptionalOffer:
            {
                int option = ReadOptionKey(2);

                if (option == 0)
                {
                    PlaySfx(SoundId::Select);
                    StartEnemyFight(game);
                }
                else if (option == 1)
                {
                    PlaySfx(SoundId::Select);
                    AddLog(game, TextFormat("Jugador %i pasa de largo la batalla opcional",
                                            game.currentPlayer + 1));
                    AdvanceTurn(game);
                }
            } break;

            case GamePhase::TreasureResult:
            case GamePhase::RestResult:
            {
                if (IsKeyPressed(KEY_SPACE)) AdvanceTurn(game);
            } break;

            case GamePhase::ItemMenu:
            {
                if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_TAB))
                {
                    game.phase = game.itemReturnPhase;
                    break;
                }

                int option = ReadOptionKey(static_cast<int>(game.players[game.currentPlayer].items.size()));

                if (option >= 0 && UseItem(game, option))
                {
                    game.phase = game.itemReturnPhase;
                }
            } break;

            case GamePhase::Victory:
            {
                if (IsKeyPressed(KEY_R))
                {
                    PlaySfx(SoundId::Select);
                    game.phase = GamePhase::CharacterSelect;
                }
            } break;
        }
    }

    // ------------------------------------------------------------------ dibujo

    static const char* PlayerLabel(const Player& player)
    {
        return player.isCpu ? TextFormat("Jugador %i [CPU]", player.id + 1)
                            : TextFormat("Jugador %i", player.id + 1);
    }

    static Rectangle WidePanelBounds()
    {
        return Rectangle{ OVERLAY_X, OVERLAY_Y, OVERLAY_W, OVERLAY_H };
    }

    static bool IsRevealing(const GameState& game)
    {
        return GetTime() < game.revealUntil;
    }

    static void DrawPanel(Rectangle bounds, const char* title)
    {
        DrawSoftPanel(bounds, PANEL_COLOR, PANEL_BORDER, 0.08f);

        Rectangle header{ bounds.x, bounds.y, bounds.width, 34.0f };
        DrawRectangleRounded(header, 0.35f, 6, Fade(COL_MAGENTA, 0.35f));
        DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y) + 24,
                      static_cast<int>(bounds.width), 10, Fade(COL_MAGENTA, 0.0f));

        UiText(title, static_cast<int>(bounds.x) + 16, static_cast<int>(bounds.y) + 7,
               20, COL_CREAM, FontStyle::Bold);
    }

    static void DrawDie(int x, int y, int size, int value, bool spinning, Color accent)
    {
        (void)spinning;

        Rectangle face{ static_cast<float>(x), static_cast<float>(y),
                        static_cast<float>(size), static_cast<float>(size) };

        Rectangle shadow = face;
        shadow.x += 3.0f; shadow.y += 4.0f;
        DrawRectangleRounded(shadow, 0.26f, 6, Fade(BLACK, 0.4f));

        DrawRectangleRounded(face, 0.26f, 6, COL_CREAM);
        DrawRectangleRoundedLinesEx(face, 0.26f, 6, 3.0f, accent);

        const char* text = TextFormat("%i", value);
        int textSize = (value >= 10) ? size - 22 : size - 16;
        int width = UiMeasure(text, textSize, FontStyle::Bold);

        UiText(text, x + size / 2 - width / 2, y + size / 2 - textSize / 2 - 2,
               textSize, COL_INK_DEEP, FontStyle::Bold);
    }

    static void DrawBanner(const GameState& game)
    {
        if (game.bannerText.empty()) return;

        double elapsed = GetTime() - game.bannerTime;
        if (elapsed < 0.0 || elapsed > 1.6) return;

        float t = EaseOutBack(static_cast<float>(elapsed) / 0.28f);
        float fade = (elapsed > 1.2) ? 1.0f - static_cast<float>(elapsed - 1.2) / 0.4f : 1.0f;

        const char* text = game.bannerText.c_str();
        Color color = game.bannerColor;

        // Sobre la columna del tablero, para no tapar los paneles de la derecha.
        // Con un enemigo en pantalla baja al pie, que es el hueco que queda.
        bool enemyShown = (game.phase == GamePhase::EnemyIntro   ||
                           game.phase == GamePhase::EnemyTarget  ||
                           game.phase == GamePhase::EnemyChoice  ||
                           game.phase == GamePhase::EnemyResult  ||
                           game.phase == GamePhase::EnemyOutcome);

        int size = static_cast<int>(16.0f + 24.0f * t);
        int width = UiMeasure(text, size, FontStyle::Bold);
        int x = 316 - width / 2;
        int y = enemyShown ? 604 : 268;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x) - 26.0f, static_cast<float>(y) - 12.0f,
                                        static_cast<float>(width) + 52.0f, static_cast<float>(size) + 24.0f },
                             0.4f, 8, Fade(COL_INK_DEEP, 0.8f * fade));
        UiText(text, x, y, size, Fade(color, fade), FontStyle::Bold);
    }

    static void DrawChoice(const PendingChoice& choice)
    {
        Rectangle bounds = WidePanelBounds();
        DrawPanel(bounds, "ELEGÍ");

        UiTextWrapped(choice.title.c_str(), static_cast<int>(bounds.x) + 16,
                      static_cast<int>(bounds.y) + 42, static_cast<int>(bounds.width) - 32,
                      18, COL_CREAM, 1, FontStyle::Bold);

        int count = static_cast<int>(choice.options.size());
        if (count <= 0) return;

        float available = bounds.height - 86.0f;
        float step      = available / static_cast<float>(count);
        if (step > 46.0f) step = 46.0f;

        float rowHeight = step - 8.0f;
        int   textSize  = (step >= 42.0f) ? 17 : 15;

        for (int i = 0; i < count; ++i)
        {
            Rectangle row{ bounds.x + 14.0f, bounds.y + 76.0f + static_cast<float>(i) * step,
                           bounds.width - 28.0f, rowHeight };

            DrawRectangleRounded(row, 0.3f, 6, Fade(COL_MAGENTA, 0.18f));
            DrawRectangleRoundedLinesEx(row, 0.3f, 6, 2.0f, Fade(COL_GOLD, 0.7f));

            UiText(TextFormat("[%i]", i + 1), static_cast<int>(row.x) + 14,
                   static_cast<int>(row.y + rowHeight * 0.5f) - textSize / 2, textSize,
                   COL_GOLD, FontStyle::Bold);

            UiText(choice.options[i].label.c_str(), static_cast<int>(row.x) + 58,
                   static_cast<int>(row.y + rowHeight * 0.5f) - textSize / 2, textSize, COL_CREAM);
        }
    }

    static void DrawPlayerList(const GameState& game)
    {
        Rectangle bounds{ PANEL_X, LIST_Y, PANEL_W, LIST_H };
        DrawPanel(bounds, "JUGADORES");

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[i];

            int rowY = static_cast<int>(LIST_Y) + 52 + i * 54;
            bool isActive = (i == game.currentPlayer) &&
                            (game.phase == GamePhase::PathSelection ||
                             game.phase == GamePhase::EventRoll ||
                             game.phase == GamePhase::EventChoice ||
                             game.phase == GamePhase::EventResult ||
                             game.phase == GamePhase::CardSelection);

            Rectangle row{ PANEL_X + 8.0f, static_cast<float>(rowY) - 6.0f,
                           PANEL_W - 16.0f, 46.0f };

            if (isActive)
            {
                DrawRectangleRounded(row, 0.3f, 6, Fade(player.color, 0.18f));
                DrawRectangleRoundedLinesEx(row, 0.3f, 6, 2.0f, Fade(player.color, 0.6f));
            }
            else
            {
                DrawRectangleRounded(row, 0.3f, 6, Fade(COL_INK_DEEP, 0.35f));
            }

            DrawCircle(static_cast<int>(PANEL_X) + 32, rowY + 17, 12.0f, player.color);
            UiText(TextFormat("%i", i + 1), static_cast<int>(PANEL_X) + 28, rowY + 9, 15,
                   COL_INK_DEEP, FontStyle::Bold);

            UiText(TextFormat("Jugador %i", i + 1),
                   static_cast<int>(PANEL_X) + 54, rowY + 1, 18, COL_CREAM, FontStyle::Bold);

            if (player.isCpu)
            {
                Rectangle tag{ PANEL_X + 142.0f, static_cast<float>(rowY) + 2.0f, 38.0f, 17.0f };
                DrawRectangleRounded(tag, 0.5f, 5, Fade(COL_MAGENTA, 0.55f));
                UiText("CPU", static_cast<int>(tag.x) + 8, static_cast<int>(tag.y) + 2, 12,
                       COL_CREAM, FontStyle::Bold);
            }

            UiText(GetPathName(player.path),
                   static_cast<int>(PANEL_X) + 54, rowY + 23, 15, GetPathColor(player.path));

            UiText(TextFormat("%i", CountCards(player, CardKind::Blessing)),
                   static_cast<int>(PANEL_X) + 230, rowY + 10, 18, COL_BLESSING, FontStyle::Bold);
            UiText("bendiciones", static_cast<int>(PANEL_X) + 250, rowY + 12, 14,
                   Fade(COL_BLESSING, 0.75f));

            UiText(TextFormat("%i", CountCards(player, CardKind::Curse)),
                   static_cast<int>(PANEL_X) + 370, rowY + 10, 18, COL_CURSE, FontStyle::Bold);
            UiText("maldiciones", static_cast<int>(PANEL_X) + 390, rowY + 12, 14,
                   Fade(COL_CURSE, 0.75f));

            UiText(TextFormat("%i sin usar", CountPendingCards(player)),
                   static_cast<int>(PANEL_X) + 468, rowY + 12, 15, COL_CREAM_DIM);
        }
    }

    static void DrawLog(const GameState& game)
    {
        for (int i = 0; i < LOG_LINES; ++i)
        {
            float alpha = 0.38f + 0.14f * static_cast<float>(i);

            UiText(game.log[i].c_str(),
                   static_cast<int>(PANEL_X) + 16,
                   static_cast<int>(PANEL_Y) + 196 + i * 21,
                   15, Fade(COL_CREAM, alpha));
        }
    }

    // Las tres opciones salen como cartas de verdad, lado a lado, que es lo
    // que pide el documento: nada de renglones de lista.
    static void DrawCardOptionsUI(const GameState& game)
    {
        float time = static_cast<float>(GetTime());

        constexpr float CARD_W = 296.0f;
        constexpr float CARD_H = 384.0f;
        constexpr float CARD_Y = 182.0f;
        constexpr float CARD_GAP = 20.0f;

        int count = game.cardOptionCount;
        if (count <= 0) return;

        float totalW = count * CARD_W + (count - 1) * CARD_GAP;
        float firstX = SCREEN_WIDTH * 0.5f - totalW * 0.5f;

        const Player& player = game.players[game.currentPlayer];

        UiTextCentered(TextFormat("%s: elegí una %s", PlayerLabel(player),
                                  game.offerKind == CardKind::Blessing ? "bendición" : "maldición"),
                       SCREEN_WIDTH / 2, 118, 28, COL_CREAM, FontStyle::Bold);

        UiTextCentered("las tres vienen del mazo que te tocó por el camino",
                       SCREEN_WIDTH / 2, 154, 16, Fade(COL_CREAM, 0.6f));

        for (int i = 0; i < count; ++i)
        {
            const CardDef& def = GetCardDef(game.cardOptions[i]);

            bool  isBlessing = (def.kind == CardKind::Blessing);
            Color color      = isBlessing ? COL_BLESSING : COL_CURSE;
            Color levelColor = GetPathColor(static_cast<PathKind>(static_cast<int>(def.level)));

            float lift = 3.0f * Pulse(time + static_cast<float>(i) * 0.6f, 1.4f);

            Rectangle card{ firstX + static_cast<float>(i) * (CARD_W + CARD_GAP),
                            CARD_Y - lift, CARD_W, CARD_H };

            Rectangle shadow = card;
            shadow.x += 5.0f; shadow.y += 9.0f;
            DrawRectangleRounded(shadow, 0.07f, 8, Fade(BLACK, 0.45f));

            DrawRectangleRounded(card, 0.07f, 8, Color{ 26, 36, 64, 255 });
            DrawRectangleRoundedLinesEx(card, 0.07f, 8, 3.0f, Fade(color, 0.9f));

            // Cabecera de la carta con el tipo y la rareza.
            Rectangle head{ card.x + 10.0f, card.y + 10.0f, card.width - 20.0f, 46.0f };
            DrawRectangleRounded(head, 0.25f, 6, Fade(color, 0.25f));
            DrawRectangleRounded(Rectangle{ head.x, head.y, 7.0f, head.height }, 0.6f, 4, levelColor);

            UiText(isBlessing ? "BENDICIÓN" : "MALDICIÓN",
                   static_cast<int>(head.x) + 18, static_cast<int>(head.y) + 6, 15,
                   color, FontStyle::Bold);

            UiText(GetCardLevelName(def.level),
                   static_cast<int>(head.x) + 18, static_cast<int>(head.y) + 25, 14,
                   Fade(levelColor, 0.95f), FontStyle::Bold);

            // Ilustración de relleno: un rombo con el símbolo del tipo. Es a
            // propósito simple, hasta que el grupo defina el arte.
            Rectangle art{ card.x + 10.0f, head.y + head.height + 10.0f, card.width - 20.0f, 120.0f };
            DrawRectangleRounded(art, 0.1f, 6, Fade(COL_INK_DEEP, 0.75f));
            DrawRectangleRoundedLinesEx(art, 0.1f, 6, 1.0f, Fade(color, 0.35f));

            float cx = art.x + art.width * 0.5f;
            float cy = art.y + art.height * 0.5f;

            DrawCircleV(Vector2{ cx, cy }, 40.0f, Fade(color, 0.12f));

            if (isBlessing)
            {
                DrawCrown(static_cast<int>(cx), static_cast<int>(cy) + 6, 44.0f, Fade(color, 0.85f));
            }
            else
            {
                DrawTriangle(Vector2{ cx, cy - 30.0f },
                             Vector2{ cx - 28.0f, cy + 20.0f },
                             Vector2{ cx + 28.0f, cy + 20.0f }, Fade(color, 0.85f));
                DrawRectangle(static_cast<int>(cx) - 3, static_cast<int>(cy) - 12, 6, 20, COL_INK_DEEP);
                DrawRectangle(static_cast<int>(cx) - 3, static_cast<int>(cy) + 12, 6, 5, COL_INK_DEEP);
            }

            UiTextWrapped(def.name, static_cast<int>(card.x) + 20,
                          static_cast<int>(art.y + art.height) + 14,
                          static_cast<int>(card.width) - 40, 22, color, 2, FontStyle::Bold);

            UiTextWrapped(def.text, static_cast<int>(card.x) + 20,
                          static_cast<int>(art.y + art.height) + 66,
                          static_cast<int>(card.width) - 40, 16, Fade(COL_CREAM, 0.9f), 6);

            // El número de la tecla, abajo y bien grande.
            Rectangle key{ card.x + card.width * 0.5f - 26.0f, card.y + card.height - 46.0f,
                           52.0f, 34.0f };
            DrawRectangleRounded(key, 0.4f, 6, Fade(COL_GOLD, 0.22f));
            DrawRectangleRoundedLinesEx(key, 0.4f, 6, 2.0f, Fade(COL_GOLD, 0.8f));

            UiTextCentered(TextFormat("%i", i + 1),
                           static_cast<int>(key.x + key.width * 0.5f),
                           static_cast<int>(key.y) + 5, 23, COL_GOLD, FontStyle::Bold);
        }

        UiTextCentered(TextFormat("[1-%i] quedarte con una", count),
                       SCREEN_WIDTH / 2, 600, 20, COL_GOLD, FontStyle::Bold);
    }

    static bool IsEnemyPhase(const GameState& game)
    {
        return game.phase == GamePhase::EnemyIntro   ||
               game.phase == GamePhase::EnemyTarget  ||
               game.phase == GamePhase::EnemyChoice  ||
               game.phase == GamePhase::EnemyResult  ||
               game.phase == GamePhase::EnemyOutcome;
    }

    static void DrawWoundPips(const EnemyPart& part, int x, int y, Color color)
    {
        for (int i = 0; i < part.wounds; ++i)
        {
            int cx = x + i * 20;
            bool alive = (i < part.woundsLeft);

            if (alive) DrawCircle(cx, y, 7.0f, color);
            else       DrawCircle(cx, y, 7.0f, Fade(COL_CREAM, 0.12f));

            DrawCircleLines(cx, y, 7.0f, Fade(COL_INK_DEEP, 0.9f));
        }
    }

    // El enemigo se dibuja sobre la columna del tablero: una parte por caja,
    // de la cabeza abajo hasta la cola, con sus heridas y su tecla.
    static void DrawEnemyFigure(const GameState& game)
    {
        const Enemy& enemy = game.enemy;
        if (enemy.partCount <= 0) return;

        const Player& player = game.players[game.currentPlayer];
        float time = static_cast<float>(GetTime());

        Rectangle area{ 40.0f, 168.0f, 552.0f, 492.0f };
        DrawSoftPanel(area, Fade(COL_INK_DEEP, 0.9f), Fade(COL_MAGENTA, 0.55f), 0.05f);

        UiTextCentered(GetEnemyRankName(enemy.rank), 316, 184, 15,
                       Fade(COL_GOLD, 0.85f), FontStyle::Bold);
        UiTextCentered(enemy.name, 316, 204, 25, COL_CREAM, FontStyle::Bold);
        UiTextCentered(enemy.subtitle, 316, 238, 15, Fade(COL_CREAM, 0.6f));

        float step = (enemy.partCount >= 4) ? 92.0f : 118.0f;
        float boxH = step - 18.0f;
        float top  = 268.0f;

        bool aiming = (game.phase == GamePhase::EnemyTarget);

        for (int slot = 0; slot < enemy.partCount; ++slot)
        {
            int index = enemy.partCount - 1 - slot;
            const EnemyPart& part = enemy.parts[index];

            bool down  = (part.woundsLeft <= 0);
            bool aimed = (index == game.enemyTarget) && !down;

            Color tint = part.counts ? COL_MAGENTA_LIT : COL_GOLD;

            Rectangle box{ 316.0f - part.width * 0.5f, top + static_cast<float>(slot) * step,
                           part.width, boxH };

            Rectangle shadow = box;
            shadow.x += 3.0f; shadow.y += 4.0f;
            DrawRectangleRounded(shadow, 0.3f, 6, Fade(BLACK, 0.35f));

            DrawRectangleRounded(box, 0.3f, 6, Fade(COL_INK_SOFT, down ? 0.45f : 1.0f));
            DrawRectangleRounded(Rectangle{ box.x, box.y, box.width, box.height * 0.42f },
                                 0.45f, 6, Fade(tint, down ? 0.08f : 0.24f));

            float border = (aimed && aiming) ? 3.0f + Pulse(time, 3.4f) : 2.0f;
            Color borderColor = down ? Fade(COL_CREAM, 0.18f)
                                     : ((aimed && aiming) ? COL_GOLD : Fade(tint, 0.8f));
            DrawRectangleRoundedLinesEx(box, 0.3f, 6, border, borderColor);

            float dim = down ? 0.35f : 1.0f;

            UiText(TextFormat("[%i]", index + 1), static_cast<int>(box.x) + 14,
                   static_cast<int>(box.y) + 8, 16, Fade(COL_GOLD, dim), FontStyle::Bold);

            UiText(part.name, static_cast<int>(box.x) + 52, static_cast<int>(box.y) + 6, 20,
                   Fade(COL_CREAM, dim), FontStyle::Bold);

            if (down)
            {
                UiText("DERRIBADA", static_cast<int>(box.x) + 52,
                       static_cast<int>(box.y) + 34, 16, Fade(COL_CREAM, 0.4f), FontStyle::Bold);
            }
            else
            {
                UiText(TextFormat("pide %i", GetPartDifficulty(enemy, index, player.path, game.currentStage)),
                       static_cast<int>(box.x) + 52, static_cast<int>(box.y) + 34, 16,
                       Fade(COL_CREAM, 0.8f));

                DrawWoundPips(part, static_cast<int>(box.x + box.width) - 24 - (part.wounds - 1) * 20,
                              static_cast<int>(box.y) + 22, tint);
            }

            if (!part.counts)
            {
                UiText("bonus", static_cast<int>(box.x + box.width) - 62,
                       static_cast<int>(box.y) + 42, 13, Fade(COL_GOLD, dim * 0.8f), FontStyle::Bold);
            }
        }
    }

    static void DrawTurnPanel(const GameState& game)
    {
        Rectangle bounds{ PANEL_X, PANEL_Y, PANEL_W, PANEL_H };

        int textX = static_cast<int>(PANEL_X) + 16;
        int line1 = static_cast<int>(PANEL_Y) + 50;
        int line2 = static_cast<int>(PANEL_Y) + 78;
        int line3 = static_cast<int>(PANEL_Y) + 106;
        int line4 = static_cast<int>(PANEL_Y) + 140;

        const Player& player = game.players[game.currentPlayer];

        switch (game.phase)
        {
            case GamePhase::TurnOrder:
            {
                DrawPanel(bounds, "ORDEN DE TURNOS");

                UiText("Los cuatro tiran un D20 para ver quién abre la partida.",
                       textX, line1, 18, COL_CREAM);
                UiText("El más alto empieza y sigue el jugador de su derecha.",
                       textX, line2, 16, Fade(COL_CREAM, 0.75f));

                UiText("[ESPACIO] tirar", textX, line4 + 12, 20, COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::PathSelection:
            {
                DrawPanel(bounds, TextFormat("TURNO DE %s  ·  ETAPA %i / %i",
                                             player.isCpu ? TextFormat("LA CPU (JUGADOR %i)", game.currentPlayer + 1)
                                                          : TextFormat("JUGADOR %i", game.currentPlayer + 1),
                                             game.currentStage + 1, STAGE_COUNT));

                UiText(player.isCpu ? "La CPU está eligiendo camino..."
                                    : "Elegí tu camino para esta etapa:",
                       textX, line1, 18, COL_CREAM);

                const char* labels[3] = { "[1] FÁCIL", "[2] NORMAL", "[3] DIFÍCIL" };
                const char* rewards[3] = { "carta común", "carta peligrosa", "carta épica" };

                for (int i = 0; i < 3; ++i)
                {
                    PathKind path = static_cast<PathKind>(i);
                    int rowY = line2 + i * 28;
                    bool selected = (path == game.pathPreview);

                    Rectangle row{ static_cast<float>(textX) - 6.0f, static_cast<float>(rowY) - 4.0f,
                                   PANEL_W - 44.0f, 24.0f };

                    DrawRectangleRounded(row, 0.4f, 6, Fade(GetPathColor(path), selected ? 0.32f : 0.10f));
                    if (selected) DrawRectangleRoundedLinesEx(row, 0.4f, 6, 2.0f, COL_GOLD);

                    UiText(labels[i], textX, rowY, 17, GetPathColor(path), FontStyle::Bold);
                    UiText(TextFormat("dificultad %i", GetEventDifficulty(path, game.currentStage)),
                           textX + 130, rowY, 17, COL_CREAM);
                    UiText(rewards[i], textX + 280, rowY, 16, Fade(COL_CREAM, 0.75f));
                }

                if (!player.isCpu)
                {
                    UiText("[ENTER] confirmar", textX, line4 + 12, 20, COL_GOLD, FontStyle::Bold);
                    UiText("podés cambiar de camino todas las veces que quieras antes de confirmar",
                           textX + 190, line4 + 16, 14, Fade(COL_CREAM, 0.6f));
                }
            } break;

            case GamePhase::EventRoll:
            {
                DrawPanel(bounds, TextFormat("JUGADOR %i  ·  CAMINO %s",
                                             game.currentPlayer + 1, GetPathName(player.path)));

                UiText("Dificultad", textX, line1, 15, Fade(COL_CREAM, 0.65f));
                UiText(TextFormat("%i", GetEventDifficulty(player.path, game.currentStage)),
                       textX, line1 + 16, 32, COL_CREAM, FontStyle::Bold);

                RollRequest request;
                request.isCombat  = false;
                request.rollIndex = -1;

                std::string pending = DescribePendingEffects(player, nullptr, request);

                UiText("Cartas que se activan en esta tirada:", textX + 100, line1, 15, Fade(COL_CREAM, 0.65f));
                UiTextWrapped(pending.empty() ? "ninguna" : pending.c_str(),
                              textX + 100, line1 + 22, static_cast<int>(PANEL_W) - 140, 16,
                              pending.empty() ? Fade(COL_CREAM, 0.45f) : COL_GOLD, 2);

                if (player.isCpu) UiText("La CPU está por tirar...", textX, line4 + 14, 19,
                                          Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                else               UiText("[ESPACIO] tirar el D20", textX, line4 + 14, 19,
                                          COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::EventChoice:
            {
                DrawChoice(game.eventSession.choice);
            } break;

            case GamePhase::EventResult:
            {
                bool revealing = IsRevealing(game);

                DrawPanel(bounds, revealing ? "TIRANDO..."
                                            : (game.eventSuccess ? "EVENTO SUPERADO" : "EVENTO FALLIDO"));

                int value = revealing ? SpinningValue(static_cast<float>(GetTime()), DICE_FACES)
                                      : game.eventRoll.natural;
                Color accent = revealing ? COL_GOLD
                                         : (game.eventSuccess ? COL_BLESSING : COL_CURSE);

                DrawDie(textX, line1 - 4, 58, value, revealing, accent);

                if (!revealing)
                {
                    UiText(TextFormat("Total %i", game.eventRoll.total),
                           textX + 76, line1 + 2, 26, COL_CREAM, FontStyle::Bold);
                    UiText(TextFormat("contra dificultad %i", game.eventDifficulty),
                           textX + 76, line1 + 34, 16, Fade(COL_CREAM, 0.75f));

                    if (game.eventRoll.criticalHit)
                    {
                        UiText("20 NATURAL: éxito crítico, se ignoran los modificadores",
                               textX, line3 + 14, 16, COL_GOLD);
                    }
                    else if (game.eventRoll.criticalFail)
                    {
                        UiText("1 NATURAL: fallo crítico, se ignoran los modificadores",
                               textX, line3 + 14, 16, ORANGE);
                    }
                    else if (!game.eventEffects.empty())
                    {
                        UiTextWrapped(TextFormat("Se aplicaron: %s", game.eventEffects.c_str()),
                                      textX, line3 + 10, static_cast<int>(PANEL_W) - 32, 16, COL_GOLD, 2);
                    }

                    if (player.isCpu)
                    {
                        UiText("La CPU elige su carta...", textX, line4 + 14, 19,
                               Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                    }
                    else
                    {
                        UiText(game.eventSuccess ? "[ESPACIO] elegir bendición"
                                                 : "[ESPACIO] recibir maldición",
                               textX, line4 + 12, 20, COL_GOLD, FontStyle::Bold);
                    }
                }
            } break;

            case GamePhase::OptionalOffer:
            {
                DrawPanel(bounds, "BATALLA OPCIONAL");

                UiText("Hay un enemigo que podés esquivar.", textX, line1, 19,
                       COL_CREAM, FontStyle::Bold);

                UiText("Si peleás y ganás te llevás un ítem, no una carta.",
                       textX, line2, 16, Fade(COL_CREAM, 0.85f));
                UiText("Si perdés, maldición igual que en cualquier combate.",
                       textX, line2 + 24, 16, Fade(COL_CREAM, 0.85f));
                UiText("Si pasás de largo no gana ni pierde nadie.",
                       textX, line2 + 48, 16, Fade(COL_CREAM, 0.85f));

                if (player.isCpu)
                {
                    UiText("La CPU lo está pensando...", textX, line4 + 84, 19,
                           Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                }
                else
                {
                    UiText("[1] pelear", textX, line4 + 78, 21, COL_GOLD, FontStyle::Bold);
                    UiText("[2] pasar de largo", textX + 160, line4 + 78, 21,
                           Fade(COL_CREAM, 0.8f), FontStyle::Bold);
                }
            } break;

            case GamePhase::TreasureResult:
            {
                DrawPanel(bounds, "TESORO");

                UiTextWrapped(game.tileNote.c_str(), textX, line1,
                              static_cast<int>(PANEL_W) - 48, 20, COL_GOLD, 2, FontStyle::Bold);

                UiText("Los ítems se usan a mano con [I] antes de tirar.",
                       textX, line3, 16, Fade(COL_CREAM, 0.75f));

                if (!player.isCpu)
                {
                    UiText("[ESPACIO] seguir", textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                }
            } break;

            case GamePhase::RestResult:
            {
                DrawPanel(bounds, "DESCANSO");

                UiTextWrapped(game.tileNote.c_str(), textX, line1,
                              static_cast<int>(PANEL_W) - 48, 20, COL_BLESSING, 2, FontStyle::Bold);

                UiText("En esta casilla no se tira el dado ni se reparten cartas.",
                       textX, line3, 16, Fade(COL_CREAM, 0.75f));

                if (!player.isCpu)
                {
                    UiText("[ESPACIO] seguir", textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                }
            } break;

            case GamePhase::EnemyIntro:
            {
                DrawPanel(bounds, TextFormat("%s  ·  ETAPA %i / %i",
                                             GetEnemyRankName(game.enemy.rank),
                                             game.currentStage + 1, STAGE_COUNT));

                UiText(TextFormat("Tenés %i turnos para derribarlo.", game.enemy.turns),
                       textX, line1, 19, COL_CREAM, FontStyle::Bold);

                const char* rules[4] =
                {
                    "Cada turno elegís una parte y tirás el D20 contra su dificultad.",
                    "Si acertás le sacás una herida; si fallás, contraataca con una maldición.",
                    "Ganás cuando derribás las partes que cuentan (las de color violeta).",
                    "La cabeza no hace falta, pero derribarla sube el nivel de la recompensa."
                };

                for (int i = 0; i < 4; ++i)
                {
                    int rowY = line2 + i * 24;
                    DrawCircle(textX + 5, rowY + 9, 3.0f, COL_MAGENTA_LIT);
                    UiText(rules[i], textX + 20, rowY, 16, Fade(COL_CREAM, 0.85f));
                }

                if (player.isCpu)
                {
                    UiText("La CPU se prepara...", textX, line4 + 86, 19,
                           Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                }
                else
                {
                    UiText("[ESPACIO] atacar", textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                }
            } break;

            case GamePhase::EnemyTarget:
            {
                DrawPanel(bounds, TextFormat("TURNO %i / %i  ·  CAMINO %s",
                                             game.enemy.turns - game.enemy.turnsLeft + 1,
                                             game.enemy.turns, GetPathName(player.path)));

                UiText(TextFormat("Faltan derribar %i partes de las que cuentan.",
                                  game.enemy.neededDowns - CountDownedCounting(game.enemy)),
                       textX, line1, 18, COL_CREAM, FontStyle::Bold);

                UiText(TextFormat("Atacás: %s   (pide %i)",
                                  game.enemy.parts[game.enemyTarget].name,
                                  GetPartDifficulty(game.enemy, game.enemyTarget,
                                                    player.path, game.currentStage)),
                       textX, line2, 18, COL_GOLD, FontStyle::Bold);

                RollRequest request;
                // El preview tiene que preguntar por lo mismo que la tirada real:
                // los combates contra enemigos cuentan como combate.
                request.isCombat  = true;
                request.rollIndex = game.enemyMemory.rollsPlayed;

                std::string pending = DescribePendingEffects(player, nullptr, request);

                UiText("Cartas que se activan en esta tirada:", textX, line3 + 6, 15,
                       Fade(COL_CREAM, 0.65f));
                UiTextWrapped(pending.empty() ? "ninguna" : pending.c_str(),
                              textX, line3 + 28, static_cast<int>(PANEL_W) - 48, 16,
                              pending.empty() ? Fade(COL_CREAM, 0.45f) : COL_GOLD, 2);

                if (player.isCpu)
                {
                    UiText("La CPU elige dónde pegar...", textX, line4 + 84, 19,
                           Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                }
                else
                {
                    UiText(TextFormat("[1-%i] elegir parte   ·   [ESPACIO] tirar",
                                      game.enemy.partCount),
                           textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                }
            } break;

            case GamePhase::EnemyChoice:
            {
                DrawChoice(game.eventSession.choice);
            } break;

            case GamePhase::EnemyResult:
            {
                bool revealing = IsRevealing(game);

                DrawPanel(bounds, revealing ? "TIRANDO..."
                                            : (game.eventSuccess ? "ACIERTO" : "FALLO"));

                int value = revealing ? SpinningValue(static_cast<float>(GetTime()), DICE_FACES)
                                      : game.eventRoll.natural;
                Color accent = revealing ? COL_GOLD
                                         : (game.eventSuccess ? COL_BLESSING : COL_CURSE);

                DrawDie(textX, line1 - 4, 58, value, revealing, accent);

                if (!revealing)
                {
                    UiText(TextFormat("Total %i", game.eventRoll.total),
                           textX + 76, line1 + 2, 26, COL_CREAM, FontStyle::Bold);
                    UiText(TextFormat("contra %i", game.eventDifficulty),
                           textX + 76, line1 + 34, 16, Fade(COL_CREAM, 0.75f));

                    UiTextWrapped(game.enemyNote.c_str(), textX, line3 + 8,
                                  static_cast<int>(PANEL_W) - 48, 18,
                                  game.eventSuccess ? COL_BLESSING : COL_CURSE, 2, FontStyle::Bold);

                    if (!game.eventEffects.empty())
                    {
                        UiTextWrapped(TextFormat("Se aplicaron: %s", game.eventEffects.c_str()),
                                      textX, line4 + 18, static_cast<int>(PANEL_W) - 48, 15,
                                      COL_GOLD, 2);
                    }

                    bool canRetire = EnemyBeaten(game.enemy) && game.enemy.turnsLeft > 0;

                    UiText(canRetire
                               ? TextFormat("Ya lo tenés ganado y te quedan %i turnos.", game.enemy.turnsLeft)
                               : TextFormat("Turnos restantes: %i", game.enemy.turnsLeft),
                           textX, line4 + 58, 16,
                           canRetire ? COL_GOLD : Fade(COL_CREAM, 0.7f));

                    if (!player.isCpu)
                    {
                        if (canRetire)
                        {
                            UiText("[ENTER] cobrar la recompensa", textX, line4 + 84, 19,
                                   COL_GOLD, FontStyle::Bold);
                            UiText("[ESPACIO] seguir y arriesgar por la cabeza",
                                   textX, line4 + 108, 17, Fade(COL_CREAM, 0.8f), FontStyle::Bold);
                        }
                        else
                        {
                            UiText("[ESPACIO] seguir", textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                        }
                    }
                }
            } break;

            case GamePhase::EnemyOutcome:
            {
                DrawPanel(bounds, game.enemyWon ? "ENEMIGO DERROTADO" : "COMBATE PERDIDO");

                UiText(game.enemyWon ? "Derribaste las partes que hacían falta."
                                     : "Se te acabaron los turnos.",
                       textX, line1, 19, game.enemyWon ? COL_BLESSING : COL_CURSE,
                       FontStyle::Bold);

                if (game.enemyWon && BonusPartDown(game.enemy))
                {
                    UiText("Además le volteaste la cabeza: la recompensa sube de nivel.",
                           textX, line2, 16, COL_GOLD);
                }
                else if (game.enemyWon)
                {
                    UiText("La cabeza quedó en pie: recompensa normal.",
                           textX, line2, 16, Fade(COL_CREAM, 0.7f));
                }
                else
                {
                    UiText("El enemigo se queda con la ladera y vos con una maldición.",
                           textX, line2, 16, Fade(COL_CREAM, 0.7f));
                }

                if (!player.isCpu)
                {
                    UiText(game.enemyWon ? "[ESPACIO] elegir bendición"
                                         : "[ESPACIO] recibir maldición",
                           textX, line4 + 84, 20, COL_GOLD, FontStyle::Bold);
                }
            } break;

            case GamePhase::CardSelection:
            {
                DrawCardOptionsUI(game);
            } break;

            default: break;
        }
    }

    static void DrawTitle(int centerX, int y, int size)
    {
        const char* title = "KING OF THE HILL";
        int width = UiMeasure(title, size, FontStyle::Bold);

        UiText(title, centerX - width / 2 + 3, y + 4, size, Fade(COL_MAGENTA, 0.9f), FontStyle::Bold);
        UiText(title, centerX - width / 2, y, size, COL_CREAM, FontStyle::Bold);

        DrawCrown(centerX + width / 2 + static_cast<int>(size * 0.42f),
                  y - static_cast<int>(size * 0.06f), size * 0.5f, COL_GOLD);
    }

    static void DrawMenu()
    {
        float time = static_cast<float>(GetTime());

        Rectangle band{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 112.0f };
        DrawRectangleRec(band, COL_MAGENTA);
        DrawWaveBand(band, Fade(COL_INK_DEEP, 0.55f), time, 4);

        DrawTitle(SCREEN_WIDTH / 2, 150, 56);

        UiTextCentered("roguelite de un jugador  ·  tres rivales de la CPU suben con vos",
                       SCREEN_WIDTH / 2, 222, 19, Fade(COL_CREAM, 0.75f));

        Rectangle card{ SCREEN_WIDTH / 2.0f - 350.0f, 258.0f, 700.0f, 248.0f };
        DrawSoftPanel(card, PANEL_COLOR, PANEL_BORDER, 0.06f);

        UiTextCentered("INTEGRANTES", SCREEN_WIDTH / 2, 278, 15, Fade(COL_GOLD, 0.85f), FontStyle::Bold);
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 300, 18, COL_CREAM, FontStyle::Bold);

        const char* rules[] =
        {
            "Elegís personaje y subís la montaña por uno de los tres caminos.",
            "Cada casilla es distinta: evento, enemigo, tesoro, descanso u opcional.",
            "Tirás un D20 contra la dificultad; el 20 y el 1 naturales deciden solos.",
            "Si salís bien elegís una bendición; si no, recibís una maldición.",
            "A los enemigos los atacás por partes, eligiendo dónde pegar.",
            "En la cima está el jefe final: ganarle es llevarse la corona."
        };

        for (int i = 0; i < 6; ++i)
        {
            int y = 338 + i * 28;
            DrawCircle(static_cast<int>(card.x) + 34, y + 9, 4.0f, COL_MAGENTA_LIT);
            UiText(rules[i], static_cast<int>(card.x) + 58, y, 16, Fade(COL_CREAM, 0.9f));
        }

        UiTextCentered(TextFormat("Las %i cartas del diseño están implementadas", GetCardDefCount()),
                       SCREEN_WIDTH / 2, 528, 15, Fade(COL_CREAM, 0.55f));

        float glow = 0.75f + 0.25f * Pulse(time, 2.4f);
        UiTextCentered("[ENTER] comenzar", SCREEN_WIDTH / 2, 572, 28, Fade(COL_GOLD, glow), FontStyle::Bold);
        UiTextCentered("[F11] pantalla completa   ·   [TAB] cartas   ·   [I] mochila   ·   [M] silenciar",
                       SCREEN_WIDTH / 2, 622, 14, Fade(COL_CREAM, 0.45f));
    }

    // El orden final sale del jefe de la cima: primero los que le ganaron,
    // ordenados por cuánto le sacaron, y después los que no llegaron.
    static void DrawCharacterSelect(const GameState& game)
    {
        float time = static_cast<float>(GetTime());

        Rectangle band{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 96.0f };
        DrawRectangleRec(band, COL_MAGENTA);
        DrawWaveBand(band, Fade(COL_INK_DEEP, 0.55f), time, 4);

        UiTextCentered("ELEGÍ TU PERSONAJE", SCREEN_WIDTH / 2, 34, 30, COL_CREAM, FontStyle::Bold);

        UiTextCentered("los tres suben la misma montaña, cambia con qué arrancan",
                       SCREEN_WIDTH / 2, 118, 17, Fade(COL_CREAM, 0.7f));

        constexpr float CARD_W = 300.0f;
        constexpr float CARD_H = 348.0f;
        constexpr float CARD_Y = 164.0f;
        constexpr float CARD_GAP = 24.0f;

        int count = static_cast<int>(CharacterId::Count);
        float totalW = count * CARD_W + (count - 1) * CARD_GAP;
        float firstX = SCREEN_WIDTH * 0.5f - totalW * 0.5f;

        for (int i = 0; i < count; ++i)
        {
            const CharacterDef& def = GetCharacterDef(static_cast<CharacterId>(i));
            bool selected = (static_cast<int>(game.menuCharacter) == i);

            float lift = selected ? 8.0f + 3.0f * Pulse(time, 2.0f) : 0.0f;

            Rectangle card{ firstX + static_cast<float>(i) * (CARD_W + CARD_GAP),
                            CARD_Y - lift, CARD_W, CARD_H };

            Rectangle shadow = card;
            shadow.x += 5.0f; shadow.y += 9.0f;
            DrawRectangleRounded(shadow, 0.08f, 8, Fade(BLACK, 0.45f));

            DrawRectangleRounded(card, 0.08f, 8, Color{ 26, 36, 64, 255 });
            DrawRectangleRoundedLinesEx(card, 0.08f, 8, selected ? 4.0f : 2.0f,
                                        selected ? COL_GOLD : Fade(def.color, 0.7f));

            // Retrato de relleno: un escudo con la inicial. Simple a propósito.
            Rectangle art{ card.x + 14.0f, card.y + 14.0f, card.width - 28.0f, 150.0f };
            DrawRectangleRounded(art, 0.1f, 6, Fade(def.color, 0.14f));

            float cx = art.x + art.width * 0.5f;
            float cy = art.y + art.height * 0.5f;

            DrawCircleV(Vector2{ cx, cy }, 50.0f, Fade(def.color, 0.22f));
            DrawCircleV(Vector2{ cx, cy }, 42.0f, Fade(COL_INK_DEEP, 0.7f));
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 50.0f, Fade(def.color, 0.9f));

            UiTextCentered(TextFormat("%c", def.name[3]), static_cast<int>(cx),
                           static_cast<int>(cy) - 26, 54, def.color, FontStyle::Bold);

            UiTextCentered(def.name, static_cast<int>(card.x + card.width * 0.5f),
                           static_cast<int>(art.y + art.height) + 16, 22, COL_CREAM, FontStyle::Bold);

            UiTextCentered(def.title, static_cast<int>(card.x + card.width * 0.5f),
                           static_cast<int>(art.y + art.height) + 44, 15,
                           Fade(COL_CREAM, 0.6f));

            UiTextWrapped(def.passive, static_cast<int>(card.x) + 22,
                          static_cast<int>(art.y + art.height) + 76,
                          static_cast<int>(card.width) - 44, 17,
                          Fade(def.color, 0.95f), 3, FontStyle::Bold);

            Rectangle key{ card.x + card.width * 0.5f - 24.0f, card.y + card.height - 44.0f,
                           48.0f, 32.0f };
            DrawRectangleRounded(key, 0.4f, 6, Fade(COL_GOLD, selected ? 0.3f : 0.15f));
            DrawRectangleRoundedLinesEx(key, 0.4f, 6, 2.0f, Fade(COL_GOLD, selected ? 0.9f : 0.5f));

            UiTextCentered(TextFormat("%i", i + 1),
                           static_cast<int>(key.x + key.width * 0.5f),
                           static_cast<int>(key.y) + 4, 22, COL_GOLD, FontStyle::Bold);
        }

        float glow = 0.75f + 0.25f * Pulse(time, 2.4f);
        UiTextCentered("[1-3] elegir      [ENTER] empezar la subida",
                       SCREEN_WIDTH / 2, 566, 24, Fade(COL_GOLD, glow), FontStyle::Bold);
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 648, 15, Fade(COL_CREAM, 0.45f));
    }

    static void DrawItemMenu(const GameState& game)
    {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(COL_INK_DEEP, 0.85f));

        const Player& player = game.players[game.currentPlayer];

        Rectangle panel{ 300.0f, 150.0f, 680.0f, 420.0f };
        DrawSoftPanel(panel, PANEL_COLOR, PANEL_BORDER, 0.05f);

        Rectangle header{ panel.x, panel.y, panel.width, 40.0f };
        DrawRectangleRounded(header, 0.3f, 6, Fade(COL_MAGENTA, 0.4f));

        UiText("MOCHILA", static_cast<int>(panel.x) + 20, static_cast<int>(panel.y) + 10, 21,
               COL_CREAM, FontStyle::Bold);

        UiText("[I] cerrar", static_cast<int>(panel.x + panel.width) - 110,
               static_cast<int>(panel.y) + 13, 16, Fade(COL_CREAM, 0.8f));

        int count = static_cast<int>(player.items.size());

        if (count <= 0)
        {
            UiText("No tenés ningún ítem todavía.", static_cast<int>(panel.x) + 24,
                   static_cast<int>(panel.y) + 70, 19, Fade(COL_CREAM, 0.6f));
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            const ItemDef& def = GetItemDef(player.items[i]);

            Rectangle row{ panel.x + 16.0f, panel.y + 58.0f + static_cast<float>(i) * 76.0f,
                           panel.width - 32.0f, 68.0f };

            DrawRectangleRounded(row, 0.16f, 6, Fade(COL_INK_DEEP, 0.7f));
            DrawRectangleRoundedLinesEx(row, 0.16f, 6, 2.0f, Fade(COL_GOLD, 0.7f));

            UiText(TextFormat("[%i]", i + 1), static_cast<int>(row.x) + 18,
                   static_cast<int>(row.y) + 10, 19, COL_GOLD, FontStyle::Bold);

            UiText(def.name, static_cast<int>(row.x) + 64, static_cast<int>(row.y) + 9, 20,
                   COL_CREAM, FontStyle::Bold);

            UiTextWrapped(def.text, static_cast<int>(row.x) + 64, static_cast<int>(row.y) + 36,
                          static_cast<int>(row.width) - 90, 16, Fade(COL_CREAM, 0.8f), 1);
        }

        if (player.hasMap)
        {
            UiText("Mapa de la montaña: ya lo tenés, el tablero está revelado.",
                   static_cast<int>(panel.x) + 24,
                   static_cast<int>(panel.y + panel.height) - 40, 16, Fade(COL_GOLD, 0.8f));
        }
    }

    static void GetFinalRanking(const GameState& game, int ranking[])
    {
        for (int i = 0; i < PLAYER_COUNT; ++i) ranking[i] = i;

        for (int i = 0; i < PLAYER_COUNT - 1; ++i)
        {
            for (int j = 0; j < PLAYER_COUNT - 1 - i; ++j)
            {
                const Player& a = game.players[ranking[j]];
                const Player& b = game.players[ranking[j + 1]];

                int scoreA = (a.bossBeaten ? 1000 : 0) + a.bossParts * 100 + a.bossTurnsLeft * 10;
                int scoreB = (b.bossBeaten ? 1000 : 0) + b.bossParts * 100 + b.bossTurnsLeft * 10;

                if (scoreB > scoreA)
                {
                    int swap         = ranking[j];
                    ranking[j]       = ranking[j + 1];
                    ranking[j + 1]   = swap;
                }
            }
        }
    }

    static void DrawFinalScreen(const GameState& game)
    {
        float time = static_cast<float>(GetTime());

        Rectangle band{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 96.0f };
        DrawRectangleRec(band, COL_MAGENTA);
        DrawWaveBand(band, Fade(COL_INK_DEEP, 0.55f), time, 3);

        int ranking[PLAYER_COUNT];
        GetFinalRanking(game, ranking);

        UiTextCentered("FIN DE LA PARTIDA", SCREEN_WIDTH / 2, 36, 19, Fade(COL_CREAM, 0.85f), FontStyle::Bold);

        if (game.kingId >= 0)
        {
            const Player& king = game.players[game.kingId];

            DrawCrown(SCREEN_WIDTH / 2, 128, 64.0f + 4.0f * Pulse(time, 1.8f), COL_GOLD);

            UiTextCentered("REY DE LA COLINA", SCREEN_WIDTH / 2, 164, 46, COL_GOLD, FontStyle::Bold);
            UiTextCentered(TextFormat("JUGADOR %i  ·  %s", game.kingId + 1,
                                      GetCharacterDef(king.character).name),
                           SCREEN_WIDTH / 2, 218, 30, king.color, FontStyle::Bold);
        }
        else
        {
            UiTextCentered("LA COLINA SE QUEDA SIN REY", SCREEN_WIDTH / 2, 150, 40,
                           COL_CURSE, FontStyle::Bold);
            UiTextCentered("Nadie pudo con el jefe de la cima.", SCREEN_WIDTH / 2, 206, 20,
                           Fade(COL_CREAM, 0.75f));
        }

        Rectangle panel{ SCREEN_WIDTH / 2.0f - 350.0f, 266.0f, 700.0f, 232.0f };
        DrawSoftPanel(panel, PANEL_COLOR, PANEL_BORDER, 0.06f);

        UiText("CÓMO LES FUE CONTRA EL JEFE", static_cast<int>(panel.x) + 24,
               static_cast<int>(panel.y) + 14, 16, COL_GOLD, FontStyle::Bold);

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[ranking[i]];
            int rowY = static_cast<int>(panel.y) + 48 + i * 44;

            Rectangle row{ panel.x + 12.0f, static_cast<float>(rowY) - 4.0f, panel.width - 24.0f, 40.0f };
            DrawRectangleRounded(row, 0.3f, 6,
                                 (player.id == game.kingId) ? Fade(COL_GOLD, 0.14f)
                                                            : Fade(COL_INK_DEEP, 0.35f));

            UiText(TextFormat("%iº", i + 1), static_cast<int>(panel.x) + 28, rowY + 6, 20,
                   (player.id == game.kingId) ? COL_GOLD : COL_CREAM_DIM, FontStyle::Bold);

            DrawCircle(static_cast<int>(panel.x) + 78, rowY + 16, 12.0f, player.color);
            UiText(TextFormat("%i", player.id + 1), static_cast<int>(panel.x) + 74, rowY + 8, 15,
                   COL_INK_DEEP, FontStyle::Bold);

            UiText(PlayerLabel(player),
                   static_cast<int>(panel.x) + 104, rowY + 2, 18, COL_CREAM, FontStyle::Bold);

            UiText(GetCharacterDef(player.character).name,
                   static_cast<int>(panel.x) + 104, rowY + 22, 14,
                   Fade(GetCharacterDef(player.character).color, 0.9f));

            UiText(player.bossBeaten
                       ? TextFormat("venció al jefe  ·  %i partes derribadas", player.bossParts)
                       : TextFormat("cayó ante el jefe  ·  %i partes derribadas", player.bossParts),
                   static_cast<int>(panel.x) + 300, rowY + 4, 15,
                   player.bossBeaten ? COL_BLESSING : Fade(COL_CREAM, 0.6f));

            UiText(TextFormat("%i bendiciones   %i maldiciones   %i ítems",
                              CountCards(player, CardKind::Blessing),
                              CountCards(player, CardKind::Curse),
                              static_cast<int>(player.items.size())),
                   static_cast<int>(panel.x) + 300, rowY + 24, 14, Fade(COL_CREAM, 0.6f));
        }

        float glow = 0.75f + 0.25f * Pulse(time, 2.4f);
        UiTextCentered("[R] jugar de nuevo", SCREEN_WIDTH / 2, 524, 26, Fade(COL_GOLD, glow), FontStyle::Bold);
        UiTextCentered("[ESC] salir del juego", SCREEN_WIDTH / 2, 562, 17, Fade(COL_CREAM, 0.6f));
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 648, 15, Fade(COL_CREAM, 0.45f));
    }

    static void DrawCardsOverlay(const GameState& game)
    {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(COL_INK_DEEP, 0.88f));

        const Player& player = game.players[game.cardsShown];

        Rectangle panel{ 120.0f, 80.0f, SCREEN_WIDTH - 240.0f, SCREEN_HEIGHT - 160.0f };
        DrawSoftPanel(panel, PANEL_COLOR, PANEL_BORDER, 0.04f);

        Rectangle header{ panel.x, panel.y, panel.width, 38.0f };
        DrawRectangleRounded(header, 0.3f, 6, Fade(COL_MAGENTA, 0.4f));

        UiText(TextFormat("CARTAS DE %s", PlayerLabel(player)),
               static_cast<int>(panel.x) + 18, static_cast<int>(panel.y) + 9, 20,
               COL_CREAM, FontStyle::Bold);

        UiText("[1-4] ver otro jugador      [TAB] cerrar",
               static_cast<int>(panel.x + panel.width) - 330, static_cast<int>(panel.y) + 12, 15,
               Fade(COL_CREAM, 0.8f));

        if (player.cards.empty())
        {
            UiText("Todavía no juntó ninguna carta.",
                   static_cast<int>(panel.x) + 24, static_cast<int>(panel.y) + 64, 18,
                   Fade(COL_CREAM, 0.6f));
            return;
        }

        for (int i = 0; i < static_cast<int>(player.cards.size()); ++i)
        {
            const CardInstance& card = player.cards[i];
            const CardDef& def = GetCardDef(card.defId);

            bool isBlessing = (def.kind == CardKind::Blessing);
            Color color = isBlessing ? COL_BLESSING : COL_CURSE;
            Color levelColor = GetPathColor(static_cast<PathKind>(static_cast<int>(def.level)));

            Rectangle row{ panel.x + 16.0f, panel.y + 52.0f + static_cast<float>(i) * 74.0f,
                           panel.width - 32.0f, 68.0f };

            DrawRectangleRounded(row, 0.14f, 6, Fade(COL_INK_DEEP, 0.7f));
            DrawRectangleRoundedLinesEx(row, 0.14f, 6, 2.0f, Fade(color, card.spent ? 0.3f : 0.85f));
            DrawRectangleRounded(Rectangle{ row.x + 6.0f, row.y + 6.0f, 6.0f, row.height - 12.0f },
                                 0.8f, 4, Fade(levelColor, card.spent ? 0.35f : 1.0f));

            float dim = card.spent ? 0.45f : 1.0f;

            UiText(def.name, static_cast<int>(row.x) + 24, static_cast<int>(row.y) + 6, 19,
                   Fade(color, dim), FontStyle::Bold);

            UiText(GetCardLevelName(def.level),
                   static_cast<int>(row.x + row.width) - 230, static_cast<int>(row.y) + 9, 15,
                   Fade(levelColor, dim * 0.9f), FontStyle::Bold);

            UiText(card.spent ? "ya se usó" : "sin usar",
                   static_cast<int>(row.x + row.width) - 110, static_cast<int>(row.y) + 9, 15,
                   card.spent ? Fade(COL_CREAM, 0.4f) : COL_GOLD, FontStyle::Bold);

            UiTextWrapped(def.text, static_cast<int>(row.x) + 24, static_cast<int>(row.y) + 32,
                          static_cast<int>(row.width) - 48, 16, Fade(COL_CREAM, dim * 0.85f), 2);
        }
    }

    void DrawGame(const GameState& game)
    {
        DrawBackdrop(static_cast<float>(GetTime()));

        if (game.phase == GamePhase::Menu)
        {
            DrawMenu();
            return;
        }

        if (game.phase == GamePhase::CharacterSelect)
        {
            DrawCharacterSelect(game);
            return;
        }

        if (game.phase == GamePhase::Victory)
        {
            DrawFinalScreen(game);
            return;
        }

        DrawRectangle(0, 0, SCREEN_WIDTH, 76, Fade(COL_INK_DEEP, 0.55f));
        DrawTitle(206, 16, 26);
        UiText(CREDITS_LINE, 42, 52, 14, Fade(COL_CREAM, 0.45f));

        if (IsAudioMuted())
        {
            UiText("SIN SONIDO [M]", SCREEN_WIDTH - 210, 54, 14, Fade(COL_CREAM, 0.55f));
        }

        UiText("[F11] pantalla completa", 42, SCREEN_HEIGHT - 22, 14, Fade(COL_CREAM, 0.35f));

        UiText(TextFormat("Etapa %i / %i",
                          (game.currentStage < STAGE_COUNT ? game.currentStage + 1 : STAGE_COUNT),
                          STAGE_COUNT),
               SCREEN_WIDTH - 210, 24, 21, Fade(COL_CREAM, 0.85f), FontStyle::Bold);

        const Player& turnPlayer = game.players[game.currentPlayer];

        DrawBoard(game.currentStage, turnPlayer.hasMap);

        bool enemyPhase = IsEnemyPhase(game);

        bool exploring = (game.phase == GamePhase::PathSelection ||
                          game.phase == GamePhase::EventRoll ||
                          game.phase == GamePhase::EventChoice ||
                          game.phase == GamePhase::EventResult ||
                          game.phase == GamePhase::CardSelection ||
                          game.phase == GamePhase::OptionalOffer ||
                          game.phase == GamePhase::TreasureResult ||
                          game.phase == GamePhase::RestResult ||
                          enemyPhase);

        DrawPlayerTokens(game.players, exploring ? game.currentPlayer : -1);

        if (enemyPhase) DrawEnemyFigure(game);

        DrawPlayerList(game);

        bool busyPanel = (game.phase == GamePhase::CardSelection ||
                          game.phase == GamePhase::EventChoice ||
                          game.phase == GamePhase::EnemyChoice);

        if (busyPanel)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(COL_INK_DEEP, 0.78f));
            DrawTurnPanel(game);
        }
        else
        {
            DrawTurnPanel(game);
            if (!enemyPhase) DrawLog(game);
        }

        DrawBanner(game);

        UiText("[TAB] ver las cartas", SCREEN_WIDTH - 230, SCREEN_HEIGHT - 22, 14,
               Fade(COL_CREAM, 0.35f));

        if (!turnPlayer.isCpu && !turnPlayer.items.empty())
        {
            UiText(TextFormat("[I] mochila (%i)", static_cast<int>(turnPlayer.items.size())),
                   430, SCREEN_HEIGHT - 22, 14, Fade(COL_GOLD, 0.7f));
        }

        if (game.phase == GamePhase::ItemMenu) DrawItemMenu(game);

        if (game.showCards) DrawCardsOverlay(game);
    }
}
