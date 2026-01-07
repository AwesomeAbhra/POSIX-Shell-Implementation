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