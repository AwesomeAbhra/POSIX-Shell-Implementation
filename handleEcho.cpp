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