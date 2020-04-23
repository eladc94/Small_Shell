#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "signals.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const char* CHDIR_PREV = "-";
const int CHDIR_MAX_ARG = 2;
const int MIN_SIG_NUM = 1;
const int MAX_SIG_NUM = 31;
const mode_t OPEN_MODE = 0666;

const int NO_ARROW = -1;
const int DOES_NOT_EXIST = -1;
const pid_t PID_NOT_EXIST = -1;
const int SYSCALL_ERROR = -1;
const int EXECV_ARRAY_SIZE = 4;
const int KILL_ARGS = 3;
const int BFG_ARGS = 2;
const int STDOUT_FD = 1;
const int STDERR_FD = 2;
const int READ_SIZE=1000;

const int KILL_SIG_ARG = 1;
const int KILL_JOB_ARG = 2;
const int BFG_JOB_ARG = 1;
const int COPY_SRC_ARG = 1;
const int COPY_DST_ARG = 2;


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
#define FREE_PARSE() \
for (int i=0;i<numOfArgs;i++) \
    free(args[i])
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

bool _isRedirectionOrPipe(const char* cmd_line, int* arrow_pos, char sign){
   int len=strlen(cmd_line);
   for(int i=0;i<len;i++){
       if(cmd_line[i] == sign){
           *arrow_pos = i;
           return true;
       }
   }
   return false;
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

int sendToBash(char* cmd_line){
    char* args[EXECV_ARRAY_SIZE]={(char*)"/bin/bash",(char*)"-c",cmd_line,NULL};
    return execv(args[0],args);
}

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
    FREE_PARSE();
}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash pid is: " << smash.getPid() << std::endl;
}

void GetCurrDirCommand::execute() {
    char* path = get_current_dir_name(); //change to current_dir....
    if (NULL == path){
        cerr<<"smash error: get_current_dir_name failed"<<endl;
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
       cerr<<"smash error: cd: too many arguments"<<endl;
        FREE_PARSE();
        return;
    }
    if(numOfArgs<=1) {
        FREE_PARSE();
        return;
    }
    //at this points,numofargs=2
    char* curr = get_current_dir_name();
    if(NULL == curr){
        cerr<<"smash error: get_current_dir_name failed"<<endl;
        FREE_PARSE();
        return;
    }
    char * dst;
    if (strcmp(args[1],CHDIR_PREV) == 0) { // if "-" was sent
        if (NULL == *plastPwd) {
            cerr<<"smash error: cd: OLDPWD not set"<<endl;
            free(curr);
            FREE_PARSE();
            return;
        }
        dst = *plastPwd;
    } else
        dst=args[1];
    if (chdir(dst) != 0) {
        perror("smash error: chdir failed");
        free(curr);
        FREE_PARSE();
        return;
    }
    delete[] (*plastPwd);
    *plastPwd=new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(*plastPwd, curr);
    FREE_PARSE();
    free(curr);
}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}

void KillCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    if (numOfArgs != KILL_ARGS){
        cerr<<"smash error: kill: invalid arguments"<<endl;
        FREE_PARSE();
        return;
    }
        int sig=0,job_id=DOES_NOT_EXIST;
    try {
        sig = stoi(args[KILL_SIG_ARG], nullptr);
        job_id = stoi(args[KILL_JOB_ARG], nullptr);
    }
    catch(const std::invalid_argument& ie){
        cerr<<"smash error: kill: invalid arguments"<<endl;
        FREE_PARSE();
        return;
    }
    sig = sig*(-1);
    if(sig<MIN_SIG_NUM||sig>MAX_SIG_NUM){
        cerr<<"smash error: kill: invalid arguments"<<endl;
        FREE_PARSE();
        return;
    }
    pid_t pid = jobs->getPidByJobID(job_id);
    if(PID_NOT_EXIST == pid){
        cerr<<"smash error: kill: job-id "<<job_id<<" does not exist"<<endl;
        FREE_PARSE();
        return;
    }
    if (kill(pid, sig) == SYSCALL_ERROR) {
        perror("smash error: kill failed");
        FREE_PARSE();
        return;
    }
    else
        std::cout << "signal number " << sig << " was sent to pid " << pid << std::endl;
    FREE_PARSE();
}

void ForegroundCommand::execute() {
    char* args[MAX_ARGUMENTS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int numOfArgs = _parseCommandLine(cmd_no_ampersand, args);
    pid_t pid = PID_NOT_EXIST;
    int job_id = DOES_NOT_EXIST;
    bool flag = false;
    if (numOfArgs > BFG_ARGS){
        cerr<<"smash error: fg: invalid arguments"<<endl;
        flag = true;
    }
    else if (numOfArgs == BFG_ARGS) {
        try {
            job_id = stoi(args[BFG_JOB_ARG], nullptr);
        }
        catch (const std::invalid_argument& ia){
            cerr<<"smash error: fg: invalid arguments"<<endl;
            FREE_PARSE();
            return;
        }
        pid = jobs->getPidByJobID(job_id);
        if (pid == PID_NOT_EXIST){
            cerr<<"smash error: fg: job-id "<<job_id<< " does not exists"<<endl;
            flag = true;
        }
    }
    else {
        job_id = jobs->getMaxJobId();
        pid = jobs->getPidByJobID(job_id);
        if (pid == PID_NOT_EXIST) {
            cerr<<"smash error: fg: jobs list is empty"<<endl;
            flag = true;
        }
    }
    if (flag){
        FREE_PARSE();
        return;
    }
    if (kill(-1*pid, SIGCONT) == SYSCALL_ERROR) {
        perror("smash error: kill failed");
        FREE_PARSE();
        return;
    }
    else{
            jobs->setForeground(pid);
            int status;
            if (SYSCALL_ERROR == waitpid(pid, &status, WUNTRACED)) {
                perror("smash error: waitpid failed");
                FREE_PARSE();
                return;
            }
            if (!WIFSTOPPED(status))
                jobs->removeJobById(job_id);
            jobs->setForeground(-1);
    }
    FREE_PARSE();
}

void BackgroundCommand::execute() {
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    char* args[MAX_ARGUMENTS];
    strcpy(cmd_no_ampersand,cmd_line);
    int numOfArgs=_parseCommandLine(cmd_no_ampersand,args);
    int job_id = DOES_NOT_EXIST;
    if(numOfArgs > BFG_ARGS){
        FREE_PARSE();
        cerr<<"smash error: bg: invalid arguments"<<endl;
        return;
    }
    else if(numOfArgs == BFG_ARGS){
        try{
            job_id=stoi(args[BFG_JOB_ARG], nullptr);
        }
        catch (const std::invalid_argument& ia){
            cerr<<"smash error: bg: invalid arguments"<<endl;
            FREE_PARSE();
            return;
        }
        bool flag=false;
        pid_t pid=jobs->getPidByJobID(job_id);
        JobsList::JobEntry *job= nullptr;
        if(pid == PID_NOT_EXIST){
            cerr <<"smash error: bg: job-id "<<job_id<<" does not exist"<< endl;
            flag=true;
        }
        else {
            job = jobs->getJobById(job_id);
            if (job->getStatus() == Running) {
                cerr<<"smash error: bg: job-id " <<job_id << " is already running in the background" << endl;
                flag=true;
            }
            else if (kill(-1*pid, SIGCONT) == SYSCALL_ERROR) {
                perror("smash error: kill failed");
                flag=true;
            }
        }
        if(flag){
            FREE_PARSE();
            return;
        }
        job->setStatus(Running);
    }
    else{
        JobsList::JobEntry* job = jobs->getLastStoppedJob(&job_id);
        if(nullptr == job){
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        pid_t pid=jobs->getPidByJobID(job_id);
        if(kill(-1*pid,SIGCONT) == SYSCALL_ERROR){
            perror("smash error: kill failed");
            return;
        }
        job->setStatus(Running);
    }
    FREE_PARSE();
}

void ExternalCommand::execute() {
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    int flag = sendToBash(cmd_no_ampersand);
    if (flag == SYSCALL_ERROR){
        perror("smash error: execv failed");
        exit(0);
    }
}

void QuitCommand::execute() {
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    _removeBackgroundSign(cmd_no_ampersand);
    char* args[MAX_ARGUMENTS];
    int numOfArgs=_parseCommandLine(cmd_no_ampersand,args);
    bool kill = false;
    if (numOfArgs >= BFG_ARGS)
        for (int i = 1; i<numOfArgs; ++i)
            if (strcmp(args[i],"kill") == 0)
                kill = true;
    if(kill)
        jobs->printForQuit();
    jobs->killAllJobs();
    FREE_PARSE();
}

RedirectionCommand::RedirectionCommand(const char* cmd_line, int arrow_pos): Command(cmd_line), file_name(""),
internal_cmd_line(""),single_arrow(true), background(false){
    char cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    if (_isBackgroundCommand(cmd))
        background = true;
    _removeBackgroundSign(cmd);
    char* args[COMMAND_MAX_ARGS];
    int numOfArgs = _parseCommandLine(cmd, args);
    for(int i=0;i<arrow_pos;i++)
        internal_cmd_line.push_back(cmd[i]);
    int len=strlen(cmd);
    if(arrow_pos<len-1)
        if(cmd[arrow_pos+1]=='>'){
            single_arrow = false;
            arrow_pos++;
        }
    for(int i=arrow_pos+1;i<len;i++)
        file_name.push_back(cmd[i]);
    char* file_name_args[COMMAND_MAX_ARGS];
    int numOfFileArgs = _parseCommandLine(file_name.c_str(),file_name_args);
    if(numOfFileArgs == 0)
        file_name="";
    else
        file_name=file_name_args[0];
    for(int i =0;i<numOfFileArgs;i++){
        free(file_name_args[i]);
    }
    FREE_PARSE();
}

void RedirectionCommand::execute() {
    SmallShell& smash=SmallShell::getInstance();
    char internal_copy[COMMAND_ARGS_MAX_LENGTH];
    strcpy(internal_copy, internal_cmd_line.c_str());
    Command *internal_cmd = smash.CreateCommand(internal_copy);
    int new_fd=dup(STDOUT_FD);
    close(STDOUT_FD);
    int fd;
    if (single_arrow)
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,OPEN_MODE);
    else
        fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_APPEND,OPEN_MODE);
    if(dynamic_cast<BuiltInCommand*>(internal_cmd) != nullptr){
        internal_cmd->execute();
        delete internal_cmd;
        close(fd);
        dup2(new_fd,STDOUT_FD);
        close(new_fd);
    }
    else {
        delete internal_cmd;
        int flag=sendToBash(internal_copy);
        if(flag == SYSCALL_ERROR){
            perror("smash error: execv failed");
            exit(0);
        }
    }
}
//TODO: maybe move the "copied" text
void CopyCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    char cmd_no_ampersand[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_no_ampersand,cmd_line);
    int numOfArgs=_parseCommandLine(cmd_no_ampersand,args);
    string src_name=args[COPY_SRC_ARG];
    string dst_name=args[COPY_DST_ARG];
    if(src_name==dst_name)
        return;
    int fd_read=open(src_name.c_str(), O_RDONLY);
    int fd_write=open(dst_name.c_str(), O_WRONLY|O_CREAT|O_TRUNC,OPEN_MODE);
    if( fd_read == SYSCALL_ERROR || fd_write == SYSCALL_ERROR){
        perror("smash error: open failed");
        FREE_PARSE();
        return;
    }
    char read_string[READ_SIZE];
    bool read_error=false;
    int read_bits=read(fd_read,read_string,READ_SIZE);
    if(read_bits == SYSCALL_ERROR)
       read_error=true;
    while(read_bits>0){
        int write_bits=write(fd_write,read_string,read_bits);
        if(write_bits == SYSCALL_ERROR){
            perror("smash error: write failed");
            FREE_PARSE();
            return;
        }
        read_bits=read(fd_read,read_string,READ_SIZE);
        if(read_bits == SYSCALL_ERROR)
            read_error=true;
    }
    if(read_error){
        perror("smash error: read failed");
        FREE_PARSE();
        return;
    }
    cout<<"smash: "<<src_name<<" was copied to "<<dst_name<<endl;
    int flag1=close(fd_write);
    int flag2=close(fd_read);
   if(flag1 == SYSCALL_ERROR || flag2==SYSCALL_ERROR) {
       perror("smash error: close failed");
    }
    FREE_PARSE();
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
    int max = DOES_NOT_EXIST;
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

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    this->removeFinishedJobs();
    if(!isStopped) {
        int new_job_id = this->getMaxJobId() + 1;
        JobEntry new_job = JobEntry(cmd, new_job_id, pid);
        job_list.push_back(new_job);
    }
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
        pid = waitpid((pid_t)-1,NULL,WNOHANG);
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
        if(kill(-1*(i->pid),SIGKILL) == SYSCALL_ERROR)
            perror("smash error: kill failed");
        else{
            if (SYSCALL_ERROR == waitpid(i->pid,NULL,0))
                perror("smash error: waitpid failed");
        }
    }
    job_list.clear();
}

int JobsList::getPidByJobID(int job_id) {
    if (nullptr == getJobById(job_id))
        return PID_NOT_EXIST;
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


SmallShell::SmallShell() : jobs(JobsList()),smash_pid(getpid()),prompt("smash> "),
        previous_path(nullptr) {}

SmallShell::~SmallShell() {
    if(getpid() == smash_pid)
        jobs.killAllJobs();
    delete[] previous_path;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line){
    char* args[MAX_ARGUMENTS];
    int numOfArgs = _parseCommandLine(cmd_line, args);
    string command(args[0]);
    char* cmd = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    Command* temp= nullptr;
    int sign_pos=NO_ARROW;
    if (_isRedirectionOrPipe(cmd_line, &sign_pos, '>')){
            temp = new RedirectionCommand(cmd, sign_pos);
    }
    else if (_isRedirectionOrPipe(cmd_line,&sign_pos,'|')) {
        temp = new PipeCommand(cmd, sign_pos);
    }
    else if ("chprompt" == command){
        temp = new ChangePromptCommand(cmd, &prompt);
    }
    else if ("showpid" == command){
        temp = new ShowPidCommand(cmd);
    }
    else if ("pwd" == command){
        temp = new GetCurrDirCommand(cmd);
    }
    else if("cd" == command){
        temp = new ChangeDirCommand(cmd, &previous_path);
    }
    else if("jobs" == command){
        temp = new JobsCommand(cmd,&jobs);
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
    }
    else if("cp" == command){
        temp = new CopyCommand(cmd);
    }
    else{
        temp = new ExternalCommand(cmd);
    }
    FREE_PARSE();
    return temp;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if (strcmp(cmd_line, "") == 0)
        return;
    Command *cmd = CreateCommand(cmd_line);
    jobs.removeFinishedJobs();
    if (dynamic_cast<BuiltInCommand *>(cmd) != nullptr) {
        cmd->execute();
        if (dynamic_cast<QuitCommand *>(cmd) != nullptr) {
            delete cmd;
            exit(0);
        }
        delete cmd;
    } else if (dynamic_cast<ExternalCommand*>(cmd) != nullptr) {
            pid_t pid = fork();
            if (pid == 0) {
                setpgrp();
                cmd->execute();
                delete cmd;
                exit(0);
        } else {
                if(_isBackgroundCommand(cmd_line))
                    jobs.addJob(cmd,pid);
                else{
                    jobs.setForeground(pid);
                    jobs.addJob(cmd, pid);
                    int status;
                    if (SYSCALL_ERROR == waitpid(pid, &status, WUNTRACED)) {
                        perror("smash error: waitpid failed");
                    }
                    if (!WIFSTOPPED(status))
                        jobs.removeJobById(jobs.getMaxJobId());
                    jobs.setForeground(-1);
                }
        }
    }
    else if (dynamic_cast<RedirectionCommand *>(cmd) != nullptr) {
        auto temp = dynamic_cast<RedirectionCommand*>(cmd);
        string internal_cmd_line = temp->getInternal();
        Command *internal_cmd = CreateCommand(internal_cmd_line.c_str());
        if (dynamic_cast<BuiltInCommand *>(internal_cmd) != nullptr) {
            cmd->execute();
        }
        else {
            pid_t pid = fork();
            if (pid == 0) {
                setpgrp();
                cmd->execute();
                delete cmd;
                delete internal_cmd;
                exit(0);
            }
            else {
                bool background = _isBackgroundCommand(cmd_line);
                if (background) {
                    jobs.addJob(cmd, pid);
                }
                else {
                    jobs.setForeground(pid);
                    jobs.addJob(cmd, pid);
                    int status;
                    if (SYSCALL_ERROR == waitpid(pid, &status, WUNTRACED)) {
                        perror("smash error: waitpid failed");
                    }
                    if (!WIFSTOPPED(status))
                        jobs.removeJobById(jobs.getMaxJobId());
                    jobs.setForeground(-1);
                }
            }
        }
        delete internal_cmd;
    }
    else if (dynamic_cast<PipeCommand*>(cmd) != nullptr){
        pid_t pid = fork();
        if (-1 == pid){
            perror("smash error: fork failed");
            return;
        }
        else if (0 == pid){
            setpgrp();
            cmd->execute();
            delete cmd;
            exit(0);
        }
        else{
            bool background = _isBackgroundCommand(cmd_line);
            if (background) {
                jobs.addJob(cmd, pid);
            }
            else {
                jobs.setForeground(pid);
                jobs.setFgPipe(true);
                jobs.addJob(cmd, pid);
                int status;
                if (SYSCALL_ERROR == waitpid(pid, &status, WUNTRACED)) {
                    perror("smash error: waitpid failed");
                }
                if (!WIFSTOPPED(status))
                    jobs.removeJobById(jobs.getMaxJobId());
                jobs.setForeground(-1);
                jobs.setFgPipe(false);
            }
        }
    }
}
JobsList *SmallShell::getJobList() {
    return &jobs;
}

PipeCommand::PipeCommand(const char *cmd_line, int sign_pos) : Command(cmd_line), cmd1(""), cmd2(""),
ampersend(false) {
    char cmd[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd, cmd_line);
    _removeBackgroundSign(cmd);
    char* args[COMMAND_MAX_ARGS];
    int numOfArgs = _parseCommandLine(cmd, args);
    for(int i=0;i<sign_pos ;i++)
        cmd1.push_back(cmd[i]);
    int len=strlen(cmd);
    if(sign_pos<len-1)
        if(cmd[sign_pos+1]=='&'){
            ampersend = true;
            sign_pos++;
        }
    for(int i=sign_pos+1;i<len;i++)
        cmd2.push_back(cmd[i]);
    FREE_PARSE();
}

void PipeCommand::execute() {
    int my_pipe[2];
    pipe(my_pipe);
    pid_t left = fork();
    SmallShell& smash = SmallShell::getInstance();
    if (-1 == left){
        perror("smash error: fork failed");
        return;
    }
    else if (0 == left){
        int to_close = STDOUT_FD;
        if (ampersend)
            to_close = STDERR_FD;
        close(my_pipe[0]);
        close(to_close);
        dup2(my_pipe[1],to_close);
        Command* left_cmd = smash.CreateCommand(cmd1.c_str());
        left_cmd->execute();
        delete left_cmd;
        exit(0);
    }
    else {
        close(my_pipe[1]);
        pid_t right = fork();
        if (-1 == right) {
            perror("smash error: fork failed");
            return;
        }
        else if (0 == right) {
            close(0);
            dup2(my_pipe[0], 0);
            Command *right_cmd = smash.CreateCommand(cmd2.c_str());
            right_cmd->execute();
            delete right_cmd;
            exit(0);
        }
        else{
            close(my_pipe[0]);
            int left_stat, right_stat;
            do {
                if (SYSCALL_ERROR == waitpid(left, &left_stat, WUNTRACED))
                    perror("smash error: waitpid failed");
            }while (WIFSTOPPED(left_stat));
            do {
                if (SYSCALL_ERROR == waitpid(right, &right_stat, WUNTRACED))
                    perror("smash error: waitpid failed");
            }while (WIFSTOPPED(right_stat));
            return;
        }
    }
}
