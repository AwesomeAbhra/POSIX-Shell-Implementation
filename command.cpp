#include <bits/stdc++.h>
#include "shell.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

extern pid_t foregroundPid;

static vector<string> split_by_semicolon(const string &s) {
    vector<string> res;
    string cur;
    bool inq=false; char qc=0;
    for (char c: s) {
        if ((c=='"' || c=='\'') && !inq) { inq=true; qc=c; cur.push_back(c); }
        else if (inq && c==qc) { inq=false; cur.push_back(c); }
        else if (c==';' && !inq) { res.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) res.push_back(trim(cur));
    return res;
}

void parsecommand(char *input) {
    if (!input) return;
    string line = trim(string(input));
    if (line.empty()) return;

    vector<string> statements = split_by_semicolon(line);
    for (const string &stmt : statements) {
        if (stmt.empty()) continue;
        vector<string> pipe_segments;
        {
            string cur; bool inq=false; char qc=0;
            for (size_t i=0;i<stmt.size();++i) {
                char c = stmt[i];
                if ((c=='\"' || c=='\'') && !inq) { inq=true; qc=c; cur.push_back(c); }
                else if (inq && c==qc) { inq=false; cur.push_back(c); }
                else if (c=='|' && !inq) { pipe_segments.push_back(trim(cur)); cur.clear(); }
                else cur.push_back(c);
            }
            if (!cur.empty()) pipe_segments.push_back(trim(cur));
        }

        if (pipe_segments.empty()) continue;

        if (pipe_segments.size() == 1) {
            vector<string> toks = tokenize(pipe_segments[0]);
            if (toks.empty()) continue;
            if (toks[0] == "cd") { handleCD(toks); continue; }
            if (toks[0] == "explain") { handleExplain(toks); continue; }
        }

        handle_pipeline(pipe_segments);
    }
}

void handle_pipeline(const vector<string>& pipe_segments) {
    int n = (int)pipe_segments.size();
    if (n == 0) return;

    int prev_fd = -1;
    vector<int> child_pids;

    for (int i = 0; i < n; ++i) {
        bool is_last = (i == n-1);
        int fdpipe[2] = {-1,-1};
        if (!is_last) {
            if (pipe(fdpipe) < 0) {
                perror("pipe");
                if (prev_fd != -1) close(prev_fd);
                return;
            }
        }

        vector<string> toks = tokenize(pipe_segments[i]);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (fdpipe[0] != -1) close(fdpipe[0]);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);
            return;
        }

        if (pid == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
            }
            if (!is_last) {
                dup2(fdpipe[1], STDOUT_FILENO);
            }
            if (fdpipe[0] != -1) close(fdpipe[0]);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);

            bool background = false;
            int in_fd = STDIN_FILENO;
            int out_fd = STDOUT_FILENO;
            vector<string> argv;
            for (size_t j = 0; j < toks.size(); ++j) {
                const string &tk = toks[j];
                if (tk == "<") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_RDONLY);
                        if (fd < 0) { perror("open infile"); _exit(EXIT_FAILURE); }
                        in_fd = fd;
                        ++j;
                    }
                } else if (tk == ">") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0) { perror("open outfile"); _exit(EXIT_FAILURE); }
                        out_fd = fd;
                        ++j;
                    }
                } else if (tk == ">>") {
                    if (j+1 < toks.size()) {
                        int fd = open(toks[j+1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (fd < 0) { perror("open outfile append"); _exit(EXIT_FAILURE); }
                        out_fd = fd;
                        ++j;
                    }
                } else if (tk == "&" && j+1 == toks.size()) {
                    background = true;
                } else {
                    argv.push_back(tk);
                }
            }

            if (in_fd != STDIN_FILENO) { dup2(in_fd, STDIN_FILENO); if (in_fd!=STDIN_FILENO) close(in_fd); }
            if (out_fd != STDOUT_FILENO) { dup2(out_fd, STDOUT_FILENO); if (out_fd!=STDOUT_FILENO) close(out_fd); }

            if (argv.empty()) _exit(EXIT_SUCCESS);

            if (argv[0] == "echo" || argv[0] == "pwd" || argv[0] == "ls" || argv[0] == "pinfo" || argv[0] == "search" || argv[0] == "history" || argv[0] == "explain") {
                vector<string> tmp = argv;
                if (argv[0] == "echo") handleEcho(tmp);
                else if (argv[0] == "pwd") handlePWD(tmp);
                else if (argv[0] == "ls") handleLS(tmp);
                else if (argv[0] == "pinfo") handlepinfo(tmp);
                else if (argv[0] == "search") handlesearch(tmp);
                else if (argv[0] == "history") display_history(tmp);
                else if (argv[0] == "explain") handleExplain(tmp);
                _exit(EXIT_SUCCESS);
            }

            vector<char*> cargv;
            for (auto &s : argv) cargv.push_back(const_cast<char*>(s.c_str()));
            cargv.push_back(nullptr);

            if (background) setpgid(0, 0);

            execvp(cargv[0], cargv.data());
            perror("execvp");
            _exit(EXIT_FAILURE);
        } else {
            child_pids.push_back(pid);
            if (fdpipe[1] != -1) close(fdpipe[1]);
            if (prev_fd != -1) close(prev_fd);
            prev_fd = (fdpipe[0] != -1) ? fdpipe[0] : -1;
        }
    }

    for (int pid : child_pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}
