#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

/ Constants:

#define SMASH_DEFAULT_PROMPT   "smash> "
#define SMASH_BASH_PATH        "/bin/bash"
#define SMASH_MAX_PATH         2048
#define SMASH_PRINT_WITH_PERROR(ERR, CMD)                       perror(ERR(CMD));
#define CHDIR     "chdir"
#define SMASH_SYSCALL_FAILED_ERROR(CMD)                         "smash error: " CMD " failed"




string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 


// Utilities:

std::string removeFirstWord(std::string sentence){
    std::string::size_type n = 0;
    n = sentence.find_first_not_of( " \t", n );
    n = sentence.find_first_of( " \t", n );
    sentence.erase( 0,  sentence.find_first_not_of( " \t", n ) );
    return sentence;
}

std::string getFirstWord(const char* cmd_line){
  return cmd_s.substr(0, cmd_s.find_first_of(" \n"));
}


// Command:

  Command::Command(const char* cmd_line) : full_command(cmd_line), command_without_bg(cmd_line), is_bg_coomand(false) {
    this->full_command = cmd_line;
    if(_isBackgroundComamnd(cmd_line)){
      this->is_bg_coomand = true;
      this->command_without_bg = _removeBackgroundSign(cmd_line;)
    }
    else{
        this->is_bg_coomand = false;
        this->command_without_bg = cmd_line; 
    }
    return;
  }

  bool Command::isBgCommand() {
    return this->is_bg_coomand;
  }








// SmallShell:

SmallShell::SmallShell() : prevCommand("") currPrompt(""), prevDir(""){
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = getFirstWord(cmd_s);

  if (firstWord.compare("chprompt") == 0) {
    return new ChPromptCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
 
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
 
  Command* cmd = CreateCommand(cmd_line);
  // if (external || pipe)
  // need to fork
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)


  this->changePrevCommand(cmd_line); /// change prevCommand to the recieved command
}

std::string SmallShell::getCurrPrompt(){
  return this->currPrompt;
}

void SmallShell::changePrompt(std::string prompt){
  this->currPrompt = prompt;
}

std::string SmallShell::getPrevDir(){
  return this->prevDir;
}

void SmallShell::changePrevDir(std::string Dir){
  this->prevDir = Dir;
}

// chprompt command:

ChPromptCommand::ChPromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  std::string cmd = _trim(string(this->command_without_bg));
  cmd = removeFirstWord(cmd);
  if(cmd.length()>0){
    this->prompt = cmd + "> ";
  }
  else {
    this->prompt = SMASH_DEFAULT_PROMPT;
  }
}

ChPromptCommand::execute() : BuiltInCommand(cmd_line) {
    SmallShell& smash = SmallShell::getInstance();
    smash->changePrompt(this->prompt);
}

// ShowPidCommand :

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

ShowPidCommand::execute() {
  cout << "smash pid is" << getpid() << endl;
}

// pwd:

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

GetCurrDirCommand::execute() {
  char buffer[SMASH_MAX_PATH + 1] = "";
  if (getcwd(buffer, SMASH_MAX_PATH)) {
        cout << buffer << endl;
  } else {
        cout << "pwd error" << endl;
  }
}

// cd:

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line), isValidNumArg(false), dir("") {
  std::string cmd = _trim(string(this->command_without_bg));
  cmd = removeFirstWord(cmd);
  if(removeFirstWord(cmd).length > 0 ) { // too many arguments
    return;
  }
  else {
    this->isValidNumArg = true;
    this->dir = cmd;
    return;
  }
}

ChangeDirCommand::execute() {
  if(!this->isValidNumArg){
    cout << "smash error: cd: too many arguments" << endl;
    return;
  }
  else if (this->dir.compare("-") == 0) {
    SmallShell &smash = SmallShell::getInstance();
    prevDir = smash.getPrevDir();
    // in case that cd command has not yet been called:
    if(prevDir.length() == 0) {
        cout << "smash error: cd: OLDPWD not set" << endl;
        return;
    }
    // there is a prev dir:
    else{
      cout << prevDir << endl; 
    }
  }
  // got a dir to chage to:
  else {
    char buffer[SMASH_MAX_PATH + 1] = "";
    if (!getcwd(buffer, SMASH_MAX_PATH))
    {
        cerr << "pwd error" << endl;
    }
        // call the syscall. if succeded, save the old directory:
        if (chdir(this->dir.c_str()) != -1)
        {
            SmallShell &smash = SmallShell::getInstance();
            smash.changePrevDir(string(buffer));
        }
        else{
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CHDIR);
            return;
        }
    
  }
}
