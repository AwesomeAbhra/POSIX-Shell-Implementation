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