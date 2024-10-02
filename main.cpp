#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <fstream>

#define VERSION "1.0"

#ifdef _WIN32
#include <windows.h>

void getExePath(char *str, int *len) {
    DWORD ret = GetModuleFileName(NULL, str, *len);
    *len = (int)ret;
}

inline bool supportsColors() noexcept {
    return true; // all windows terminals have ansi support by default now
                 // honestly I don't expect anyone to use with on windows versions below win10
}

inline int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns = -1;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }

    return columns;
}
#else
#include <unistd.h>
#include <sys/ioctl.h>

void getExePath(char *str, int *len) {
    ssize_t ret = readlink("/proc/self/exe", str, *len);
    *len = (int)ret;
}

inline bool supportsColors() noexcept {
    static const bool res = []{
        const char *terms[] = {"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux", "msys", "putty", "rxvt", "screen", "vt100", "xterm", "vscode" };

        const char *envp = std::getenv("TERM");
        if (envp == nullptr) return false;
        return std::any_of(std::begin(terms), std::end(terms), [&](const char *term) { return std::strstr(envp, term) != nullptr; });
    }();

    return res;
}

inline int getConsoleWidth() {
    struct winsize w;
    int columns = -1;

    if (ioctl(STDOUT_FILENO, TIoCGWINSZ, &w) == 0) {
        columns = w.ws_col;
    }

    return columns;
}
#endif // _WIN32

std::string rightPadStr(std::string s, int count, std::string with = " ", int rstrlen = -1);
void displayDir();

bool changeDirectory(std::string dir) {
    // std::cout << "[xcd][DEBUG] Attempting directory change to '" << dir << "'.\n";
    if (std::filesystem::is_directory(std::filesystem::path(dir))) {
        // std::cout << "[xcd][DEBUG] Directory is valid, entering `xcddir(const char* path = \"" << dir << "\")`.\n";
        std::cout << "~$$;cd;" << dir << "\n";
        return true;
    }

    // std::cout << "[xcd][DEBUG] Attempted '" << dir << "'.\n";
    return false;
}

bool cdToAlias(std::string dir, const std::unordered_map<std::string, std::string> &config, const std::vector<std::string> &hidden) {
    // std::cout << "[xcd][DEBUG] haystacking '" << dir << "'.\n";
    for (const auto &[alias, fdir] : config) {
        std::string front = dir.substr(0, alias.length() + 1);
        // std::cout << "[xcd][DEBUG] front: '" << front << "', alias: '" << (alias + "/") << "', dir: '" << dir << "'\n";
        if (front == alias + "/" || front == alias + "\\") {
            // std::cout << "[xcd][DEBUG] Evaluating alias with remaining path.\n";
            std::filesystem::path currentPath(fdir);
            currentPath /= dir.substr(alias.length() + 1);
            return changeDirectory(currentPath.string());
        } else if (dir == alias) {
            // std::cout << "[xcd][DEBUG] Evaluating alias directly.\n";
            return changeDirectory(fdir);
        }
    }

    return false;
}

int main(int argc, const char **argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    if (args.size() == 0 || args[0] == "-h" || args[0] == "--help") {
        std::cout << "xcd v" VERSION " -- cross change directory -- by @starlitnova" << "\n";
        std::cout << "Usage: xcd [directory] [options]" << "\n";
        std::cout << "\n";
        std::cout << "Options:" << "\n";
        std::cout << "    -h, --help        Shows this help menu and exits" << "\n";
        std::cout << "    -v, --version     Shows the version of xcd and exits" << "\n";
        std::cout << "    -l, --list        Lists all files in the current directory and exits" << "\n";
        std::cout << "    -d, --dirs        Lists all directory aliases stored in the config file and exits" << "\n";
        std::cout << "    -a, --alias       Prioritizes aliases when changing directories" << "\n";
        return 0;
    }

    if (args[0] == "-v" || args[0] == "--version") {
        std::cout << "xcd v" VERSION << "\n";
        std::cout << "cross change directory by @starlitnova" << "\n";
        std::cout << "xcd is licensed under the MIT license, see main.cpp for more details" << "\n";
        return 0;
    }

    if (args[0] == "-l" || args[0] == "--list") {
        displayDir();
        return 0;
    }

    // the rest of the arguments require the configuration file, so we'll retrieve it
    char str[512];
    int len = 512;
    getExePath(str, &len);

    std::string path;
    path.assign(str, len);

    std::filesystem::path exePath(path);
    exePath /= "../xcd";

    std::filesystem::path config = exePath;
    config /= "xcd_conf.cf";

    std::string configData;

    std::ifstream f(config.c_str());
    if (f.good()) {
        std::stringstream ss;
        ss << f.rdbuf();
        f.close();
        configData = ss.str();
    } else {
        std::cout << "[xcd] Configuration file \"xcd/xcd_conf.cf\" not found.\n";
        // std::cout << "[xcd][DEBUG] Searching '" << config << "'.\n";
        return 1;
    }

    std::unordered_map<std::string, std::string> allConfigs{};
    for (size_t i = 0; i < configData.length(); i++) {
        while (isspace(configData[i])) i++;

        if (!isdigit(configData[i])) { // start assignment
            size_t startKey = i;
            while (configData[i] != '=' && i < configData.length()) i++;
            std::string key = configData.substr(startKey, i - startKey);
            i++;
            size_t startVal = i;
            while (configData[i] != ';' && !isspace(configData[i]) && i < configData.length()) i++;
            // i++ // automatically done by the for loop
            std::string value = configData.substr(startVal, i - startVal);
            allConfigs[key] = value;
        } else if (configData[i] == '#') { // start commennt
            while (configData[i] != '\n' && i < configData.length()) i++;
        }
    }

    static std::vector<std::string> sSpecialConfigKeys = {};

    // continue parsing args
    if (args[0] == "-d" || args[0] == "--dirs") {
        for (const auto &[key, val] : allConfigs) {
            if (std::find(sSpecialConfigKeys.begin(), sSpecialConfigKeys.end(), key) == sSpecialConfigKeys.end()) {
                std::cout << key << " @ " << val << "\n";
            }
        }

        return 0;
    }

    // obtain the optional parameters for directory navigating
    std::string todir;
    bool aliasPriority = false;
    for (const auto & arg : args) {
        if (arg == "-a" || arg == "--alias") {
            aliasPriority = true;
        } else {
            todir = arg;
        }
    }

    if (todir.length() < 1) {
        std::cout << std::filesystem::current_path().string() << "\n";
        return 0;
    }

    bool succeeded = false;
    if (aliasPriority) {
        succeeded = cdToAlias(todir, allConfigs, sSpecialConfigKeys) || changeDirectory(todir);
    } else {
        succeeded = changeDirectory(todir) || cdToAlias(todir, allConfigs, sSpecialConfigKeys);
    }

    if (!succeeded) {
        std::cout << "[xcd] The system could not find the path specified.\n";
        return 1;
    }

    // implicit return 0;
}

struct FileObj {
    std::string consoleText;
    size_t plainTextLength;
    std::filesystem::path pathObj;
    bool isdir;

    FileObj(const std::string &a, size_t b, const std::filesystem::path &p, bool d) : consoleText(a), plainTextLength(b), pathObj(p), isdir(d) {}
};

const std::vector<std::string> execFmts = {
    ".exe", ".bat", ".cmd", ".bat",
    ".vbs", ".vbe",
    ".out", ".bin",
    ".so", ".o", ".dll", ".lib", ".shared",
    ".elf", ".sh"
};

// pretty biased, yea?
// I'll accept changes to this depending on  if I like them (or if you added a crap ton of other useful code)
const std::unordered_map<std::string, std::string> ffmtColors = {
    { ".cpp", "\x1b[36m" },
    { ".hpp", "\x1b[36m" },
    { ".cc", "\x1b[36m" },
    { ".hh", "\x1b[36m" },
    { ".c", "\x1b[96m" },
    { ".h", "\x1b[96m" }
};

void displayDir() {
    bool colors = supportsColors();

    std::vector<FileObj> dirfiles;
    size_t longestfname = 0;
    for (const auto &file : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        std::string fname = file.path().filename().string();
        std::string ext = file.path().extension().string();
        size_t flen = fname.length();

        if (colors) {
            if (file.is_directory()) {
                fname = "\x1b[94m" + fname + "\x1b[0m";
            } else if (std::find(execFmts.begin(), execFmts.end(), ext) != execFmts.end()) {
                fname = "\x1b[92m" + fname + "\x1b[0m";
            } else if (ffmtColors.count(ext)) {
                fname = ffmtColors.at(ext) + fname + "\x1b[0m";
            }
        } else {
            if (file.is_directory()) {
                fname = "[DIR] " + fname;
            } else if (std::find(execFmts.begin(), execFmts.end(), ext) != execFmts.end()) {
                fname = "<BIN> " + fname;
            }

            flen = fname.length();
        }

        if (flen > longestfname) {
            longestfname = flen;
        }

        dirfiles.emplace_back(fname, flen, file.path(), file.is_directory());
    }

    std::sort(dirfiles.begin(), dirfiles.end(), [](const FileObj &a, const FileObj &b) {
        if ((a.isdir && b.isdir) || !(a.isdir || b.isdir)) {
            return a.pathObj.filename().string() < b.pathObj.filename().string();
        }

        return a.isdir;
    });

    longestfname += 2; // 2 character gap between each object

    int maxPerLine = 1;
    int consoleLen = getConsoleWidth();

    if (consoleLen > longestfname) {
        maxPerLine = consoleLen / longestfname;
    }

    for (int i = 0; i < dirfiles.size(); i++) {
        for (int fi = 0; fi < maxPerLine; fi++) {
            std::cout << rightPadStr(dirfiles[i].consoleText, longestfname, " ", dirfiles[i].plainTextLength);
            i++;
            if (i >= dirfiles.size()) break;
        }

        std::cout << "\n";
    }
}

std::string rightPadStr(std::string s, int count, std::string with, int rstrlen) {
    std::string f = s;
    size_t len = rstrlen < 0 ? f.length() : rstrlen;

    while (len < count) {
        f += with;
        len += with.length();
    }

    return f;
}

