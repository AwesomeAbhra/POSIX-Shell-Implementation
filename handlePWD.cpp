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