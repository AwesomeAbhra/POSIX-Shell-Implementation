# POSIX Shell Implementation

## Overview

#### This project implements a custom POSIX shell in C++ with support for basic shell commands (cd, pwd, echo, ls, search, pinfo) which were implemented manually, foreground and background commands execution, piping, and redirection. It also has the commands like history to check last used commands. It also has simple signals like ctrl-D, ctrl-C and ctrl-Z.

## Features

* Customized prompt display
* Background and Foreground Command Execution
* changing directory using cd command
* pwd, echo commands execution
* ls commands with different flags
* commands like search and pinfo execution
* IO Redirection and pipeline
* stores last 20 commands history
* ctrl-D, ctrl-C and ctrl-Z sinals


## Implementation Details and File Structure

#### There are total 11 cpp files and one makefile and one header file. To run this project you need to open terminal in the same folder as of these files. Then run make command. It will compile all the files together and create one executable file name shell. Then run ./shell command and the virtual POSIX shell environment will start. After the execution of this program you can run make clean command to remove all the executable files including shell.