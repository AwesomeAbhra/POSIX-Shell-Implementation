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
