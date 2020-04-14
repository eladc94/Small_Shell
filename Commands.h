#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <iostream>
#include <list>
#include <time.h>
using std::string;
using std::list;
using std::ostream;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

enum Status{
    Running,
    Stopped,
    Finished
};

class Command {
protected:
    const char* cmd_line;
    int pid;
public:
    Command(const char* cmd_line);
    virtual ~Command()= default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    const char* getCommandLine() const {return cmd_line;}
    int getPid() const {return pid;}
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    ~BuiltInCommand() override =default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line);
    ~ExternalCommand() override = default;
    void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    ~PipeCommand() override = default;
    void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    ~RedirectionCommand() override = default;
    void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    ~ChangeDirCommand() override = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    ~GetCurrDirCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    ~ShowPidCommand() override = default;
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    ~QuitCommand() override = default;
    void execute() override;
};

class CommandsHistory {
protected:
    class CommandHistoryEntry {
	  // TODO: Add your data members
    };
 // TODO: Add your data members
public:
    CommandsHistory();
    ~CommandsHistory()  =default;
    void addRecord(const char* cmd_line);
    void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    HistoryCommand(const char* cmd_line, CommandsHistory* history);
    ~HistoryCommand() override = default;
    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        const Command* cmd;
        const int job_id;
        const time_t start_time;
        Status status;
    public:
        JobEntry(Command* cmd,int job_id) :cmd(cmd),job_id(job_id),start_time(time(NULL)),status(Running){}

        friend bool operator==(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2);
        friend bool operator!=(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2);
        friend bool operator<(const JobEntry& je1,const JobEntry& je2);
        friend ostream& operator<<(ostream& os,const JobEntry& je);
        friend class JobsList;
    };
private:
    list<JobEntry> job_list;
    JobEntry* foreground;
public:
    JobsList()= default;
    ~JobsList()= default;
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    int getMaxJobId() const;
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    ~JobsCommand() override = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    ~KillCommand() override = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    ~ForegroundCommand() override = default;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    ~BackgroundCommand() override = default;
    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
public:
    CopyCommand(const char* cmd_line);
    ~CopyCommand() override = default;
    void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?
class ChangePromptCommand : public BuiltInCommand {
    string* new_prompt;
public:
    ChangePromptCommand(const char* cmd_line,string* prompt) :
    BuiltInCommand(cmd_line),new_prompt(prompt){}
    ~ChangePromptCommand() override = default;
    void execute() override;
};

class SmallShell {
private:
    JobsList jobs;
    string prompt;
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance(){ // make SmallShell singleton
        static SmallShell instance; // Guaranteed to be destroyed.
        return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
