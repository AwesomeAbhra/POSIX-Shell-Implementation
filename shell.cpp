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


