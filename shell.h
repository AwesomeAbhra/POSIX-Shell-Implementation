#ifndef SHELL_H
#define SHELL_H
#include <string>
#include <vector>
extern std::vector<std::string> history_lines;
extern pid_t foregroundPid;
void setupSignalHandlers();
void display();
void append_to_history(const std::string &cmd);
void save_history_to_file(const std::string &path);
void load_history_from_file(const std::string &path);
void parsecommand(char *input);
void handle_pipeline(const std::vector<std::string>& pipe_segments);
void handlecommand(std::vector<std::string>& tokens);
void executecommand(const std::vector<std::string>& tokens, bool background, int in_fd, int out_fd);
std::vector<std::string> split_tokens(const std::string &s);
std::vector<std::string> tokenize(const std::string& command);
std::string trim(const std::string &s);
void handleCD(const std::vector<std::string>& tokens);
void handleEcho(const std::vector<std::string>& tokens);
void handleLS(const std::vector<std::string>& tokens);
void handlePWD(const std::vector<std::string>& tokens);
void handlepinfo(const std::vector<std::string>& tokens);
void handlesearch(const std::vector<std::string>& tokens);
void display_history(const std::vector<std::string>& tokens);
void handleExplain(const std::vector<std::string>& tokens);
std::vector<std::string> get_files_in_directory(const std::string& path);
#endif