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