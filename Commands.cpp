#include <unistd.h>
#include <string.h>
#include <iostream>
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

void GetCurrDirCommand::execute() {
    char* path = get_current_dir_name(); //change to current_dir....
    if (NULL == path){
        perror("smash error: get_current_dir_name failed");
        return;
    }
    std::cout << path << std::endl;
    free(path);
}

void ChangeDirCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    if (numOfArgs > CHDIR_MAX_ARG){
        perror("smash error: cd: too many arguments");
        for(int i=0;i<numOfArgs;i++)
            free(args[i]);
        return;
    }
    if(numOfArgs<=1) {
        for(int i=0;i<numOfArgs;i++)
            free(args[i]);
        return;
    }
    //at this points,numofargs=2
    char* curr = get_current_dir_name();
    if(NULL == curr){
        perror("smash error: get_current_dir_name failed");
        free(args[0]);
        free(args[1]);
        return;
    }
    char * dst;
    if (strcmp(args[1],CHDIR_PREV) == 0) { // if "-" was sent
        if (NULL == *plastPwd) {
            perror("smash error: cd: OLDPWD not set");
            free(curr);
            free(args[0]);
            free(args[1]);
            return;
        }
        dst = *plastPwd;
    } else
        dst=args[1];
    if (chdir(dst) != 0) {
        perror("smash error: chdir failed");
        free(curr);
        free(args[0]);
        free(args[1]);
        return;
    }
    delete[] (*plastPwd);
    *plastPwd=new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(*plastPwd, curr);
    for(int i=0;i<numOfArgs;i++)
        free(args[i]);
    free(curr);
}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}
//kill is supposed to get job id, not pid!!!
void KillCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    //if (args[1][0] != '-')
    if (numOfArgs != 3){
        perror("smash error: kill: invalid arguments");
        for (int i = 0; i < numOfArgs; ++i)
            free(args[i]);
        return;
    }
    //check format of ints
    int sig = stoi(args[1], nullptr);
    int pid = stoi(args[2], nullptr);
    sig = sig*(-1);
    if (kill(pid, sig) == -1) {
        perror("smash error: kill failed");
        for(int i=0;i<numOfArgs;i++)
            free(args[i]);
        return;
    }
    else
        std::cout << "signal number " << sig << " was sent to pid " << pid << std::endl;
    for (int i = 0; i < numOfArgs; ++i)
        free(args[i]);
}

void ForegroundCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    pid_t pid = -1;
    int job_id = -1;
    bool flag = false;
    if (numOfArgs > 2){
        perror("smash error: fg: invalid arguments");
        flag = true;
    }
    else if (numOfArgs == 2) {
        try {
            job_id = stoi(args[1], nullptr);
        }
        catch (const std::invalid_argument& ia){
            perror("smash error: fg: invalid arguments");
            for (int i = 0; i < numOfArgs; ++i) {
                free(args[i]);
            }
            return;
        }
        pid = jobs->getPidByJobID(job_id);
        if (pid == -1){
            string error = "smash error: fg: job-id " + to_string(job_id) + " does not exists";
            perror(error.c_str());
            flag = true;
        }
    }
    else {
        job_id = jobs->getMaxJobId();
        pid = jobs->getPidByJobID(job_id);
        if (pid == -1) {
            perror("smash error: fg: jobs list is empty");
            flag = true;
        }
    }
    if (flag){
        for (int i = 0; i < numOfArgs; ++i) {
            free(args[i]);
        }
        return;
    }
    if (kill(pid, SIGCONT) == -1)
            perror("smash error: kill failed");
    else{
            jobs->setForeground(pid);
            waitpid(pid, NULL, WUNTRACED);
            if (Stopped != jobs->getJobByPid(pid)->getStatus())
                jobs->removeJobById(job_id);
    }
    for(int i=0;i<numOfArgs;i++){
        free(args[i]);
    }
}

void BackgroundCommand::execute() {
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    char* args[MAX_ARGUMENTS];
    strcpy(cmd_no_ampersand,cmd_line);
    int numOfArgs=_parseCommandLine(cmd_no_ampersand,args);
    int job_id = -1;
    if(numOfArgs>2){
        for(int i=0;i<numOfArgs;i++)
            free(args[i]);
        perror("smash error: bg: invalid arguments");
        return;
    }
    else if(numOfArgs==2){
        try{
            job_id=stoi(args[1], nullptr);
        }
        catch (const std::invalid_argument& ia){
            perror("smash error: bg: invalid arguments");
            for (int i = 0; i < numOfArgs; ++i) {
                free(args[i]);
            }
            return;
        }
        bool flag=false;
        pid_t pid=jobs->getPidByJobID(job_id);
        JobsList::JobEntry *job= nullptr;
        if(pid == -1){
            string error="smash error: bg: job-id " + to_string(job_id)+" does not exist";
            perror(error.c_str());
            flag=true;
        }else {
            job = jobs->getJobById(job_id);
            if (job->getStatus() == Running) {
                string error = "smash error: bg: job-id " + to_string(job_id) + " is already running in the background";
                perror(error.c_str());
                flag=true;
            }
            else if (kill(pid, SIGCONT) == -1) {
                perror("smash error: kill failed");
                flag=true;
            }
        }
        if(flag){
            for (int i = 0; i < numOfArgs; ++i) {
                free(args[i]);
            }
            return;
        }
        job->setStatus(Running);
    }
    else{
        free(args[0]);
        JobsList::JobEntry* job = jobs->getLastStoppedJob(&job_id);
        if(nullptr == job){
            perror("smash error: bg: there is no stopped jobs to resume");
            return;
        }
        pid_t pid=jobs->getPidByJobID(job_id);
        if(kill(pid,SIGCONT) == -1){
            perror("smash error: kill failed");
            return;
        }
        job->setStatus(Running);
    }
}

void ExternalCommand::execute() {
    _removeBackgroundSign((char*)cmd_line);
    char* args[4]={(char*)"/bin/bash",(char*)"-c",(char*)cmd_line,NULL};
    int flag = execv(args[0],args);
    if (flag == -1){
        perror("smash error: execv failed");
        exit(0);
    }
}

void QuitCommand::execute() {
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    char* args[MAX_ARGUMENTS];
    int numOfArgs=_parseCommandLine(cmd_no_ampersand,args);
    bool kill = false;
    if (numOfArgs >= 2)
        for (int i = 1; i<numOfArgs; ++i)
            if (strcmp(args[i],"kill") == 0)
                kill = true;
    if(kill)
        jobs->printForQuit();
    jobs->killAllJobs();
    for(int i=0;i<numOfArgs;i++)
        free(args[i]);
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
    os << "[" << job.job_id << "] ";
    os<<job.cmd->getCommandLine()<<" : "<<job.pid<<" ";
    time_t curr_time=time(NULL);
    int elapsed=(int)(difftime(curr_time,job.start_time));
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


JobsList::JobEntry *JobsList::getJobById(int jobId){
    list<JobEntry>::iterator i;
    for(i = job_list.begin(); i != job_list.end(); ++i){
        if (i->job_id == jobId)
            return &(*i);
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){
    JobEntry* last = &(job_list.back());
    *lastJobId = last->job_id;
    return last;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId){
    list<JobEntry>::reverse_iterator i;
    for (i = job_list.rbegin(); i != job_list.rend(); ++i){
        if ((*i).status == Stopped){
            *jobId = (*i).job_id ;
            return &(*i);
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    list<JobEntry>::iterator i;
    for(i=this->job_list.begin();i!=this->job_list.end();i++)
        if(i->job_id==jobId) {
            job_list.erase(i);
            return;
        }
}

void JobsList::removeJobByPid(int pid) {
    list<JobEntry>::iterator i;
    for(i = this->job_list.begin(); i != this->job_list.end(); ++i)
        if(i->job_id == pid) {
            job_list.erase(i);
            return;
        }
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    this->removeFinishedJobs();
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
    if (job_list.empty())
        return;
    pid_t pid = waitpid((pid_t)-1,NULL,WNOHANG);
    if (pid < 0) {
        perror("smash error: waitpid failed");
        return;
    }
    list<JobEntry>::iterator i;
    while(pid > 0){
        for (i = job_list.begin(); i!=job_list.end(); ++i){
            if ((*i).pid == pid) {
                job_list.erase(i);
                break;
            }
        }
        if (job_list.empty())
            return;
        pid = waitpid(-1,NULL,WNOHANG);
        if (pid < 0) {
            perror("smash error: waitpid failed");
            return;
        }
    }
}


void JobsList::printJobsList() const{
    list<JobEntry>::const_iterator i;
    list<JobEntry> temp = job_list;
    for (i = temp.begin(); i != temp.end(); ++i){
        std::cout<<(*i)<<endl;
    }
}

void JobsList::printForQuit() const{
    cout<<"smash: sending SIGKILL signal to "<<job_list.size()<< " jobs:"<<endl;
    list<JobEntry>::const_iterator i;
    for(i=job_list.begin();i!=job_list.end();++i){
        cout<<i->pid<<": "<<i->cmd->getCommandLine()<<endl;
    }

}
void JobsList::killAllJobs() {
    list<JobEntry>::iterator i;
    for(i=job_list.begin();i!=job_list.end();++i){
        if(kill(i->pid,SIGKILL) == -1)
            perror("smash error: kill failed");
        else{
            waitpid(i->pid,NULL,0);
        }
    }
    job_list.clear();
}
void JobsList::setForeground(int fg) {
    foreground = fg;
}

int JobsList::getPidByJobID(int job_id) {
    if (nullptr == getJobById(job_id))
        return -1;
    return getJobById(job_id)->pid;
}

pid_t JobsList::getForeground() {
    return foreground;
}

JobsList::JobEntry *JobsList::getJobByPid(int pid) {
    list<JobEntry>::iterator i;
    for(i = job_list.begin(); i != job_list.end(); ++i){
        if (i->pid == pid)
            return &(*i);
    }
    return nullptr;
}


SmallShell::SmallShell() :jobs(JobsList()),prompt("smash> "),
        previous_path(nullptr) {}

SmallShell::~SmallShell() {
    jobs.killAllJobs();
    delete[] previous_path;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
//free parse!!
Command * SmallShell::CreateCommand(const char* cmd_line) {//check if & was supplied
    char* args[MAX_ARGUMENTS];
    int num_of_args = _parseCommandLine(cmd_line, args);
    string command(args[0]);
    char* cmd = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    Command* temp= nullptr;
    if ("chprompt" == command){
        temp= new ChangePromptCommand(cmd, &prompt);
    }
    else if ("showpid" == command){
        temp= new ShowPidCommand(cmd);
    }
    else if ("pwd" == command){
        temp= new GetCurrDirCommand(cmd);
    }
    else if("cd" == command){
        temp= new ChangeDirCommand(cmd, &previous_path);
    }
    else if("jobs" == command){
        temp= new JobsCommand(cmd,&jobs);
    }
    else if("kill" == command){
        temp = new KillCommand(cmd, &jobs);
    }
    else if("fg" == command){
       temp = new ForegroundCommand(cmd, &jobs);
    }
    else if("bg" == command){
        temp = new BackgroundCommand(cmd,&jobs);
    }
    else if("quit" == command){
        temp = new QuitCommand(cmd,&jobs);
    }else{
        temp = new ExternalCommand(cmd);
    }
    for(int i=0;i<num_of_args;i++)
        free(args[i]);
    return temp;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if(strcmp(cmd_line,"")==0)
        return;
    Command* cmd = CreateCommand(cmd_line);
    jobs.removeFinishedJobs();
    if (dynamic_cast<BuiltInCommand*>(cmd) != nullptr){
        cmd->execute();
        if(dynamic_cast<QuitCommand*>(cmd)!= nullptr){
            delete cmd;
            exit(0);
        }
        delete cmd;
    } else if (dynamic_cast<ExternalCommand*>(cmd) != nullptr){
          if(_isBackgroundCommand(cmd_line)){
              pid_t pid = fork();
              if (pid == 0){
                  setpgrp();
                  cmd->execute();
              } else{
              jobs.addJob(cmd, pid);
              }
          } else{
              pid_t pid = fork();
              if (pid == 0) {
                  setpgrp();
                  cmd->execute();
              } else {
                jobs.setForeground(pid);
                jobs.addJob(cmd,pid);
                int status;
                waitpid(pid, &status, WUNTRACED);
                if (!WIFSTOPPED(status))
                    jobs.removeJobById(jobs.getMaxJobId());
                jobs.setForeground(-1);
              }
          }
    }
}

JobsList *SmallShell::getJobList() {
    return &jobs;
}
