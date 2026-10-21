# Blackjack (C++ command line)

A terminal blackjack game with a basic-strategy coach, a Hi-Lo card counting trainer with timed drills, fractional-Kelly
bet sizing, a daily challenge with a local leaderboard and achievements, configurable table rules, multi-line card art
with a dealing animation, sound, session summaries and saved progress. Plain C++14, no dependencies.

## Build and run

```sh
make run                  # build (if needed) and play
make                      # build only; produces ./blackjack
make clean                # remove build output
./blackjack --seed 42     # a fixed shuffle (see Challenge and seeds)
./blackjack --help
```

Needs a C++14 compiler and `make`. In VS Code, the Code Runner button runs `make -s run` (see `.vscode/settings.json`).

**Compiler flags:** the Makefile adds `-isysroot`/`-isystem` flags that point at the macOS SDK. They work around a
broken Command Line Tools install on the machine this was written on, where `#include <iostream>` isn't found.
On any other machine, or after reinstalling the Command Line Tools, build with:

```sh
make SDK_FLAGS=
```

## Docker

A `Dockerfile` builds the game on Debian and can hand you either a runnable image or the plain files:

```sh
# build and play in a container (-it is needed for input and colors)
docker build -t blackjack .
docker run -it --rm blackjack

# export the Linux build as files (the executable plus assets/) into ./dist
docker build --target export --output type=local,dest=dist .

# export the image itself
docker save blackjack | gzip > blackjack-image.tar.gz
```

Notes:

- The exported executable is a **Linux** program, so it won't run directly on macOS. It links the C++ runtime
  statically but still needs a glibc at least as new as Debian 12's. Keep `assets/` next to it.
- Docker builds for your machine's architecture by default (arm64 on Apple Silicon). For an x86-64 Linux target, add
  `--platform linux/amd64` to the `docker build` command.
- Sound is unavailable inside a container (there is no `afplay` and no audio device), and Settings says so. Colors, card
  art and the animation work when you run with `-it`.
- Progress is saved inside the container, so it is lost with `--rm`. To keep it, mount a folder and point
  `BLACKJACK_SAVE` at a file in it, for example
  `docker run -it --rm -v "$PWD/data:/data" -e BLACKJACK_SAVE=/data/save blackjack` (the folder must be writable by
  the container's `player` user).
- The Makefile only applies the macOS SDK workaround on a Mac that has that SDK, so the same Makefile builds on Linux.

## Rules

| | |
|---|---|
| Shoe | 6 decks by default (1–8 in Settings); reshuffled after about 75% is dealt |
| Dealer | Stands on all 17s by default; the hole card is always revealed at the end of a round |
| Blackjack | Pays 3:2 by default (rounded down to a whole chip) |
| Double down | On your first two cards, including after a split |
| Split | Two cards of equal value (so 10 and K can split); up to 4 hands; split aces get one card each; 21 after a split pays 1:1 |
| Insurance | Offered when the dealer shows an Ace; costs half your bet and pays 2:1 if the dealer has blackjack |
| Options | Dealer hits soft 17, blackjack pays 6:5, late surrender, resplit aces (see **Table rules**) |

You start with 1,000 chips (changeable in Settings). The session ends when you quit, run out of chips, or fall below the
minimum bet.

## Controls

| Prompt | Keys |
|---|---|
| Main menu | `1` play, `2` counting practice, `3` settings, `4` statistics, `5` training drills, `6` challenge, `7` leaderboard, `8` achievements, `q` quit |
| Bet | a whole number of chips (at least the minimum bet), or a shortcut (below), `k` for Kelly advice, `q` to leave the table |
| Your turn | `h` hit, `s` stand, `d` double, `p` split, `r` surrender (only the options currently allowed are offered) |
| Insurance | `y` or `n` (Enter means no) |

Prompts take a single character, so typing a whole word such as `split` is rejected rather than being read as `s`.
When a round has several hands, the prompt names the hand it is for ("Hand 2: ...").

**Bet shortcuts:**

| Type | Bets |
|---|---|
| Enter | the same as your last bet (capped at your chips) |
| `m` | the minimum bet |
| `a` | all your chips |
| `kk` | the Kelly-suggested bet (`k` alone just shows the advice) |

Each shortcut prints what it bet, for example `Betting 25 (minimum).`.

**Leaving:** `q` at the bet prompt, or Ctrl-D, leaves the table normally: you get the session summary and the session is
saved. Ctrl-C quits immediately, so a session still in progress isn't counted.

## Settings

- **Decks** in the shoe (1–8)
- **Suit symbols**: Unicode (♠ ♥ ♦ ♣) or ASCII (S H D C). Use ASCII if your terminal doesn't show the Unicode suits.
- **Kelly fraction**: how much of the full Kelly stake to bet (5–100%, default 50%)
- **Starting chips** (10–1,000,000)
- **Minimum bet** (1 up to the starting chips, default 1): the smallest opening bet. Doubling, splitting and insurance
  aren't limited by it. If your chips ever drop below it the session ends. Lowering the starting chips below the
  minimum bet lowers the minimum bet to match.
- **Colors**, **Music**, **Sound effects**: on or off (see below)
- **Card art** and **Dealing animation**: on or off (see below)
- **Basic-strategy coach**: off, grading only, or hints and grading (see below)
- **Table rules**: opens the rules screen (see below)

Settings are saved between runs (see **Saved progress**).

## Table rules

Settings → Table rules lets you change four rules and shows the estimated house edge for the current table:

| Rule | Default | Effect on the house edge |
|---|---|---|
| Dealer on soft 17 | stands | hitting adds about +0.22% |
| Blackjack pays | 3:2 | 6:5 adds about +1.39% |
| Late surrender | off | allowing it takes off about 0.07% |
| Resplit aces | off | allowing it takes off about 0.08% |

- **Late surrender** is offered as `r` on your first decision with a two-card hand, before any split or hit, once the
  dealer has checked for blackjack. You lose half your bet (rounded in the house's favor), and the dealer's hole card is
  turned over.
- **Resplit aces:** a split ace still gets just one card, but if that card is another ace you may split again (up to 4
  hands). The hand's prompt is limited to stand or split.
- The house edge is a rough estimate against perfect basic strategy: about 0.40% at 6 decks with the default rules,
  starting from `0.45% − 0.30% ÷ decks`. The adjustments above are commonly quoted rules of thumb, not exact values. It
  feeds the Kelly estimate, so a worse table gives smaller suggested bets. The intro to each session shows it.

## Basic-strategy coach

Settings → Basic-strategy coach has three settings:

| Mode | What you see |
|---|---|
| off | nothing (the default) |
| grading only | after each play, whether it matched basic strategy |
| hints and grading | the recommended play before you choose, then the grade |

```
Coach: Double down (hard 11 vs 6).
[H]it  [S]tand  [D]ouble: Coach: correct.
...
Coach: Stand (hard 16 vs 7).
[H]it  [S]tand: Coach: basic strategy says Stand (hard 16 vs 7), not Hit.
```

- The advice is limited to what you are allowed to do at that moment: "double if allowed, otherwise hit" comes back as
  Double or Hit. It also covers splits, soft hands, late surrender and the locked-pair prompt after a resplit.
- The chart is the usual multi-deck chart with double after split, adjusted for the dealer's soft-17 rule and for
  surrender. For 11 against an Ace it doubles if the dealer hits soft 17 or the shoe has one or two decks, and hits
  otherwise. It never splits tens. I checked it against exact expected-value calculations: the only cells where an
  infinite-deck calculation disagrees are marginal, deck-dependent ones such as A,2 against a 5, where a real shoe
  favors the standard play.
- **Insurance** is graded too: basic strategy says to decline it unless the Hi-Lo true count is +3 or more. The count
  the coach uses leaves out the dealer's face-down card, so it never gives that away.
- Your accuracy and best streak show in the session summary and the Statistics screen, and only plays made with the
  coach on are counted.

**It also corrects the Kelly numbers.** The Kelly estimate assumes perfect basic strategy, so once the coach has graded
at least 20 of your plays, the estimate subtracts an allowance for your mistakes:

```
penalty = mistakes × 0.07 ÷ rounds played with the coach on      (lifetime plus this session)
```

The 0.07 is a rough average cost of one mistake, as a fraction of the bet, so treat the result as an approximation.
When it applies, the Kelly line says so ("estimated edge -1.10% (after 0.70% lost to strategy slips)"), and suggested
bets shrink. It is switched off in a challenge.

## Training drills

Main menu → Training drills. Each drill has five levels; Enter picks the next level you haven't passed. Passing a level
is saved, and a few achievements depend on them.

| Drill | What it asks | Levels | Pass |
|---|---|---|---|
| Speed count | Cards are flashed one at a time and you give the Hi-Lo running count | 1: 10 cards at 2.0 s · 2: 20 cards at 1.5 s · 3: a whole deck with one card hidden (51 cards) at 1.2 s · 4: the same at 0.8 s · 5: the same at 0.5 s | the count is exactly right |
| True-count conversion | Ten questions: a running count and the decks left, answer the true count | whole decks (30 s each) → half decks (15 s) → quarter decks down to 0.75 (8 s) | 8 of 10 correct and in time |
| Bet sizing | Ten questions: a true count, a bankroll and a Kelly fraction, answer the bet | the allowed error narrows from 25% to 5% and the time from 40 s to 15 s | 8 of 10 correct and in time |

- In the hidden-card levels a whole deck counts to 0, so the cards you see always add up to minus the hidden card's
  value. The game shows the hidden card afterwards.
- The flash speed is real only in a terminal; when output is piped the cards print at once.
- The time limits are measured from when the question appears to when you press Enter. An answer that arrives late
  counts as wrong, and the game tells you how long you took.
- The bet-sizing answers come from the same formula as the Kelly advice, using your table's decks and rules, so the
  drill shows the working: edge, full Kelly, your fraction, the stake and the bet.

## Challenge, seeds and the leaderboard

**`--seed N`** shuffles every shoe, and the drills' questions, from the number N, so a game repeats exactly for the same
plays. Without it, shuffles are random.

**The challenge** (main menu → Challenge) is a fixed puzzle: 25 rounds on one shoe, with standard rules, 6 decks, a
1,000-chip stack and a 10-chip minimum, whatever your own settings are. The coach and the Kelly advice are switched
off. Everyone who plays the same shoe gets the same cards, so scores are comparable.

- The shoe is today's date (so it changes daily and is the same for everyone) unless you started the game with
  `--seed N`, in which case N is the shoe. That lets friends compete on a seed they pick.
- Finishing all 25 rounds, or going broke, scores your chips onto the leaderboard. Leaving early doesn't.
- In a terminal you're asked for up to three initials; piped runs use `YOU`.
- The leaderboard (main menu → Leaderboard) shows the best scores for today's shoe and the best of all time, and keeps
  the top 50. A tie ranks below the earlier score.
- `BLACKJACK_TODAY=YYYYMMDD` overrides today's date, for replaying an old day's shoe or for testing.
- A challenge still counts toward your session statistics.

## Achievements

Main menu → Achievements lists them. They are announced when you earn them and saved.

| Achievement | How |
|---|---|
| Natural | Be dealt a blackjack. |
| Hot streak | Give 10 correct running counts in a row. |
| Deck master | Pass the speed count at level 3. |
| Quick hands | Pass the speed count at level 5. |
| Math whiz | Pass true-count conversion at level 3. |
| Right size | Pass bet sizing at level 3. |
| By the book | Make 25 basic-strategy plays in a row with the coach on. |
| Sharp | Finish a session with at least 50 coached plays and 95% of them correct. |
| Marathon | Play 100 rounds in one session. |
| Comeback | End a session in profit after falling to half your starting chips. |
| High roller | Win 500 chips or more in one session. |
| Challenger | Finish a challenge. |
| Top of the board | Beat an earlier score on a challenge and take first place. |
| Veteran | Play 500 rounds in total. |

## Session summary

When you leave the table the game shows how the session went:

```
Session summary
  Rounds: 24   Wins 11 | Losses 10 | Pushes 3   (win rate 46%)
  Blackjacks: 2
  Strategy: 31/36 plays matched basic strategy (86%), best streak 12
  Biggest win: +75   Biggest loss: -50
  Net: +120   (1000 → 1120)   Peak 1190   Low 940
  Bankroll: ▁▂▃▅▇▆▅▆▇
```

A round counts as a win, loss or push by its net chips (including insurance and splits). The strategy line appears when
the coach was on. The bankroll chart shows your chips after every round, scaled between the session's low and high and
shortened to 40 points; it uses ASCII characters when Suit symbols is set to ASCII. If the session beats your best
session or highest bankroll so far, it says so.

## Saved progress

The game saves your settings and your profile: lifetime statistics, drill records, achievements and the leaderboard.
The main menu's **Statistics** screen shows the lifetime totals: sessions, rounds, wins, losses and pushes, blackjacks,
biggest win and loss, total net, best session, peak bankroll, basic-strategy accuracy and counting-practice accuracy.
Statistics → Reset erases those totals (after a confirmation); settings, drill records, achievements and the
leaderboard are kept.

Saving is on only in an interactive terminal, so piping the game or running it from a script never reads or writes a
file. The file is `~/.blackjack_save`, a small text file you can read and edit.

| | |
|---|---|
| `BLACKJACK_SAVE=/path` | use this file, in any mode (handy for tests) |
| `NO_SAVE=1` | don't read or write a save file |

A missing, damaged or hand-edited file can't stop the game from starting: anything unreadable or out of range is ignored
and keeps its default. The file is written to a temporary name and renamed, and it is never read or replaced if it isn't
a regular file (so `BLACKJACK_SAVE=/dev/null` safely turns saving off).

## Card art and the dealing animation

In a terminal the table is drawn with multi-line cards (a face-down card is shaded):

```
  Dealer: (13)
  ┌─────┐ ┌─────┐
  │A    │ │2    │
  │  ♦  │ │  ♥  │
  │    A│ │    2│
  └─────┘ └─────┘
  You: (18)
  ┌─────┐ ┌─────┐ ┌─────┐
  │4    │ │3    │ │A    │
  │  ♥  │ │  ♦  │ │  ♣  │
  │    4│ │    3│ │    A│
  └─────┘ └─────┘ └─────┘
```

With the dealing animation on, the cards of the opening deal and the dealer's hits appear one at a time, in place, in
sync with the card sounds. A split's cards appear the same way. It works by moving the cursor, so it needs colors on, and
only the dealer's final frame stays on screen.

Both are on by default in a terminal and off when output is piped, where cards print in the compact `[A♦] [4♥] (7)` style.
Turn either one on or off from Settings. Between consecutive cards the game pauses a fifth of a second when the animation
or sound effects are on.

## Sound

The background track loops for as long as the game is running, and short effects play on actions:

| Sound | When | File in `assets/` |
|---|---|---|
| Background music (looped) | the whole time, including menus | `Midnight_Bluff.mp3` |
| Card | every card drawn: each card of the deal, hits, doubles, each card dealt by a split, and each dealer hit | `oxidvideos-taking-playing-card-2-522516.mp3` |
| Shuffle | sitting down at the table and every reshuffle of the shoe | `oxidvideos-shuffling-deck-of-cards-522518.mp3` |
| Chips | a bet goes down, and when you double, split or buy insurance | `chips_placement.mp3` |
| Win | a round where you come out ahead | `freesound_community-player-wins-94889.mp3` |
| Blackjack | you are paid a natural blackjack | `blackjack.mp3` |
| Push | a round that ties | `push.mp3` |
| Dealer wins | the dealer beats you at the showdown | `dealer_wins.mp3` |
| Bust | one of your hands goes over 21, on a hit or a double | `player_busts.mp3` |
| Dealer takes chips | you surrender, so the dealer simply takes half your bet | `dealer_takes_chips.mp3` |
| Failure | every round you lose, on top of how you lost | `freesound_community-failure-2-89169.mp3` |

A win, a blackjack or a push plays one outcome sound. A loss adds the failure sound to how you lost: the dealer winning at
the showdown, the dealer taking your chips when you surrender, or, when every hand busted, the bust sound that already
played the moment you went over 21. Going broke needs no separate sound. `old_fail_sound.mp3` isn't used.

Sounds are played by macOS's built-in `afplay`, so there is nothing extra to install, but on other systems sound is
reported as unavailable in Settings. The `assets/` folder is found next to the `blackjack` executable (or in
`./assets`), so the game works from any directory. A missing file just means that one sound is silent.

Sound is on by default only in an interactive terminal, so piping the game to a file or another program stays silent.
Turn music and effects on or off from Settings. The music stops when you quit, press Ctrl-C, or the game is killed.

| | |
|---|---|
| `NO_SOUND=1` | start with sound off |
| `BLACKJACK_ASSETS=/path` | look for the sound files in this folder first |
| `BLACKJACK_PLAYER=/path` | use another player program; it is run as `player -v VOLUME FILE` |

The sound files come from third-party sources, so check their licenses before redistributing the project.

## Colors

In a color-capable terminal the game draws cards as white faces with red or black suits (the hidden hole card is blue),
shows wins in green, losses in red and pushes in yellow, and highlights the keys in prompts and menus. Totals turn red
when a hand busts, and the count and Kelly numbers are green when positive and red when negative.

Color turns on automatically only when the output is a terminal, so piping the game to a file or another program gives
plain text. You can override that:

| | |
|---|---|
| `NO_COLOR=1` | never use color |
| `CLICOLOR_FORCE=1` | use color even when output is piped (`NO_COLOR` still wins) |
| `TERM=dumb` | no color |
| Settings → Colors | toggle color |

With color off the banner is a plain line, the dealing animation is off, and compact cards are shown as `[10♥]`.

## Card counting practice

Choose **Card counting practice** from the main menu. Before every bet you're asked for the Hi-Lo running count of all
cards dealt since the last shuffle:

| Cards | Count |
|---|---|
| 2, 3, 4, 5, 6 | +1 |
| 7, 8, 9 | 0 |
| 10, J, Q, K, A | −1 |

After you answer, the game shows the real running count, the decks remaining, the true count (running count ÷ decks
remaining), and your accuracy and streak. The count resets to 0 whenever the shoe is reshuffled. Your counting accuracy
is added to the lifetime statistics. For timed practice see **Training drills**.

## Fractional Kelly bet sizing

The Kelly criterion sizes a bet as a share of your bankroll based on your edge. The game estimates that edge from the
true count:

```
edge        = −(house edge for your decks and rules) + 0.5% × true count − (strategy penalty, if the coach has data)
full Kelly  = edge ÷ 1.32            (1.32 ≈ variance of one blackjack hand)
suggested   = chips × full Kelly × your Kelly fraction, rounded, never below the minimum bet
```

When the estimated edge is zero or negative, Kelly says not to bet, and the game suggests the minimum bet.

It is shown automatically in counting practice, and in any mode you can type `k` at the bet prompt (or `kk` to place that
bet). With 6 decks and the default rules the estimated edge only turns positive above a true count of about +0.8. At
1,000 chips and half Kelly, a true count of +4 suggests about 6 chips.

**Treat these numbers as rough.** The 0.5%-per-count figure, the house edge by deck count and rules, the 1.32
variance and the cost of a strategy mistake are rules of thumb, not exact values. Without the coach the estimate assumes
you play perfect basic strategy, and if you play worse than that your real edge is lower and Kelly will suggest bets
that are too large.

## Project layout

```
main.cpp              entry point: command line, loading and saving, the main menu
Makefile
Dockerfile            build/run/export in a container (.dockerignore keeps local build output out)
assets/               music and sound effects (mp3)
src/                  included as "folder/name.h" (the Makefile adds -Isrc)
  core/               the game itself, with no input or output
    card.*              Card and Hand
    shoe.*              Shoe, with the Hi-Lo running count
    rules.*             table rules, house edge estimate, blackjack payout
    strategy.*          the basic-strategy chart and the insurance rule
    seed.*              the shuffle seed (--seed)
  config/             settings.* (Settings and which features are active), toggle.h
  platform/           audio.* (music and effects), terminal.* (interactive-terminal detection)
  ui/                 everything printed or typed
    style.*             ANSI colors
    format.*            number and percent formatting
    input.*             prompts and parsing
    table.*             card art and the table display with in-place animation
    pacing.*            the short pause between cards
  game/               playing blackjack
    game_round.*        one round: insurance, split, double, surrender, dealer, payouts, coach hooks
    session.*           a sitting at the table, bet shortcuts, the sound for each outcome
    kelly.*             edge estimate and Kelly bet sizing
    challenge.*         the daily / seeded challenge
  training/           counting.* (count quiz), coach.* (hints, grading, the strategy penalty), drills.*
  progress/           what is remembered
    stats.*             session summary, lifetime statistics, bankroll chart
    profile.h           statistics, drill records, achievements and leaderboard in one place
    achievements.*      the achievement list and what unlocks each
    leaderboard.*       challenge scores
    save.*              reading and writing the save file
  app/                cli.* (the command line), menu.* (menus and screens)
```

Everything lives in the `bj` namespace. Each module exposes a small header and keeps its helpers private to its `.cpp`.
