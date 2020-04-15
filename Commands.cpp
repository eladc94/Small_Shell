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

void ChangePromptCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    if (numOfArgs == 1)
        (*new_prompt)="smash> ";
    else {
        (*new_prompt) = args[1];
        (*new_prompt)+="> ";
    }
    for(int i=0;i<numOfArgs;i++)
        free(args[i]);
}

void ShowPidCommand::execute() {
    std::cout << "smash pid is: " << getpid() << std::endl;
}
//change syscall
void GetCurrDirCommand::execute() {
    char* path = getcwd(NULL,0); //change to current_dir....
    if (NULL == path){
        perror("smash error: getcwd failed");
        return;
    }
    std::cout << path << std::endl;
    free(path);
}
//free from parse!!!
void ChangeDirCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    if (numOfArgs > CHDIR_MAX_ARG){
        perror("smash error: cd: too many arguments");
        return;
    }
    if(numOfArgs==1)
        return;
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
    delete[] (*plastPwd);
    *plastPwd=new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(*plastPwd, curr);
    free(curr);
}

void JobsCommand::execute() {

}

void ExternalCommand::execute() {
    char* args[4]={(char*)"/bin/bash",(char*)"-c",(char*)cmd_line,NULL};
    execv(args[0],args);
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
    //stopping
    /*else{

    }*/
}

void JobsList::removeFinishedJobs() {
    pid_t pid=waitpid(-1,NULL,WNOHANG)!=0;
    while(pid!=0){

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

SmallShell::SmallShell() :jobs(JobsList()),prompt("smash> "),
        previous_path(nullptr),foreground(nullptr) {}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
//free parse!!
Command * SmallShell::CreateCommand(const char* cmd_line) {//check if & was supplied
    char* args[MAX_ARGUMENTS];
    int num_of_args = _parseCommandLine(cmd_line, args);
    string command(args[0]);
    Command* temp;
    if ("chprompt" == command){
        temp= new ChangePromptCommand(cmd_line, &prompt);
    }
    else if ("showpid" == command){
        temp= new ShowPidCommand(cmd_line);
    }
    else if ("pwd" == command){
        temp= new GetCurrDirCommand(cmd_line);
    }
    else if("cd" == command){
        temp= new ChangeDirCommand(cmd_line, &previous_path);
    }
    else if("jobs" == command){
        temp= new JobsCommand(cmd_line,&jobs);
    }
    else if("kill" == command){
        //return new KillCommand
    }
    else if("fg" == command){
        //return new FG
    }
    else if("bg" == command){
        //return new BG
    }
    else if("quit" == command){
        //return new q
    }else{
        temp = new ExternalCommand(cmd_line);
    }
    for(int i=0;i<num_of_args;i++)
        free(args[i]);
    return temp;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  Command* cmd = CreateCommand(cmd_line);
  if (dynamic_cast<BuiltInCommand*>(cmd) != nullptr){
      cmd->execute();
      delete cmd;
  }
  else if (dynamic_cast<ExternalCommand*>(cmd) != nullptr){
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
          int new_job_id=jobs.getMaxJobId()+1;

          if (pid == 0)
              cmd->execute();
          else {
              foreground=new JobsList::JobEntry(cmd,new_job_id,pid);
              waitpid(pid, NULL, 0);
              delete foreground;
              foreground= nullptr;
          }
      }
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}