# myShellC

## Overview
This project is a simplified shell, similar to those in Unix systems, which can be executed in two modes: interactive and batch. In interactive mode, the shell displays a prompt named after your email login, and you can enter commands directly. In batch mode, the shell executes a list of commands from a provided file (`batchFile`), displaying each command on the terminal before execution.

## Features
1. **Two Execution Styles**: 
   - Sequential: Commands execute one after the other.
   - Parallel: Commands execute concurrently.

2. **Command Execution**: 
   - Multiple commands per line, separated by semicolons (`;`).
   - Command 'style' switch between sequential (`seq`) and parallel (`par`).

3. **Support for Pipe (`|`)**: Output of one command can be piped as input to another.

4. **I/O Redirection**: Ability to redirect input and output to/from files.

5. **History Feature (`!!`)**: Re-executes the last command entered.

6. **Background Execution (`&`)**: Run commands in the background.

## Usage
- **Interactive Mode**: Launch with `./shell`. Enter commands at the prompt.
- **Batch Mode**: Launch with `./shell [batchFile]`. Executes commands from the file.

## Installation
1. Clone the repository.
2. Compile using `make`.
