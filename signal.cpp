#include <bits/stdc++.h>
#include "shell.h"
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
using namespace std;
pid_t foregroundPid = -1;
static void sigchld_handler(int) {
    int saved_errno = errno;
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) break;
    }
    errno = saved_errno;
}
void handleSigint(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGINT);
    } else {
        cout << endl;
    }
}
void handleSigtstp(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGTSTP);
    } else {
        cout << endl;
    }
}
void setupSignalHandlers()
{
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);
    struct sigaction sa_int;
    sa_int.sa_handler = handleSigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);
    struct sigaction sa_tstp;
    sa_tstp.sa_handler = handleSigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    load_history_from_file("");
}