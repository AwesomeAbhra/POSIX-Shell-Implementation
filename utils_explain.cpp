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