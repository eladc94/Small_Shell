#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const char* CHDIR_PREV = "-";
const int CHDIR_MAX_ARG=2;

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
#define MAX_ARGUMENTS 20
#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

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

bool _isBackgroundCommand(const char* cmd_line) {
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

/*void ExternalCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char* args[MAX_ARGUMENTS];
    _parseCommandLine(cmd_line, args);
    if(_isBackgroundCommand(cmd_line)){
        pid_t pid = fork();
        if (pid == 0)
            execv("/bin/bash", args);
        else{
            smash.getJobList()->addJob(this);
        }
    }
    else{
        pid_t pid = fork();
        if (pid == 0)
            execv("/bin/bash", args);
        else
            waitpid(pid, NULL, 0);
    }
}*/

void ShowPidCommand::execute() {
    std::cout << "smash pid is: " << getPid() << std::endl;
}

void GetCurrDirCommand::execute() {
    char* path = getcwd(NULL,0); //change to current_dir....
    if (NULL == path){
        perror("smash error: getcwd failed");
        return;
    }
    std::cout << path << std::endl;
    free(path);
}

void ChangeDirCommand::execute() {
    char* args[MAX_ARGUMENTS];
    int numOfArgs = _parseCommandLine(this->cmd_line, args);
    if (numOfArgs > CHDIR_MAX_ARG){
        perror("smash error: cd: too many arguments");
        return;
    }
    //maybe 0 arguments error - do nothing
    char* curr = getcwd(NULL, 0);
    if(NULL == curr){
        perror("smash error: getcwd failed");
    }
    char * dst;
    if (strcmp(args[1],CHDIR_PREV) == 0) { // if "-" was sent
        if (NULL == *plastPwd) {
            perror("smash error: cd: OLDPWD not set");
            free(curr);
            return;
        }
        dst = *plastPwd;
    } else
        dst=args[1];
    int flag = chdir(dst);
    if (flag != 0) {
        free(curr);
        perror("smash error: chdir failed");
        return;
    }
    strcpy(*plastPwd, curr);
    free(curr);
}

void ChangePromptCommand::execute() {
    char* args[MAX_ARGUMENTS];
    int numOfArgs = _parseCommandLine(this->cmd_line, args);
    if (numOfArgs == 0)
        (*new_prompt)="smash>";
    else {
        (*new_prompt) = args[1];
        (*new_prompt)+=">";
    }
}
bool operator==(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2){
    return je1.job_id==je2.job_id;
}

bool operator!=(const JobsList::JobEntry& je1, const JobsList::JobEntry& je2){
    return !(je1==je2);
}

bool operator<(const JobsList::JobEntry& je1,const JobsList::JobEntry& je2){
    return je1.job_id<je2.job_id;
}

ostream& operator<<(ostream& os, const JobsList::JobEntry& job){
    os<<job.cmd->getCommandLine()<<" : "<<job.pid<<" ";
    time_t curr_time=time(NULL);
    int elapsed=(int)(difftime(job.start_time,curr_time));
    os<<elapsed<<" secs";
    if(job.status==Stopped)
        os<<" (stopped)";
    return os;
}

int JobsList::getMaxJobId() const {
    list<JobEntry>::const_iterator i;
    int max = -1;
    if (this->job_list.empty())
        return 0;
    for (i = job_list.begin(); i != job_list.end(); ++i){
        if ((*i).job_id > max)
            max = (*i).job_id;
    }
    return max;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    if(!isStopped) {
        int new_job_id = this->getMaxJobId() + 1;
        JobEntry new_job = JobEntry(cmd, new_job_id, pid);
        job_list.push_back(new_job);
    }
    else{
        foreground->start_time = time(NULL);
        this->foreground->status=Stopped;
        job_list.push_back(*foreground);
        foreground=nullptr;
    }
}

void JobsList::removeJobById(int jobId) {
    list<JobEntry>::const_iterator i;
    for(i=this->job_list.begin();i!=this->job_list.end();i++)
        if(i->job_id==jobId) {
            job_list.erase(i);
            return;
        }
}

void JobsList::printJobsList() {
    list<JobEntry>::const_iterator i;
    list<JobEntry> temp = job_list;
    temp.sort();
    for (i = temp.begin(); i != temp.end(); ++i){
        std::cout<<(*i)<<endl;
    }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){
    JobEntry* last = &(job_list.back());
    *lastJobId = last->job_id;
    return last;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
    list<JobEntry>::reverse_iterator i;
    for (i = job_list.rbegin(); i != job_list.rend(); ++i){
        if ((*i).status == Stopped){
            *jobId = (*i).job_id;
            return &(*i);
        }
    }
    return nullptr;
}

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

void SmallShell::setPrompt(const char *prompt) {
    this->prompt = prompt;
    this->prompt += ">";
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {//check if & was supplied
    char* args[MAX_ARGUMENTS];
    char cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    string command(args[0]);
    int num_of_args = _parseCommandLine(cmd_line, args);
    if ("chprompt" == command){
        _removeBackgroundSign(cmd);
        return new ChangePromptCommand(cmd, &prompt);
    }
    else if ("showpid" == command){
        _removeBackgroundSign(cmd);
        return new ShowPidCommand(cmd);
    }
    else if ("pwd" == command){
        _removeBackgroundSign(cmd);
        return new GetCurrDirCommand(cmd);
    }
    else if("cd" == command){
        _removeBackgroundSign(cmd);
        return new ChangeDirCommand(cmd, &previous_path);
    }
    else if("jobs" == command){
        _removeBackgroundSign(cmd);
        //return new JobsCommand
    }
    else if("kill" == command){
        _removeBackgroundSign(cmd);
        //return new KillCommand
    }
    else if("fg" == command){
        _removeBackgroundSign(cmd);
        //return new FG
    }
    else if("bg" == command){
        _removeBackgroundSign(cmd);
        //return new BG
    }
    else if("quit" == command){
        _removeBackgroundSign(cmd);
        //return new q
    }
    else{
        return new ExternalCommand(cmd_line);
    }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  Command* cmd = CreateCommand(cmd_line);
  if (dynamic_cast<BuiltInCommand*>(cmd) != nullptr){
      cmd->execute();
      delete cmd;
  }
  else if (dynamic_cast<ExternalCommand*>(cmd) != nullptr){
      char* args[MAX_ARGUMENTS];
      _parseCommandLine(cmd_line, args);
      if(_isBackgroundCommand(cmd_line)){
          pid_t pid = fork();
          if (pid == 0)
              cmd->execute();
          else{
              jobs.addJob(cmd, pid);
          }
      }
      else{
          pid_t pid = fork();
          if (pid == 0)
              cmd->execute();
          else {
              waitpid(pid, NULL, 0);
              delete cmd;
          }
      }
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}