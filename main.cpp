#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
using namespace std;
string shellhomedirectory;
bool flagforcd = false;
extern bool readline_with_autocomplete(std::string &out);
int main()
{
    setupSignalHandlers();
    char currentworkingdirectory[1024];
    if((getcwd(currentworkingdirectory, sizeof(currentworkingdirectory)) != nullptr))
    {
        shellhomedirectory = string(currentworkingdirectory);
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }
    while(true)
    {
        display();
        string inputstr;
        if (!readline_with_autocomplete(inputstr)) {
            cout << endl;
            break;
        }
        append_to_history(inputstr);
        if(inputstr.empty()) continue;
        if(inputstr == "exit") exit(EXIT_SUCCESS);
        char* inputCopy = strdup(inputstr.c_str());
        if (!inputCopy) { perror("strdup"); continue; }
        parsecommand(inputCopy);
        free(inputCopy);
    }
    return 0;
}