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