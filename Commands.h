#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <memory>
#include <iostream>
#include <list>
#include <time.h>
using std::string;
using std::shared_ptr;
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
public:
    explicit Command(const char* cmd_line) : cmd_line(cmd_line){}
    virtual ~Command(){delete[] cmd_line;}
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    const char* getCommandLine() const {return cmd_line;}
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char* cmd_line): Command(cmd_line){}
    ~BuiltInCommand() override =default;
};

class ExternalCommand : public Command {
public:
    explicit ExternalCommand(const char* cmd_line): Command(cmd_line){}
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
    char** plastPwd;
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), plastPwd(plastPwd){}
    ~ChangeDirCommand() override = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    ~GetCurrDirCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
   explicit ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    ~ShowPidCommand() override = default;
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobs(jobs){}
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
        const shared_ptr<Command> cmd;
        const int job_id;
        const pid_t pid;
        time_t start_time;
        Status status;
    public:
        JobEntry(Command* cmd,int job_id, pid_t pid) :cmd(cmd),job_id(job_id),pid(pid)
        ,start_time(time(NULL)),status(Running){}
        JobEntry(const JobEntry& je)= default;
        ~JobEntry() = default;
        Status getStatus() const {return status;}
        void setTime(time_t time){start_time=time;}
        void setStatus(Status st){status=st;}


        friend bool operator==(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2);
        friend bool operator!=(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2);
        friend bool operator<(const JobEntry& je1,const JobEntry& je2);
        friend ostream& operator<<(ostream& os,const JobEntry& je);

        friend class JobsList;
    };
private:
    list<JobEntry> job_list;
    pid_t foreground;
public:
    JobsList() : job_list(list<JobEntry>()), foreground(-1){}
    ~JobsList()= default;
    void addJob(Command* cmd, pid_t pid, bool isStopped = false);
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    JobEntry * getJobByPid(int pid);
    pid_t getPidByJobID(int job_id);
    JobEntry *getLastStoppedJob(int *jobId);
    JobEntry * getLastJob(int* lastJobId);
    void removeJobById(int jobId);
    void removeJobByPid(int pid);
    int getMaxJobId() const;
    void setForeground(int fg);
    void printJobsList() const;
    void printForQuit() const;
    void killAllJobs();
    pid_t getForeground();
};

class JobsCommand : public BuiltInCommand {
 JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs) :BuiltInCommand(cmd_line),jobs(jobs) {}
    ~JobsCommand() override = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
 JobsList* jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    ~KillCommand() override = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){}
    ~ForegroundCommand() override = default;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList* jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),jobs(jobs){}
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
    char* previous_path;
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
  string getPrompt() const {return prompt;}
  JobsList* getJobList();
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
