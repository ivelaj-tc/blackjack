#include "platform/audio.h"

#include "platform/terminal.h"

#ifdef _WIN32

// No audio backend on Windows: every call is a no-op and sound reports itself as unavailable.
namespace bj {
namespace audio {

void init(Toggle, Toggle) {}
bool available() { return false; }
bool musicEnabled() { return false; }
void setMusicEnabled(bool) {}
bool effectsEnabled() { return false; }
void setEffectsEnabled(bool) {}
void play(Sound) {}

}  // namespace audio
}  // namespace bj

#else

#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

extern char** environ;

namespace bj {
namespace audio {
namespace {

// Sounds are played by an external command-line player that accepts `-v VOLUME FILE` (macOS's afplay does).
// BLACKJACK_PLAYER overrides it and BLACKJACK_ASSETS overrides where the sound files are looked for.
#ifdef __APPLE__
const char* const kDefaultPlayer = "/usr/bin/afplay";
#else
const char* const kDefaultPlayer = "";
#endif

const char* const kMusicFile = "Midnight_Bluff.mp3";
// Indexed by Sound.
const char* const kEffectFiles[] = {
    "oxidvideos-taking-playing-card-2-522516.mp3",    // Card
    "oxidvideos-shuffling-deck-of-cards-522518.mp3",  // Shuffle
    "freesound_community-player-wins-94889.mp3",      // Win
    "blackjack.mp3",                                  // Blackjack
    "push.mp3",                                       // Push
    "dealer_wins.mp3",                                // DealerWins
    "dealer_takes_chips.mp3",                         // TakesChips
    "chips_placement.mp3",                            // ChipsPlace
    "freesound_community-failure-2-89169.mp3",        // Failure
    "player_busts.mp3",                               // PlayerBusts
};
const std::size_t kNumEffects = sizeof(kEffectFiles) / sizeof(kEffectFiles[0]);

const char* const kMusicVolume = "0.35";
const char* const kEffectVolume = "0.8";
const std::size_t kMaxOverlappingEffects = 8;

std::string player;  // empty when no player was found
std::string musicPath;  // empty when the file is missing
std::string effectPaths[kNumEffects];
bool musicOn = false;
bool effectsOn = false;
volatile pid_t musicPid = 0;  // leader of the music loop's process group; read from a signal handler
std::vector<pid_t> effectPids;

bool fileExists(const std::string& path) { return access(path.c_str(), R_OK) == 0; }

bool envSet(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0';
}

std::string executableDir() {
    char buf[4096];
#ifdef __APPLE__
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return "";
#elif defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return "";
    buf[n] = '\0';
#else
    return "";
#endif
    char* resolved = realpath(buf, nullptr);
    std::string path = resolved != nullptr ? resolved : buf;
    std::free(resolved);
    std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

bool dirHasSounds(const std::string& dir) {
    if (fileExists(dir + "/" + kMusicFile)) return true;
    for (const char* file : kEffectFiles) {
        if (fileExists(dir + "/" + file)) return true;
    }
    return false;
}

// Looks next to the executable first, so the game works from any directory, then in ./assets.
std::string findAssetDir() {
    std::vector<std::string> candidates;
    if (envSet("BLACKJACK_ASSETS")) candidates.push_back(std::getenv("BLACKJACK_ASSETS"));
    std::string exeDir = executableDir();
    if (!exeDir.empty()) candidates.push_back(exeDir + "/assets");
    candidates.push_back("assets");

    for (const std::string& dir : candidates) {
        if (dirHasSounds(dir)) return dir;
    }
    return "";
}

std::string findPlayer() {
    std::string path = envSet("BLACKJACK_PLAYER") ? std::getenv("BLACKJACK_PLAYER") : kDefaultPlayer;
    return !path.empty() && access(path.c_str(), X_OK) == 0 ? path : "";
}

bool effectsAvailable() {
    if (player.empty()) return false;
    for (const std::string& path : effectPaths) {
        if (!path.empty()) return true;
    }
    return false;
}

// Starts `program` with its input and output on /dev/null so it can't disturb the terminal.
// With ownGroup it leads a new process group, which lets us stop it and everything it spawns at once.
// Returns 0 if it couldn't be started.
pid_t spawnQuiet(const std::string& program, std::vector<std::string> args, bool ownGroup) {
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    if (ownGroup) {
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
        posix_spawnattr_setpgroup(&attr, 0);
    }

    std::vector<char*> argv;
    for (std::string& arg : args) argv.push_back(&arg[0]);
    argv.push_back(nullptr);

    pid_t pid = 0;
    int rc = posix_spawn(&pid, program.c_str(), &actions, &attr, argv.data(), environ);
    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);
    return rc == 0 ? pid : 0;
}

void startMusic() {
    if (musicPid > 0 || player.empty() || musicPath.empty()) return;

    // Replays the track until this process is gone. While a track plays it checks every quarter second that
    // we're still alive and stops the player if not, so the music can't outlive a crash or kill -9. If the
    // player fails, the sleep keeps the loop from spinning the CPU.
    static const char* const script =
        "while kill -0 \"$PPID\" 2>/dev/null; do\n"
        "  \"$1\" -v \"$3\" \"$2\" &\n"
        "  p=$!\n"
        "  while kill -0 \"$p\" 2>/dev/null; do\n"
        "    kill -0 \"$PPID\" 2>/dev/null || { kill \"$p\"; exit; }\n"
        "    sleep 0.25\n"
        "  done\n"
        "  wait \"$p\" || sleep 5\n"
        "done";
    musicPid = spawnQuiet("/bin/sh", {"sh", "-c", script, "sh", player, musicPath, kMusicVolume}, true);
}

void stopMusic() {
    pid_t pid = musicPid;
    if (pid <= 0) return;
    musicPid = 0;
    kill(-pid, SIGTERM);
    waitpid(pid, nullptr, 0);
}

// Ctrl-C and friends would otherwise leave the music playing, since it runs in its own process group.
void onTerminationSignal(int sig) {
    pid_t pid = musicPid;
    if (pid > 0) kill(-pid, SIGTERM);
    signal(sig, SIG_DFL);
    raise(sig);
}

void installSignalHandlers() {
    for (int sig : {SIGINT, SIGTERM, SIGHUP}) {
        struct sigaction old;
        if (sigaction(sig, nullptr, &old) == 0 && old.sa_handler == SIG_IGN) continue;  // respect "ignore"

        struct sigaction action;
        action.sa_handler = onTerminationSignal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(sig, &action, nullptr);
    }
}

void reapFinishedEffects() {
    for (std::size_t i = 0; i < effectPids.size();) {
        if (waitpid(effectPids[i], nullptr, WNOHANG) != 0) {
            effectPids.erase(effectPids.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

}  // namespace

void init(Toggle music, Toggle effects) {
    player = findPlayer();
    std::string dir = player.empty() ? "" : findAssetDir();
    if (!dir.empty()) {
        if (fileExists(dir + "/" + kMusicFile)) musicPath = dir + "/" + kMusicFile;
        for (std::size_t i = 0; i < kNumEffects; ++i) {
            if (fileExists(dir + "/" + kEffectFiles[i])) effectPaths[i] = dir + "/" + kEffectFiles[i];
        }
    }

    std::atexit(stopMusic);
    installSignalHandlers();

    // NO_SOUND beats everything; otherwise an explicit On/Off wins and Default means "on in a terminal".
    auto wanted = [](Toggle toggle) {
        if (envSet("NO_SOUND") || toggle == Toggle::Off) return false;
        return toggle == Toggle::On || isInteractive();
    };
    setMusicEnabled(wanted(music));
    setEffectsEnabled(wanted(effects));
}

bool available() { return !player.empty() && (!musicPath.empty() || effectsAvailable()); }

bool musicEnabled() { return musicOn; }

void setMusicEnabled(bool on) {
    if (on) {
        startMusic();
        musicOn = musicPid > 0;
    } else {
        stopMusic();
        musicOn = false;
    }
}

bool effectsEnabled() { return effectsOn; }

void setEffectsEnabled(bool on) { effectsOn = on && effectsAvailable(); }

void play(Sound sound) {
    if (!effectsOn) return;
    const std::string& path = effectPaths[static_cast<std::size_t>(sound)];
    if (path.empty()) return;

    reapFinishedEffects();
    if (effectPids.size() >= kMaxOverlappingEffects) return;
    pid_t pid = spawnQuiet(player, {player, "-v", kEffectVolume, path}, false);
    if (pid > 0) effectPids.push_back(pid);
}

}  // namespace audio
}  // namespace bj

#endif  // _WIN32
