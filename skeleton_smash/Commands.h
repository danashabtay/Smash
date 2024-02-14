#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <memory>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
const char* full_command;
const char* command_without_bg;
bool is_bg_coomand;
bool is_Timed;
int duration;
 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  bool isBgCommand();
  std::string getFullCommand();
  std::string getCommandWOBg();
  bool getIsTimed();
  int getDuration();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members 
  public:
  bool isValidNumArg;
  std::string dir;
  ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
private:
  JobsList* jobs_list;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsList {
 public:

  class JobEntry {
   // TODO: Add your data members
  private:
  int job_id;
  pid_t pid;
  bool is_stooped_by_user;
  bool is_timed;
  time_t insert_time;
  int duration;
  std::string command;
  public:
  JobEntry(int job_id, pid_t pid, bool is_stooped_by_user, bool is_timed, int duration, std::string command);
  JobEntry(const JobEntry &other);
  JobEntry& operator=(const JobEntry &other);
  ~JobEntry();
  int getJobId();
  pid_t getPid();
  bool isStooped();
  bool isTimed();
  time_t getInsertTime();
  std::string getCommand();
  int getDuration();
  void setAsResumed();
  void setAsStopped();
  bool operator>(const JobEntry& other);
  bool operator<(const JobEntry& other);
  bool operator==(const JobEntry& other);
  bool operator!=(const JobEntry& other);
  };

 private:
    std::vector<std::shared_ptr<JobEntry>> jobsList;
 // TODO: Add your data members
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, const pid_t& pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int getMaxJobId();
  // TODO: Add extra methods or modify exisitng ones as needed

};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  std::string prevCommand;
  std::string currPrompt;
  std::string prevDir;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // newly added:
  std::string getCurrPrompt();
  void changePrevCommand(const std::string cmd);
  void changePrompt(std::string prompt);
  std::string getPrevDir();
  void changePrevDir(std::string Dir);
  // TODO: add extra methods as needed
};


/// BUILT-IN COMMANDS: ///

class ChPromptCommand : public BuiltInCommand {
  std::string prompt;
 public:
  ChPromptCommand(const char* cmd_line);
  virtual ~ChPromptCommand() {}
  void execute() override;
};

#endif //SMASH_COMMAND_H_
