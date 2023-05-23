#include <iostream>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <limits.h>
#include <string>

#define TOKENSIZE 100

using namespace std;
namespace filesystem = std::filesystem;

void StrTokenizer(char *line, char **argv);
void ExecuteCommand(char **argv);
void TouchCommand(char *filename);
void PwdCommand();
void RmCommand(char *filename);
void GrepCommand(char *pattern, char *filename);
void ManCommand(char *command);
void RmdirCommand(char *directory);
void CatCommand(char *filename);
void WriteCommand(char *filename);
void WriteAtCommand(char *filename, int offset);
void ReadCommand(char *filename);
void ReadAtCommand(char *filename, int offset);
void HeadCommand(char *filename, int lines);
void TailCommand(char *filename, int lines);
string cdCommand(const string& dir);

bool cdused = false;
string input;

int main()
{
    string command;
    while (true)
    {
    	if (cdused) {
    		string cwd = cdCommand(input);
    		cout << cwd << "$ ";
    		cdused = false;
    	}
    	else {
    		char cwd[PATH_MAX];
    		if (getcwd(cwd, sizeof(cwd)) != NULL) {
        		cout << cwd << "$ ";
    		}
    		else {
        		cout << "ERROR: Failed to get current working 	directory" << endl;
    		}
        	
	}
	getline(cin, command);
        if (command == "exit")
            break;

        vector<string> commands;
        string delimiter = "|";
        size_t pos = 0;
        while ((pos = command.find(delimiter)) != string::npos)
        {
            string token = command.substr(0, pos);
            commands.push_back(token);
            command.erase(0, pos + delimiter.length());
        }
        commands.push_back(command);

        size_t numCommands = commands.size();
        int prevRead = STDIN_FILENO;

        for (size_t i = 0; i < numCommands; ++i)
        {
            int pipefd[2];
            pipe(pipefd);

            pid_t pid = fork();
            if (pid == 0)
            {
                // Child process
                close(pipefd[0]);  // Close unused read end of the pipe

                dup2(prevRead, STDIN_FILENO);  // Redirect input from previous command

                if (i != numCommands - 1)
                {
                    dup2(pipefd[1], STDOUT_FILENO);  // Redirect output to the next command
                }

                // Split the command into arguments
                vector<string> args;
                string argDelimiter = " ";
                size_t argPos = 0;
                while ((argPos = commands[i].find(argDelimiter)) != string::npos)
                {
                    string arg = commands[i].substr(0, argPos);
                    args.push_back(arg);
                    commands[i].erase(0, argPos + argDelimiter.length());
                }
                args.push_back(commands[i]);

                // Convert the arguments to a null-terminated array of C-style strings
                vector<char*> argv;
                for (const auto& arg : args)
                    argv.push_back(const_cast<char*>(arg.c_str()));
                argv.push_back(nullptr);

                // Execute the command
                ExecuteCommand(argv.data());

                exit(0);
            }
            else if (pid > 0)
            {
                // Parent process
                close(pipefd[1]);  // Close unused write end of the pipe
                wait(nullptr);     // Wait for the child process to finish

                prevRead = pipefd[0];  // Set the read end of the pipe for the next command
            }
            else
            {
                // Fork failed
                cerr << "Fork failed" << endl;
                exit(1);
            }
        }
    }

    return 0;
}

void ExecuteCommand(char **argv)
{
    if (strcmp(argv[0], "touch") == 0)
    {
        TouchCommand(argv[1]);
    }
    else if (strcmp(argv[0], "pwd") == 0)
    {
        PwdCommand();
    }
    else if (strcmp(argv[0], "rm") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        RmCommand(argv[1]);
    }
    else if (strcmp(argv[0], "grep") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing pattern" << endl;
            return;
        }
        if (argv[2] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[3] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        GrepCommand(argv[1], argv[2]);
    }
    else if (strcmp(argv[0], "man") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing command" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        ManCommand(argv[1]);
    }
    else if (strcmp(argv[0], "rmdir") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing directory" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        RmdirCommand(argv[1]);
    }
    else if (strcmp(argv[0], "cat") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        CatCommand(argv[1]);
    }
    else if (strcmp(argv[0], "write") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        WriteCommand(argv[1]);
    }
    else if (strcmp(argv[0], "writeat") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] == NULL)
        {
            cout << "ERROR: Missing offset" << endl;
            return;
        }
        if (argv[3] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        int offset = atoi(argv[2]);
        WriteAtCommand(argv[1], offset);
    }
    else if (strcmp(argv[0], "read") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        ReadCommand(argv[1]);
    }
    else if (strcmp(argv[0], "readat") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] == NULL)
        {
            cout << "ERROR: Missing offset" << endl;
            return;
        }
        if (argv[3] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        int offset = atoi(argv[2]);
        ReadAtCommand(argv[1], offset);
    }
    else if (strcmp(argv[0], "head") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] == NULL)
        {
            cout << "ERROR: Missing number of lines" << endl;
            return;
        }
        if (argv[3] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        int lines = atoi(argv[2]);
        HeadCommand(argv[1], lines);
    }
    else if (strcmp(argv[0], "tail") == 0)
    {
        if (argv[1] == NULL)
        {
            cout << "ERROR: Missing filename" << endl;
            return;
        }
        if (argv[2] == NULL)
        {
            cout << "ERROR: Missing number of lines" << endl;
            return;
        }
        if (argv[3] != NULL)
        {
            cout << "ERROR: Too many arguments" << endl;
            return;
        }
        int lines = atoi(argv[2]);
        TailCommand(argv[1], lines);
    }
    else if (strcmp(argv[0], "cd") == 0) {
    	if (argv[1] == NULL) {
        	cout << "ERROR: Missing directory" << endl;
        	return;
    	}
    	if (argv[2] != NULL) {
        	cout << "ERROR: Too many arguments" << endl;
        	return;
    	}
    	 string dirString(argv[1]);
   	 cdCommand(dirString);
	}
	else {
        	pid_t pid;
        	int status;
        	int childStatus;
        	pid = fork();
        	if (pid == 0)
        	{
            		childStatus = execvp(argv[0], argv);
            		if (childStatus < 0)
            		{
                		cout << "ERROR: Command execution failed" << endl;
            		}
            		exit(0);
        	}
        	else if (pid < 0)
        	{
        	    cout << "ERROR: Fork failed" << endl;
        	}
        	else
        	{
        	    waitpid(pid, &status, 0);
        	}
    	}
}
// touch: create file
void TouchCommand(char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        cout << "ERROR: Failed to create file" << endl;
    }
    else
    {
        fclose(file);
        cout << "File created: " << filename << endl;
    }
}
// pwd: print current working directory
void PwdCommand()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        cout << "Current working directory: " << cwd << endl;
    }
    else
    {
        cout << "ERROR: Failed to get current working directory" << endl;
    }
}

// rm: remove directory
void RmCommand(char *filename)
{
    if (remove(filename) == 0)
    {
        cout << "File/directory removed: " << filename << endl;
    }
    else
    {
        cout << "ERROR: Failed to remove file/directory" << endl;
    }
}

// Grep Command: matches chars in file 
void GrepCommand(char *pattern, char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        cout << "ERROR: Failed to open file" << endl;
        return;
    }

    char line[1000];
    while (fgets(line, sizeof(line), file))
    {
        char *match = strstr(line, pattern);
        if (match != NULL)
        {
            // Printing the line until the matched pattern
            cout << string(line, match - line);

            // Applying highlighting to the matched pattern
            cout << "\033[1;31m" << string(match, strlen(pattern)) << "\033[0m";
            // rest of line
            cout << string(match+strlen(pattern));
            // For example, "\033[1m" for bold, "\033[4m" for underline, etc.
        }
    }

    fclose(file);
}

void ManCommand(char *command)
{
    string manualCommand = "man ";
    manualCommand += command;
    system(manualCommand.c_str());
}

void RmdirCommand(char *directory)
{
    string removeCommand = "rmdir ";
    removeCommand += directory;
    int status = system(removeCommand.c_str());
    if (status != 0)
    {
        cout << "ERROR: Failed to remove the directory" << endl;
    }
}

void CatCommand(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        cout << "ERROR: Failed to open file" << endl;
        return;
    }

    char line[1000];
    while (fgets(line, sizeof(line), file))
    {
        cout << line;
    }

    fclose(file);
}

void WriteCommand(char *filename)
{
    ofstream outfile(filename);
    if (outfile)
    {
        string line;
        while (getline(cin, line))
        {
            if (line == ":wq")
                break;
            outfile << line << endl;
        }
        outfile.close();
        cout << "File written: " << filename << endl;
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

void WriteAtCommand(char *filename, int offset)
{
    fstream file(filename, ios::in | ios::out);
    if (file)
    {
        file.seekp(offset);
        string line;
        while (getline(cin, line))
        {
            if (line == ":wq")
                break;
            file << line << endl;
        }
        file.close();
        cout << "File written at offset " << offset << ": " << filename << endl;
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

void ReadCommand(char *filename)
{
    ifstream file(filename);
    if (file)
    {
        string line;
        while (getline(file, line))
        {
            cout << line << endl;
        }
        file.close();
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

void ReadAtCommand(char *filename, int offset)
{
    ifstream file(filename);
    if (file)
    {
        file.seekg(offset);
        string line;
        while (getline(file, line))
        {
            cout << line << endl;
        }
        file.close();
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

void HeadCommand(char *filename, int lines)
{
    ifstream file(filename);
    if (file)
    {
        string line;
        int count = 0;
        while (getline(file, line) && count < lines)
        {
            cout << line << endl;
            count++;
        }
        file.close();
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

void TailCommand(char *filename, int lines)
{
    ifstream file(filename);
    if (file)
    {
        string line;
        vector<string> buffer;
        while (getline(file, line))
        {
            buffer.push_back(line);
            if (buffer.size() > lines)
            {
                buffer.erase(buffer.begin());
            }
        }
        for (const auto &line : buffer)
        {
            cout << line << endl;
        }
        file.close();
    }
    else
    {
        cout << "ERROR: Failed to open file" << endl;
    }
}

string cdCommand(const string& dir)
{
    if (chdir(dir.c_str()) == 0)
    {
        cdused = true;
    	input = string(dir);
    	// Update the current working directory for the main process
    	char cwd[PATH_MAX];
    	if (getcwd(cwd, sizeof(cwd)) != NULL)
    	{
        	cout << "Current working directory changed to: " << cwd << endl;
        	return string(cwd);
    	}
    	else
    	{	
        	cout << "ERROR: Failed to get current working directory" << endl;
    	}
    }
    else {
    	cout << "ERROR: Can't execute command" << endl;
    	return "";
    }
    return "";
}

void StrTokenizer(char *command, char **argv)
{
    char *stringTokenized;
    stringTokenized = strtok(command, " ");
    while (stringTokenized != NULL)
    {
        *argv++ = stringTokenized;
        stringTokenized = strtok(NULL, " ");
    }

    *argv = NULL;
}