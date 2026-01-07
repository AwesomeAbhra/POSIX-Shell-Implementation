#include "shell.h"
#include <bits/stdc++.h>
#include <fstream>
using namespace std;

void display_history(const vector<string>& tokens) {
    size_t max_display = 10;
    if (tokens.size() > 2) {
        cerr << "history: too many arguments\n";
        return;
    }
    if (tokens.size() == 2) {
        try {
            int v = stoi(tokens[1]);
            if (v > 0) max_display = (size_t)v;
        } catch (...) {
            cerr << "history: invalid argument\n";
            return;
        }
    }
    size_t n = history_lines.size();
    size_t start = (n > max_display) ? n - max_display : 0;
    for (size_t i = start; i < n; ++i) {
        cout << history_lines[i] << "\n";
    }
}