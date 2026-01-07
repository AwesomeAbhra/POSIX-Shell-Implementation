#include "line_reader.h"
#include "shell.h"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <algorithm>
#include <iostream>

using std::string;
using std::vector;

extern std::vector<std::string> history_lines;

static vector<string> list_dir_matches(const string &prefix) {
    vector<string> res;
    DIR *d = opendir(".");
    if (!d) return res;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        string name = entry->d_name;
        if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
            res.push_back(name);
        }
    }
    closedir(d);
    std::sort(res.begin(), res.end());
    return res;
}

bool readline_with_autocomplete(string &out) {
    struct termios orig, raw;
    if (tcgetattr(STDIN_FILENO, &orig) == -1) return false;
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;

    out.clear();
    string buf;
    size_t cursor = 0;
    ssize_t nread;
    char c;

    int history_index = (int)history_lines.size();
    auto restore_term = [&](){ tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); };

    while (true) {
        nread = read(STDIN_FILENO, &c, 1);
        if (nread <= 0) { restore_term(); return false; }

        if (c == 127 || c == 8) {
            if (cursor > 0) {
                buf.erase(cursor-1,1);
                cursor--;
                write(STDOUT_FILENO, "\r", 1);
                display();
                write(STDOUT_FILENO, buf.c_str(), buf.size());
            }
        } else if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            restore_term();
            out = buf;
            return true;
        } else if (c == '\t') {
            size_t start = (cursor==0)?0:buf.rfind(' ', cursor-1);
            if (start == string::npos) start = 0; else start = start + 1;
            string prefix = buf.substr(start, cursor - start);
            vector<string> matches = list_dir_matches(prefix);
            if (matches.empty()) {
                const char *bell = "\a";
                write(STDOUT_FILENO, bell, 1);
            } else if (matches.size() == 1) {
                string rest = matches[0].substr(prefix.size());
                buf.insert(cursor, rest);
                cursor += rest.size();
                write(STDOUT_FILENO, rest.c_str(), rest.size());
            } else {
                write(STDOUT_FILENO, "\n", 1);
                string listing;
                for (auto &m: matches) listing += m + "  ";
                listing += "\n";
                write(STDOUT_FILENO, listing.c_str(), listing.size());
                display();
                write(STDOUT_FILENO, buf.c_str(), buf.size());
            }
        } else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    if (!history_lines.empty() && history_index > 0) {
                        history_index--;
                        buf = history_lines[history_index];
                        cursor = buf.size();
                        write(STDOUT_FILENO, "\r", 1);
                        display();
                        write(STDOUT_FILENO, buf.c_str(), buf.size());
                    }
                } else if (seq[1] == 'B') {
                    if (!history_lines.empty() && history_index < (int)history_lines.size()-1) {
                        history_index++;
                        buf = history_lines[history_index];
                        cursor = buf.size();
                    } else {
                        history_index = history_lines.size();
                        buf.clear();
                        cursor = 0;
                    }
                    write(STDOUT_FILENO, "\r", 1);
                    display();
                    write(STDOUT_FILENO, buf.c_str(), buf.size());
                } else if (seq[1] == 'C') {
                    if (cursor < buf.size()) {
                        char ch = buf[cursor++];
                        write(STDOUT_FILENO, &ch, 1);
                    }
                } else if (seq[1] == 'D') {
                    if (cursor > 0) {
                        write(STDOUT_FILENO, "\b", 1);
                        cursor--;
                    }
                }
            }
        } else if (isprint((unsigned char)c)) {
            buf.insert(buf.begin()+cursor, c);
            cursor++;
            write(STDOUT_FILENO, &c, 1);
        }
    }

    restore_term();
    return false;
}
