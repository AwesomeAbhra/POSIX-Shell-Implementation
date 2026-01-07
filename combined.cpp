// ----- shell.cpp -----
#include "shell.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <dirent.h>

using namespace std;

vector<string> history_lines;
static const string HISTORY_FILE = ".myshell_history";
static const size_t MAX_HISTORY = 1000;

void append_to_history(const string &cmd) {
    if (cmd.empty()) return;
    if (!history_lines.empty() && history_lines.back() == cmd) return;
    history_lines.push_back(cmd);
    if (history_lines.size() > MAX_HISTORY) history_lines.erase(history_lines.begin());
}

void save_history_to_file(const std::string &path) {
    string p = path.empty() ? (getenv("HOME") ? string(getenv("HOME")) + "/" + HISTORY_FILE : HISTORY_FILE) : path;
    ofstream out(p, ios::app);
    if (!out) return;
    size_t start = (history_lines.size() > 500) ? history_lines.size()-500 : 0;
    for (size_t i = start; i < history_lines.size(); ++i) {
        out << history_lines[i] << "\n";
    }
    out.close();
}

void load_history_from_file(const std::string &path) {
    string p = path.empty() ? (getenv("HOME") ? string(getenv("HOME")) + "/" + HISTORY_FILE : HISTORY_FILE) : path;
    ifstream in(p);
    if (!in) return;
    string line;
    while (getline(in, line)) {
        if (!line.empty()) history_lines.push_back(line);
    }
    if (history_lines.size() > MAX_HISTORY) {
        auto start = history_lines.size() - MAX_HISTORY;
        vector<string> truncated(history_lines.begin() + start, history_lines.end());
        history_lines.swap(truncated);
    }
}

string trim(const string &s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

vector<string> split_tokens(const string &s) {
    vector<string> tokens;
    string cur;
    bool in_quote = false;
    char quote_char = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if ((c == '\"' || c == '\'') ) {
            if (!in_quote) { in_quote = true; quote_char = c; }
            else if (quote_char == c) { in_quote = false; quote_char = 0; }
            else cur.push_back(c);
        } else if (isspace((unsigned char)c) && !in_quote) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

vector<string> tokenize(const string& command) { return split_tokens(command); }

vector<string> get_files_in_directory(const string& path) {
    vector<string> res;
    DIR *d = opendir(path.empty() ? "." : path.c_str());
    if (!d) return res;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        res.push_back(entry->d_name);
    }
    closedir(d);
    sort(res.begin(), res.end());
    return res;
}




// ----- line_reader.cpp -----
#include "line_reader.h"
#include "shell.h"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <algorithm>
#include <iostream>

using std::string;
using std::vector;

extern std::vector<std::string> history_lines;

static vector<string> list_dir_matches(const string &prefix) {
    vector<string> res;
    DIR *d = opendir(".");
    if (!d) return res;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        string name = entry->d_name;
        if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
            res.push_back(name);
        }
    }
    closedir(d);
    std::sort(res.begin(), res.end());
    return res;
}

bool readline_with_autocomplete(string &out) {
    struct termios orig, raw;
    if (tcgetattr(STDIN_FILENO, &orig) == -1) return false;
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;

    out.clear();
    string buf;
    size_t cursor = 0;
    ssize_t nread;
    char c;

    int history_index = (int)history_lines.size();
    auto restore_term = [&](){ tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); };

    while (true) {
        nread = read(STDIN_FILENO, &c, 1);
        if (nread <= 0) { restore_term(); return false; }

        if (c == 127 || c == 8) {
            if (cursor > 0) {
                buf.erase(cursor-1,1);
                cursor--;
                write(STDOUT_FILENO, "\r", 1);
                display();
                write(STDOUT_FILENO, buf.c_str(), buf.size());
            }
        } else if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            restore_term();
            out = buf;
            return true;
        } else if (c == '\t') {
            size_t start = (cursor==0)?0:buf.rfind(' ', cursor-1);
            if (start == string::npos) start = 0; else start = start + 1;
            string prefix = buf.substr(start, cursor - start);
            vector<string> matches = list_dir_matches(prefix);
            if (matches.empty()) {
                const char *bell = "\a";
                write(STDOUT_FILENO, bell, 1);
            } else if (matches.size() == 1) {
                string rest = matches[0].substr(prefix.size());
                buf.insert(cursor, rest);
                cursor += rest.size();
                write(STDOUT_FILENO, rest.c_str(), rest.size());
            } else {
                write(STDOUT_FILENO, "\n", 1);
                string listing;
                for (auto &m: matches) listing += m + "  ";
                listing += "\n";
                write(STDOUT_FILENO, listing.c_str(), listing.size());
                display();
                write(STDOUT_FILENO, buf.c_str(), buf.size());
            }
        } else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    if (!history_lines.empty() && history_index > 0) {
                        history_index--;
                        buf = history_lines[history_index];
                        cursor = buf.size();
                        write(STDOUT_FILENO, "\r", 1);
                        display();
                        write(STDOUT_FILENO, buf.c_str(), buf.size());
                    }
                } else if (seq[1] == 'B') {
                    if (!history_lines.empty() && history_index < (int)history_lines.size()-1) {
                        history_index++;
                        buf = history_lines[history_index];
                        cursor = buf.size();
                    } else {
                        history_index = history_lines.size();
                        buf.clear();
                        cursor = 0;
                    }
                    write(STDOUT_FILENO, "\r", 1);
                    display();
                    write(STDOUT_FILENO, buf.c_str(), buf.size());
                } else if (seq[1] == 'C') {
                    if (cursor < buf.size()) {
                        char ch = buf[cursor++];
                        write(STDOUT_FILENO, &ch, 1);
                    }
                } else if (seq[1] == 'D') {
                    if (cursor > 0) {
                        write(STDOUT_FILENO, "\b", 1);
                        cursor--;
                    }
                }
            }
        } else if (isprint((unsigned char)c)) {
            buf.insert(buf.begin()+cursor, c);
            cursor++;
            write(STDOUT_FILENO, &c, 1);
        }
    }

    restore_term();
    return false;
}


// ----- signal.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
using namespace std;
pid_t foregroundPid = -1;
static void sigchld_handler(int) {
    int saved_errno = errno;
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) break;
    }
    errno = saved_errno;
}
void handleSigint(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGINT);
    } else {
        cout << endl;
    }
}
void handleSigtstp(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGTSTP);
    } else {
        cout << endl;
    }
}
void setupSignalHandlers()
{
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);
    struct sigaction sa_int;
    sa_int.sa_handler = handleSigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);
    struct sigaction sa_tstp;
    sa_tstp.sa_handler = handleSigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    load_history_from_file("");
}

// ----- utils_explain.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
using namespace std;
void handleExplain(const vector<string>& tokens) {
    if (tokens.size() < 2) { cout << "Usage: explain <command>\n"; return; }
    string cmd = tokens[1];
    static unordered_map<string,string> expl = {
        {"ls", "List files and directories in the current directory. Useful flags: -l (long), -a (hidden)."},
        {"grep", "Filter input lines matching a pattern. Example: grep 'foo' file.txt"},
        {"awk", "A small text-processing language often used for column extraction and reporting."},
        {"sed", "Stream editor for basic text transformations on an input stream."},
        {"tar", "Archive utility to create/extract tar archives."},
        {"ssh", "Secure shell for remote login to other machines."}
    };
    auto it = expl.find(cmd);
    if (it != expl.end()) cout << cmd << ": " << it->second << "\n";
    else cout << "No local explanation found for '"<<cmd<<"'. (You could integrate an LLM for richer answers.)\n";
}

// ----- handleEcho.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
using namespace std;
void handleEcho(const vector<string>& tokens)
{
    bool newline = true;
    size_t start = 1;
    if (tokens.size() >= 2 && tokens[1] == "-n") { newline = false; start = 2; }
    for (size_t i = start; i < tokens.size(); ++i) { cout << tokens[i]; if (i + 1 < tokens.size()) cout << ' '; }
    if (newline) cout << '\n';
}

// ----- handlePWD.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
using namespace std;
void handlePWD(const vector<string>& tokens)
{
    if (tokens.size() > 1) { cerr << "pwd: too many arguments\n"; return; }
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) cout << cwd << endl;
    else perror("pwd");
}

// ----- handleLS.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
using namespace std;
extern string shellhomedirectory;
static string expand_home(const string &p) {
    if (!p.empty() && p[0] == '~') {
        if (p.size() == 1) return shellhomedirectory.empty() ? string(getenv("HOME") ? getenv("HOME") : ".") : shellhomedirectory;
        string home = shellhomedirectory.empty() ? (getenv("HOME") ? getenv("HOME") : string(".")) : shellhomedirectory;
        return home + p.substr(1);
    }
    return p;
}
void handleLS(const vector<string>& tokens)
{
    bool showAll = false;
    bool longFormat = false;
    vector<string> dirs;
    for(size_t i = 1; i < tokens.size(); ++i)
    {
        if(tokens[i] == "-a") { showAll = true; }
        else if(tokens[i] == "-l") { longFormat = true; }
        else if(tokens[i] == "-la" || tokens[i] == "-al") { showAll = true; longFormat = true; }
        else { dirs.push_back(tokens[i]); }
    }
    if(dirs.empty())
    {
        char currentdirectory[1024] = {0};
        if (getcwd(currentdirectory, sizeof(currentdirectory)) == nullptr) dirs.push_back(string("."));
        else dirs.push_back(string(currentdirectory));
    }
    for(size_t di = 0; di < dirs.size(); ++di)
    {
        string dirname = expand_home(dirs[di]);
        if (dirs.size() > 1) { cout << dirname << ":" << endl; }
        DIR* dir = opendir(dirname.c_str());
        if(dir == nullptr) { perror(("ls: cannot access " + dirname).c_str()); if (di + 1 < dirs.size()) cout << endl; continue; }
        vector<string> names;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (!showAll && entry->d_name[0] == '.') continue;
            names.push_back(entry->d_name);
        }
        closedir(dir);
        sort(names.begin(), names.end());
        for (const auto &name : names) {
            if (longFormat) {
                struct stat fileStat;
                string fullPath = dirname + "/" + name;
                if (stat(fullPath.c_str(), &fileStat) == 0) {
                    cout << (S_ISDIR(fileStat.st_mode) ? "d" : "-");
                    cout << (fileStat.st_mode & S_IRUSR ? "r" : "-");
                    cout << (fileStat.st_mode & S_IWUSR ? "w" : "-");
                    cout << (fileStat.st_mode & S_IXUSR ? "x" : "-");
                    cout << (fileStat.st_mode & S_IRGRP ? "r" : "-");
                    cout << (fileStat.st_mode & S_IWGRP ? "w" : "-");
                    cout << (fileStat.st_mode & S_IXGRP ? "x" : "-");
                    cout << (fileStat.st_mode & S_IROTH ? "r" : "-");
                    cout << (fileStat.st_mode & S_IWOTH ? "w" : "-");
                    cout << (fileStat.st_mode & S_IXOTH ? "x" : "-");
                    cout << " " << setw(3) << fileStat.st_nlink;
                    struct passwd* pw = getpwuid(fileStat.st_uid);
                    struct group* gr = getgrgid(fileStat.st_gid);
                    cout << " " << left << setw(8) << (pw ? pw->pw_name : "unknown");
                    cout << " " << left << setw(8) << (gr ? gr->gr_name : "unknown");
                    cout << " " << setw(8) << fileStat.st_size;
                    char timebuf[64];
                    struct tm tmres;
                    localtime_r(&fileStat.st_mtime, &tmres);
                    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", &tmres);
                    cout << " " << timebuf;
                    cout << " " << name << endl;
                } else {
                    perror(("stat error: " + fullPath).c_str());
                }
            } else {
                cout << name << "\t";
            }
        }
        if (!longFormat) cout << endl;
        if (di + 1 < dirs.size()) cout << endl;
    }
}

// ----- handlesearch.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <dirent.h>
#include <sys/stat.h>
using namespace std;
static bool searchInDirectory(const string &path, const string &target) {
    DIR *dir = opendir(path.c_str());
    if (!dir) return false;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == "." || name == "..") continue;
        string full = path + "/" + name;
        if (name == target) { closedir(dir); return true; }
        if (entry->d_type == DT_DIR) { if (searchInDirectory(full, target)) { closedir(dir); return true; } }
    }
    closedir(dir); return false;
}
void handlesearch(const vector<string>& tokens)
{
    if (tokens.size() != 2) { cerr << "Usage: search <name>\n"; return; }
    if (searchInDirectory(".", tokens[1])) cout << "True\n"; else cout << "False\n";
}

// ----- handlepinfo.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <cerrno>
using namespace std;
extern string shellhomedirectory;
void handlepinfo(const vector<string>& tokens)
{
    pid_t pid;
    if (tokens.size() > 2) { cerr << "pinfo: too many arguments\n"; return; }
    if (tokens.size() == 2) {
        try { pid = static_cast<pid_t>(stoi(tokens[1])); } catch (...) { cerr << "pinfo: invalid pid\n"; return; }
    } else { pid = getpid(); }
    cout << "pid -- " << pid << endl;
    string statPath = "/proc/" + to_string(pid) + "/stat";
    ifstream statFile(statPath);
    if (!statFile.is_open()) { perror(("pinfo: cannot open " + statPath).c_str()); return; }
    string line; getline(statFile, line); statFile.close();
    if (line.empty()) { cerr << "pinfo: empty stat file\n"; return; }
    size_t lparen = line.find('('); size_t rparen = line.rfind(')');
    if (lparen == string::npos || rparen == string::npos || rparen <= lparen) { cerr << "pinfo: unexpected /proc/[pid]/stat format\n"; return; }
    string comm = line.substr(lparen + 1, rparen - lparen - 1);
    string rest = line.substr(rparen + 1); istringstream iss(rest); string state; iss >> state;
    cout << "Process Status -- " << state << endl;
    string statmPath = "/proc/" + to_string(pid) + "/statm";
    ifstream statmFile(statmPath);
    if (statmFile.is_open()) {
        string memline; getline(statmFile, memline); statmFile.close();
        if (!memline.empty()) {
            istringstream iss2(memline); long pages;
            if (iss2 >> pages) { long page_size = sysconf(_SC_PAGESIZE); long mem_bytes = pages * page_size; cout << "memory -- " << mem_bytes << " bytes (" << pages << " pages)\n"; }
            else cout << "memory -- " << memline << "\n";
        } else cout << "memory -- (empty statm)\n";
    } else { perror(("pinfo: cannot open " + statmPath).c_str()); }
    char exePath[PATH_MAX]; string exeLink = "/proc/" + to_string(pid) + "/exe";
    ssize_t len = readlink(exeLink.c_str(), exePath, sizeof(exePath) - 1);
    if (len != -1) { exePath[len] = '\0'; string pathStr(exePath); if (!shellhomedirectory.empty() && pathStr.find(shellhomedirectory) == 0) pathStr.replace(0, shellhomedirectory.length(), "~"); cout << "Executable Path -- " << pathStr << endl; }
    else { if (errno == EACCES || errno == ENOENT || errno == EINVAL) perror(("pinfo: readlink " + exeLink).c_str()); else cerr << "pinfo: cannot read executable path\n"; }
}

// ----- handleCD.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

using namespace std;

extern string shellhomedirectory;
extern bool flagforcd;

static string prevdirectory = "";

static string get_current_working_dir() {
    char cwd[1024] = {0};
    if (getcwd(cwd, sizeof(cwd)) == nullptr) return string();
    return string(cwd);
}

void handleCD(const vector<string>& tokens)
{
    if (prevdirectory.empty()) {
        string cur = get_current_working_dir();
        if (!cur.empty()) prevdirectory = cur;
    }

    string currentdirectorystring = get_current_working_dir();

    if (tokens.size() > 2) {
        cerr << "cd: too many arguments\n";
        return;
    }

    if (tokens.size() == 1 || (tokens.size() == 2 && tokens[1] == "~")) {
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : nullptr;
        }
        if (!home) {
            cerr << "cd: HOME not set\n";
            return;
        }
        if (chdir(home) == 0) {
            prevdirectory = currentdirectorystring;
            flagforcd = true;
        } else {
            perror("cd");
        }
        return;
    }

    string arg = tokens[1];

    if (arg == "-") {
        if (prevdirectory.empty()) {
            cerr << "cd: OLDPWD not set\n";
            return;
        }
        if (chdir(prevdirectory.c_str()) == 0) {
            prevdirectory = currentdirectorystring;
            flagforcd = true;
        } else {
            perror("cd");
        }
        return;
    }

    if (arg == ".") {
        return;
    }

    if (!arg.empty() && arg[0] == '~') {
        string home = shellhomedirectory;
        if (home.empty()) {
            const char* envhome = getenv("HOME");
            if (envhome) home = string(envhome);
            else {
                struct passwd *pw = getpwuid(getuid());
                if (pw && pw->pw_dir) home = string(pw->pw_dir);
            }
        }
        string path = home + arg.substr(1);
        if (chdir(path.c_str()) == 0) {
            prevdirectory = currentdirectorystring;
            flagforcd = true;
        } else {
            perror("cd");
        }
        return;
    }

    if (arg == "..") {
        if (chdir("..") == 0) {
            prevdirectory = currentdirectorystring;
            flagforcd = true;
        } else {
            perror("cd");
        }
        return;
    }

    if (chdir(arg.c_str()) == 0) {
        prevdirectory = currentdirectorystring;
        flagforcd = true;
    } else {
        perror("cd");
    }
}


// ----- display.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
#include <pwd.h>
using namespace std;
extern string shellhomedirectory;
extern bool flagforcd;
void display()
{
    const char* loginname = getlogin();
    string username;
    if (loginname && strlen(loginname) > 0) {
        username = string(loginname);
    } else {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_name) username = string(pw->pw_name);
        else username = "user";
    }
    char systemname[1024] = {0};
    if (gethostname(systemname, sizeof(systemname)) != 0) {
        strncpy(systemname, "host", sizeof(systemname)-1);
    }
    char currentdirectory[1024] = {0};
    if (getcwd(currentdirectory, sizeof(currentdirectory)) == nullptr) {
        strncpy(currentdirectory, ".", sizeof(currentdirectory)-1);
    }
    string currentdirectorystring(currentdirectory);
    if (!shellhomedirectory.empty() && currentdirectorystring.find(shellhomedirectory) == 0) {
        string shortdir = "~";
        if (currentdirectorystring.length() > shellhomedirectory.length())
            shortdir += currentdirectorystring.substr(shellhomedirectory.length());
        printf("%s@%s:%s$ ", username.c_str(), systemname, shortdir.c_str());
    } else {
        printf("%s@%s:%s$ ", username.c_str(), systemname, currentdirectorystring.c_str());
    }
    if (flagforcd) flagforcd = false;
    fflush(stdout);
}

// ----- command.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

extern pid_t foregroundPid;

static vector<string> split_by_semicolon(const string &s) {
    vector<string> res;
    string cur;
    bool inq=false; char qc=0;
    for (char c: s) {
        if ((c=='"' || c=='\'') && !inq) { inq=true; qc=c; cur.push_back(c); }
        else if (inq && c==qc) { inq=false; cur.push_back(c); }
        else if (c==';' && !inq) { res.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) res.push_back(trim(cur));
    return res;
}

void parsecommand(char *input) {
    if (!input) return;
    string line = trim(string(input));
    if (line.empty()) return;

    vector<string> statements = split_by_semicolon(line);
    for (const string &stmt : statements) {
        if (stmt.empty()) continue;
        vector<string> pipe_segments;
        {
            string cur; bool inq=false; char qc=0;
            for (size_t i=0;i<stmt.size();++i) {
                char c = stmt[i];
                if ((c=='\"' || c=='\'') && !inq) { inq=true; qc=c; cur.push_back(c); }
                else if (inq && c==qc) { inq=false; cur.push_back(c); }
                else if (c=='|' && !inq) { pipe_segments.push_back(trim(cur)); cur.clear(); }
                else cur.push_back(c);
            }
            if (!cur.empty()) pipe_segments.push_back(trim(cur));
        }

        if (pipe_segments.empty()) continue;

        if (pipe_segments.size() == 1) {
            vector<string> toks = tokenize(pipe_segments[0]);
            if (toks.empty()) continue;
            if (toks[0] == "cd") { handleCD(toks); continue; }
            if (toks[0] == "explain") { handleExplain(toks); continue; }
        }

        handle_pipeline(pipe_segments);
    }
}

void handle_pipeline(const vector<string>& pipe_segments) {
    int n = (int)pipe_segments.size();
    if (n == 0) return;

    int prev_fd = -1;
    vector<int> child_pids;

    for (int i = 0; i < n; ++i) {
        bool is_last = (i == n-1);
        int fdpipe[2] = {-1,-1};
        if (!is_last) {
            if (pipe(fdpipe) < 0) {
                perror("pipe");
                if (prev_fd != -1) close(prev_fd);
                return;
            }
        }

        vector<string> toks = tokenize(pipe_segments[i]);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (fdpipe[0] != -1) close(fdpipe[0]);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);
            return;
        }

        if (pid == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
            }
            if (!is_last) {
                dup2(fdpipe[1], STDOUT_FILENO);
            }
            if (fdpipe[0] != -1) close(fdpipe[0]);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);

            bool background = false;
            int in_fd = STDIN_FILENO;
            int out_fd = STDOUT_FILENO;
            vector<string> argv;
            for (size_t j = 0; j < toks.size(); ++j) {
                const string &tk = toks[j];
                if (tk == "<") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_RDONLY);
                        if (fd < 0) { perror("open infile"); _exit(EXIT_FAILURE); }
                        in_fd = fd;
                        ++j;
                    }
                } else if (tk == ">") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0) { perror("open outfile"); _exit(EXIT_FAILURE); }
                        out_fd = fd;
                        ++j;
                    }
                } else if (tk == ">>") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (fd < 0) { perror("open outfile append"); _exit(EXIT_FAILURE); }
                        out_fd = fd;
                        ++j;
                    }
                } else if (tk == "&" && j+1 == toks.size()) {
                    background = true;
                } else {
                    argv.push_back(tk);
                }
            }

            if (in_fd != STDIN_FILENO) { dup2(in_fd, STDIN_FILENO); if (in_fd!=STDIN_FILENO) close(in_fd); }
            if (out_fd != STDOUT_FILENO) { dup2(out_fd, STDOUT_FILENO); if (out_fd!=STDOUT_FILENO) close(out_fd); }

            if (argv.empty()) _exit(EXIT_SUCCESS);

            if (argv[0] == "echo" || argv[0] == "pwd" || argv[0] == "ls" || argv[0] == "pinfo" || argv[0] == "search" || argv[0] == "history" || argv[0] == "explain") {
                vector<string> tmp = argv;
                if (argv[0] == "echo") handleEcho(tmp);
                else if (argv[0] == "pwd") handlePWD(tmp);
                else if (argv[0] == "ls") handleLS(tmp);
                else if (argv[0] == "pinfo") handlepinfo(tmp);
                else if (argv[0] == "search") handlesearch(tmp);
                else if (argv[0] == "history") display_history(tmp);
                else if (argv[0] == "explain") handleExplain(tmp);
                _exit(EXIT_SUCCESS);
            }

            vector<char*> cargv;
            for (auto &s : argv) cargv.push_back(const_cast<char*>(s.c_str()));
            cargv.push_back(nullptr);

            if (background) setpgid(0, 0);

            execvp(cargv[0], cargv.data());
            perror("execvp");
            _exit(EXIT_FAILURE);
        } else {
            child_pids.push_back(pid);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);
            prev_fd = (fdpipe[0] != -1) ? fdpipe[0] : -1;
        }
    }

    for (int pid : child_pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}


// ----- main.cpp -----
#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
using namespace std;
string shellhomedirectory;
bool flagforcd = false;
extern bool readline_with_autocomplete(std::string &out);
int main()
{
    setupSignalHandlers();
    char currentworkingdirectory[1024];
    if((getcwd(currentworkingdirectory, sizeof(currentworkingdirectory)) != nullptr))
    {
        shellhomedirectory = string(currentworkingdirectory);
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }
    while(true)
    {
        display();
        string inputstr;
        if (!readline_with_autocomplete(inputstr)) {
            cout << endl;
            break;
        }
        append_to_history(inputstr);
        if(inputstr.empty()) continue;
        if(inputstr == "exit") exit(EXIT_SUCCESS);
        char* inputCopy = strdup(inputstr.c_str());
        if (!inputCopy) { perror("strdup"); continue; }
        parsecommand(inputCopy);
        free(inputCopy);
    }
    return 0;
}

