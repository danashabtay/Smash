#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <algorithm>
#include <fcntl.h> 
#include <sys/stat.h>
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

// Constants:

// Constants:

#define SMASH_DEFAULT_PROMPT   "smash> "
#define SMASH_MAX_PATH         2048
#define SMASH_BIG_PATH         128
#define SMASH_MAX_ARGS         21
#define DEFAULT_LINES_FOR_TAIL 10
#define SMASH_BASH_PATH        "/bin/bash"
#define SMASH_C_ARG            "-c"
#define TIME_SEPERATOR         ":"
#define YEARS_TO_SUBSTRACT     1900
#define MONTHS_TO_SUBSTRACT    1
#define DASH                   "-"
#define REDIRECTION_CHAR       ">"
#define PIPE_CHAR              "|"
#define STDERR_PIPE_PREFIX     "&"

#define CD        "cd"
#define OPEN      "open"
#define FG        "fg"
#define BG        "bg"
#define FORK      "fork"
#define KILL      "kill"
#define CHDIR     "chdir"
#define KILL      "kill"
#define FORK      "fork"
#define EXECV     "execv"
#define DUP       "dup"
#define CLOSE     "close"
#define READ      "read"
#define WRITE     "write"
#define PIPE      "pipe"
#define TAIL      "tail"
#define TOUCH     "touch"
#define UTIME     "utime"
#define MKTIME    "mktime"
#define CHMOD     "chmod"


#define SMASH_PRINT_WITH_PERROR(ERR, CMD)                       perror(ERR(CMD));
#define SMASH_PRINT_ERROR(ERR, CMD)                             cerr << ERR(CMD) << endl;
#define SMASH_PRINT_ERROR_JOB_ID(ERR_START, ERR_END, CMD, ID)   cerr <<  ERR_START(CMD) << ID << ERR_END << endl;
#define SMASH_PRINT_COMMAND_WITH_PID(CMD, PID)                  cout << CMD << " : " << PID << endl;
#define SMASH_SYSCALL_FAILED_ERROR(CMD)                         "smash error: " CMD " failed"
#define SMASH_INVALID_ARGS_ERROR(CMD)                           "smash error: " CMD ": invalid arguments"
#define SMASH_JOBS_LIST_EMPTY_ERROR(CMD)                        "smash error: " CMD ": jobs list is empty"
#define SMASH_NO_STOPPED_JOBS_IN_LIST(CMD)                      "smash error: " CMD ": there is no stopped jobs to resume"
#define SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START(CMD)           "smash error: " CMD ": job-id "
#define SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END                  " does not exist"
#define SMASH_JOB_JOB_ALREADY_IN_BG_ERROR_START(CMD)            "smash error: " CMD ": job-id "
#define SMASH_JOB_JOB_ALREADY_IN_BG_ERROR__END                  " is already running in the background"


const std::string WHITESPACE = " \n\r\t\f\v";

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




// Utilities:

std::string removeFirstWord(std::string sentence){
    std::string::size_type n = 0;
    n = sentence.find_first_not_of( " \t", n );
    n = sentence.find_first_of( " \t", n );
    sentence.erase( 0,  sentence.find_first_not_of( " \t", n ) );
    return sentence;
}

string getFirstWord(const char *line)
{
    if (!line)
    {
        throw std::invalid_argument("NULL argument");
    }
    string cmd_s = _trim(string(line));
    return cmd_s.substr(0, cmd_s.find_first_of(" \n"));
}


string getFirstWord(const string& line)
{
    return getFirstWord(line.c_str());
}


// Command:

int extractNumber(const std::string& str) {
    int number = 0;
    bool foundNumber = false;

    for (char c : str) {
        if (isdigit(c)) {
            foundNumber = true;
            number = number * 10 + (c - '0');
        } else if (foundNumber) {
            // If we already found the number and encounter a non-digit character, break
            break;
        }
    }

    return number;
}

  Command::Command(const char* cmd_line) : full_command(cmd_line), command_without_bg(cmd_line), is_bg_coomand(false) {
    this->full_command = cmd_line;
    this->duration = extractNumber(cmd_line);
    this->is_Timed = false;
    if(duration){
      this->is_Timed=true;
    }
    if(_isBackgroundComamnd(cmd_line)){
      this->is_bg_coomand = true;
      char* str =  const_cast<char*>(cmd_line);
      _removeBackgroundSign(str);
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

  std::string Command::getFullCommand(){
    return this->full_command;
  }


  bool Command::getIsTimed(){
    return this->is_Timed;
  }
  
  int Command::getDuration(){
    return this->duration;
  }

  std::string Command::getCommandWOBg(){
    return this->command_without_bg;
  }



// SmallShell:

SmallShell::SmallShell() : prevCommand(""), currPrompt(SMASH_DEFAULT_PROMPT), prevDir(""), jobs(), current_job(nullptr){
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
  if (cmd_s[cmd_s.length() - 1] == '&'){
        cmd_s = _trim(cmd_s.substr(0, cmd_s.length() - 1));
    }
    //checking if there is a redirection request
       if (cmd_s.find(REDIRECTION_CHAR) != string::npos)
    {
        return new RedirectionCommand(cmd_line);
    }
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  //built-in commnads:
  if (firstWord.compare("chprompt") == 0) {
    return new ChPromptCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, nullptr);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, &(this->jobs));
  }
  
 // External commands:
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
 
  Command* cmd = CreateCommand(cmd_line);
  if(cmd==nullptr){
    return;
  }
  // if (external || pipe)
  // need to fork
  this->jobs.removeFinishedJobs();
  cmd->execute();
  delete cmd;
  // Please note that you must fork smash process for some commands (e.g., external commands....)


  this->changePrevCommand(cmd_line); /// change prevCommand to the recieved command
}

std::string SmallShell::getCurrPrompt(){
  return this->currPrompt;
}

void SmallShell::changePrompt(std::string prompt){
  this->currPrompt = prompt;
}

void SmallShell::changePrevCommand(const std::string cmd){
  this->prevCommand = cmd;
}

std::string SmallShell::getPrevDir(){
  return this->prevDir;
}

void SmallShell::changePrevDir(std::string Dir){
  this->prevDir = Dir;
}


//jobList:
 
 JobsList::JobEntry::JobEntry(int job_id, pid_t pid, bool is_stopped_by_user, bool is_timed, int duration, std::string command) 
 : job_id(job_id), pid(pid), is_stopped_by_user(is_stopped_by_user), is_timed(is_timed), duration(duration), command(command) {
    time_t time1;
    this->insert_time = time(&time1);
    if(time1 != this->insert_time){
        cout <<"time error" << endl;
    }
 }

JobsList::JobEntry& JobsList::JobEntry::operator=(JobsList::JobEntry &other) {
  if(this==&other){
    return *this;
  }
  this->job_id=other.getJobId();
  this->pid=other.getPid();
  this->is_stopped_by_user=other.isStopped();
  this->is_timed=other.isTimed();
  this->insert_time=other.getInsertTime();
  this->duration=other.getDuration();
  this->command=other.getCommand();

  return *this;
}

JobsList::JobEntry::~JobEntry() {}

int JobsList::JobEntry::getJobId(){
  return this->job_id;
}

pid_t JobsList::JobEntry::getPid(){
  return this->pid;
}

bool JobsList::JobEntry::isStopped(){
  return this->is_stopped_by_user;
}

bool JobsList::JobEntry::isTimed(){
  return this->is_timed;
}

time_t JobsList::JobEntry::getInsertTime(){
  return this->insert_time;
}

std::string JobsList::JobEntry::getCommand(){
  return this->command;
}

int JobsList::JobEntry::getDuration(){
  return this->duration;
}

int JobsList::getMaxJobId() {
  int max_id = 0;
    for(const auto& job: this->jobsList){
        max_id = max(max_id, job->getJobId());
    }
    return max_id;
}

shared_ptr<JobsList::JobEntry> JobsList::getJobByPid(const int& jobPid) const
{
    if(jobPid <= 0){
        return std::shared_ptr<JobsList::JobEntry>(nullptr);
    }
    for(const auto& job: this->jobsList){
        if(job->getPid() == jobPid){
            return job;
        }
    }
    return std::shared_ptr<JobsList::JobEntry>(nullptr);
}

std::vector<std::shared_ptr<JobsList::JobEntry>> JobsList::getAllJobs() const
{
    return jobsList;
}

int JobsList::getJobsNum() const
{
    return this->jobsList.size();
}

void JobsList::removeJobById(const int& jobId)
{
    if(jobId <= 0){
        return;
    }
    for (auto iter = jobsList.begin(); iter != jobsList.end();) {
        if ((*iter)->getJobId() == jobId){
            iter = jobsList.erase(iter);
        }
        else{
            iter++;
        }
    }
}

shared_ptr<JobsList::JobEntry> JobsList::getJobById(const int& jobId) const
{
    if(jobId <= 0){
        return std::shared_ptr<JobsList::JobEntry>(nullptr);
    }
    for(const auto& job: this->jobsList){
        if(job->getJobId() == jobId){
            return job;
        }
    }
    return std::shared_ptr<JobsList::JobEntry>(nullptr);
}

bool JobsList::JobEntry::operator>(JobsList::JobEntry& other){
    return this->getJobId() > other.getJobId();
}

bool JobsList::JobEntry::operator<( JobsList::JobEntry& other){
    return this->getJobId() < other.getJobId();
}

bool JobsList::JobEntry::operator==( JobsList::JobEntry& other){
    return this->getJobId() == other.getJobId();
}

bool JobsList::JobEntry::operator!=( JobsList::JobEntry& other){
    return this->getJobId() != other.getJobId();
}

void JobsList::JobEntry::setAsStopped(){
    this->is_stopped_by_user = true;
}

void JobsList::JobEntry::setAsResumed(){
    this->is_stopped_by_user = false;
}

JobsList::JobsList() : jobsList() {}

JobsList::~JobsList() {}


void JobsList::addJob(Command* cmd, const pid_t& pid, bool isStopped){
  this->removeFinishedJobs();
  int newJobId = this->getMaxJobId()+1;
  shared_ptr<JobEntry> new_job(nullptr);
    try{
        new_job = make_shared<JobEntry>(newJobId,pid,isStopped,cmd->getIsTimed(),cmd->getDuration(),cmd->getFullCommand());
    }
    catch(const std::bad_alloc& e){
        cout << e.what() << endl;
    }
    this->jobsList.push_back(new_job);
}
  
  bool JobsList::JobEntry::jobWasStopped()
{
    return this->isStopped();
}

void JobsList::printJobsList() {
  std::sort(jobsList.begin(), jobsList.end(), JobsList::JobIsBigger());
    for(const auto& job: this->jobsList){
        string jobToPrint = job->getCommand();
        time_t current_time;
        time(&current_time);
        time_t elapsed_time = difftime(current_time, job->getInsertTime());

        if (job->isTimed())
        {
            jobToPrint = "timeout " + std::to_string(job->getDuration()) + " " + job->getCommand();
        }

        cout << "[" <<  job->getJobId() << "] " << jobToPrint << " : " << job->getPid() << " " << elapsed_time << " secs";
        if(job->jobWasStopped()){
            cout << " (stopped)";
        }
        cout << endl;
    }
} 

void JobsList::removeFinishedJobs() {
{
  for (auto it = jobsList.begin(); it != jobsList.end();)
  {
    int status = 0;
    pid_t child_pid = waitpid((*it)->getPid(), &status, WNOHANG);
    if (child_pid > 0) {
      it = jobsList.erase(it);
    }
    else
    {
      if (child_pid == -1)
      {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, "waitpid");
      }
      ++it;
    }
  }
}
}

void JobsList::killAllJobs()
{
    
        for(auto& job: this->jobsList){
            cout << job->getPid() << ": " << job->getCommand() << endl;
            if (kill(job->getPid(), SIGKILL) == -1){
                SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
                return;
            }
        }
    
    this->jobsList.clear();
}

// chprompt command:

ChPromptCommand::ChPromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  std::string cmd = _trim(string(this->getCommandWOBg()));
  cmd = removeFirstWord(cmd);
  if(cmd.length()>0){
    this->prompt = cmd + "> ";
  }
  else {
    this->prompt = SMASH_DEFAULT_PROMPT;
  }
}

void ChPromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.changePrompt(this->prompt);
}

// ShowPidCommand :

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}

// pwd:

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
  char buffer[SMASH_MAX_PATH + 1] = "";
  if (getcwd(buffer, SMASH_MAX_PATH)) {
        cout << buffer << endl;
  } else {
        cout << "pwd error" << endl;
  }
}

// cd:

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line), isValidNumArg(false), dir("") {
  std::string cmd = _trim(string(this->getCommandWOBg()));
  cmd = removeFirstWord(cmd);
  if(removeFirstWord(cmd).length() > 0 ) { // too many arguments
    return;
  }
  else {
    this->isValidNumArg = true;
    this->dir = cmd;
    return;
  }
}

void changeDirectory(std::string path) {
  char buffer[SMASH_MAX_PATH + 1] = "";
    if (!getcwd(buffer, SMASH_MAX_PATH))
    {
        cerr << "pwd error" << endl;
    }
        // call the syscall. if succeded, save the old directory:
        if (chdir(path.c_str()) != -1)
        {
            SmallShell &smash = SmallShell::getInstance();
            smash.changePrevDir(string(buffer));
        }
        else{
            SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CHDIR);
            return;
        }
}

void ChangeDirCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  std::string prevDir = smash.getPrevDir();
  
  if(!this->isValidNumArg){
    cout << "smash error: cd: too many arguments" << endl;
    return;
  }
  else if (this->dir.compare("-") == 0) {
    // in case that cd command has not yet been called:
    if(prevDir.length() == 0) {
        cout << "smash error: cd: OLDPWD not set" << endl;
        return;
    }
    // there is a prev dir:
    else{
      changeDirectory(prevDir); 
    }
  }
  // got a dir to chage to:
  else {
    changeDirectory(this->dir);
  }
}

//jobs:

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.removeFinishedJobs();
    smash.jobs.printJobsList();
}

// fg:

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void ForegroundCommand::execute()
{
    string args = removeFirstWord(this->getFullCommand());
    string job_id_s = getFirstWord(args); // if empty, try getting the max job-id.
    string remainnig = removeFirstWord(args); // supposed to be empty.
    int job_id = 0;
    //In case of no arguments:
    if (job_id_s.length() == 0){
        job_id = this->jobs_list->getMaxJobId();
        if (job_id == 0){
            SMASH_PRINT_ERROR(SMASH_JOBS_LIST_EMPTY_ERROR, FG);
            return;
        }
    }
        //In case of two or more arguments:
    else if (remainnig.length() > 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, FG);
        return;
    }
        //In case of one arguments:
    else{
        try{
            job_id = stoi(job_id_s); //check if works
        }
        catch (std::invalid_argument&){
            SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, FG);
            return;
        }
    }
    //check if a job with this id exists:
    shared_ptr<JobsList::JobEntry> job = this->jobs_list->getJobById(job_id); // need to add jobbyid & MACROS
    if (job == nullptr){
        SMASH_PRINT_ERROR_JOB_ID(SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START, SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END, FG, job_id);
        return;
    }
    // execute:
    SMASH_PRINT_COMMAND_WITH_PID(job->getCommand(),job->getPid());
    SmallShell& smash = SmallShell::getInstance();
    smash.current_job = job;
    if (kill(job->getPid(), SIGCONT) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
    }
    int status;

    // smash.current_job = job;
    waitpid(job->getPid(), &status, WUNTRACED);

    if (!WIFSTOPPED(status)) {
        jobs_list->removeJobById(job->getPid()); //removenyid
        smash.current_job = nullptr;
        return;
    }
    job->setAsStopped();
}

// quit / quitkill:

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void QuitCommand::execute()
{
    string argument = getFirstWord(removeFirstWord(this->getFullCommand()));
    if(argument.compare("kill") != 0){
        exit(0);
    }
    if(jobs_list->getJobsNum()==0){
          cout << "smash: sending SIGKILL signal to 0 jobs:" << endl;
          exit(0);
    }
    cout << "smash: sending SIGKILL signal to " << this->jobs_list->getJobsNum() << " jobs:" << endl;
    this->jobs_list->killAllJobs();
    exit(0);
}



// kill:

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs_list(jobs){}

void KillCommand::execute() {
  string numSig_s = getFirstWord(removeFirstWord(this->getFullCommand()));
  string job_id_s = getFirstWord(removeFirstWord(numSig_s));
  string remaining = removeFirstWord(job_id_s); // supposed to be empty.
  int job_id=0;
  int num_sig=0;
  
  try{
    job_id = stoi(job_id_s); //check if works
  }
  catch (std::invalid_argument&){
    SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
    return;
  }

  try{
    num_sig = stoi(numSig_s); //check if works
  }
  catch (std::invalid_argument&){
    SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
    return;
  }

  if (remaining.length() > 0){
        SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, KILL);
        return;
    }
  //check if a job with this id exists:
    shared_ptr<JobsList::JobEntry> job = this->jobs_list->getJobById(job_id); // need to add jobbyid & MACROS
    if (job == nullptr){
        SMASH_PRINT_ERROR_JOB_ID(SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_START, SMASH_JOB_ID_DOES_NOT_EXISTS_ERROR_END, FG, job_id);
        return;
    }



  if(kill(job->getPid(),num_sig) == -1){
    SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, KILL);
  }
  else {
    if(num_sig == SIGSTOP || num_sig == SIGTSTP){
      job->setAsStopped();
    }
    else if (num_sig == SIGKILL) {
      jobs_list->removeJobById(job_id);
    }
    else if(num_sig == SIGCONT) {
      job->setAsResumed();
    }
  }
  cout << "signal number " << num_sig << " was sent to pid " << job->getPid() << endl;

}

//-------- BuiltInCommand Class:

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
  this->is_bg_coomand=false;
}

// external:

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line){}

bool isComplexCommand(const char* command) {
    // Check if the command contains wildcard characters (* or ?)
    return (strchr(command, '*') != nullptr || strchr(command, '?') != nullptr);
}

void ExternalCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  pid_t new_pid = fork();
  if (new_pid < 0){
     SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, FORK);
  }
  else if (new_pid > 0){ //father:
    // if bg command add to jobs list:
    if(this->getFullCommand().compare("") != 0) {
    smash.jobs.addJob(this,new_pid,false);
    }
    // if not bg then set as current command and remove from jobs list:
    if(!this->isBgCommand()){
    smash.current_job = smash.jobs.getJobByPid(new_pid);
    int id = smash.current_job->getJobId();
    smash.jobs.removeJobById(id);
    waitpid(new_pid, NULL, WUNTRACED);
    smash.current_job = nullptr;
    }
  }
  else { //son:
    if(this->getFullCommand().compare("") != 0) {
    {
    if(isComplexCommand(this->getFullCommand().c_str())) {
      //simple command:
      char* args[SMASH_MAX_PATH+1];
      int arg_count = 0;
      char* stringToTokenize = new char[SMASH_MAX_PATH+1]; //for converting from string to char*. size as path size
      strcpy(stringToTokenize,this->getFullCommand().c_str());
      char *token = strtok(stringToTokenize, WHITESPACE.c_str()); 
      while (token != NULL && arg_count < SMASH_MAX_ARGS) { //each element in args is a "word" in the command
          args[arg_count++] = token;
          token = strtok(NULL, WHITESPACE.c_str());
      }
      delete[] stringToTokenize;
      args[arg_count] = NULL; // Null-terminate the array of arguments
      execvp(args[0], args+1);
      SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, EXECV);
      
    }
    else{
      //complex command:
      setpgrp();
      char cmd_args[SMASH_MAX_PATH+1];
      strcpy(cmd_args, this->getFullCommand().c_str());
      char bash_path[SMASH_BIG_PATH+1];
      strcpy(bash_path, SMASH_BASH_PATH);
      char c_arg[SMASH_BIG_PATH+1];
      strcpy(c_arg, SMASH_C_ARG);
      char *args[] = {bash_path, c_arg, cmd_args, NULL};
      execv(SMASH_BASH_PATH, args);
      SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, EXECV);
    }
    }
    exit(0); 
  }
}
}

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line), append_mode(false){
    char* cmdl=const_cast<char*>(cmd_line);
  _removeBackgroundSign(cmdl);
  string cmd_str(cmdl);
  int index_first_redirect_sign = (int)cmd_str.find_first_of(REDIRECTION_CHAR);
  int index_last_redirect_sign = (int)cmd_str.find_last_of(REDIRECTION_CHAR);

  file_path = _trim(cmd_str.substr(index_last_redirect_sign+1));
  redirection_cmd =  _trim(cmd_str.substr(0, index_first_redirect_sign));
  //check for mode
  if(cmd_str[index_first_redirect_sign+1]==cmd_str[index_last_redirect_sign] && index_first_redirect_sign+1 <= (int)cmd_str.size()){
    append_mode=true;
  }
}

void RedirectionCommand::execute(){
  int old_fd=dup(STDOUT_FILENO);  //saving old fd of stdout
  if (old_fd==-1){
    SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR,DUP);
    return;
  }
  int new_fd=0;
    if (append_mode){
        new_fd = open(this->file_path.c_str(),O_WRONLY | O_CREAT | O_APPEND, 0777);
    }
    else{
        new_fd = open(this->file_path.c_str(),O_WRONLY | O_CREAT | O_TRUNC, 0777);
    }
  if (new_fd==-1){
     SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR,OPEN);
    return;
  }

//making 'stdout' refer to the file given in the command 
  if (dup2(new_fd,STDOUT_FILENO)==-1){
    SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR,CLOSE);
    return;
  }

  //closing the file 
  if (close(new_fd)==-1){
    SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR,CLOSE);
    return;
  }

  //command execution
    SmallShell::getInstance().executeCommand(this->redirection_cmd.c_str());
    // changing back stdout to its right position:
    if (dup2(old_fd, STDOUT_FILENO) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, DUP);
        close(old_fd);
        return;
    }

    //closing:
      if (close(old_fd) == -1){
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CLOSE);
        return;
    }
}

// chmod:

ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

bool isValidFileMode(const std::string& fileMode) {
    // Check if the file mode is either three or four characters long
    if (fileMode.empty() || ׂׂ(ׂfileMode.length() != 3 && fileMode.length() != 4)) {
        return false;
    }

    // Check if each character is between '0' and '7'
    for (char digit : fileMode) {
        if (digit < '0' || digit > '7') {
            return false;
        }
    }

    return true;
}

void ChmodCommand::execute() {
  std::string cmd = _trim(string(this->getCommandWOBg()));
  cmd = removeFirstWord(cmd);
  std::string mod_s = getFirstWord(cmd);
  std::string path = getFirstWord(removeFirstWord(cmd));

//check if it is a valid mode:
  if(!isValidFileMode(mod_s.c_str())){
    SMASH_PRINT_ERROR(SMASH_INVALID_ARGS_ERROR, CHMOD);
    return;
  }
  int mode = strtol(mod_s.c_str(), NULL, 8); // Convert permissions string to octal integer
    if (chmod(path.c_str(), mode) == -1) {
        SMASH_PRINT_WITH_PERROR(SMASH_SYSCALL_FAILED_ERROR, CHMOD);
        return;
    }
    return;
}

