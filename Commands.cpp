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

void ChangePromptCommand::execute() {
    char* args[MAX_ARGUMENTS];
    int numOfArgs = _parseCommandLine(this->cmd_line, args);
    if (numOfArgs == 0)
        (*new_prompt)="smash>";
    else {
        (*new_prompt) = args[0];
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
    os<<job.cmd->getCommandLine()<<" : "<<job.cmd->getPid()<<" ";
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
        return 1;
    for (i = job_list.begin(); i != job_list.end(); ++i){
        if ((*i).job_id > max)
            max = (*i).job_id;
    }
    return max;
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    if(!isStopped) {
        int new_job_id = this->getMaxJobId() + 1;
        JobEntry new_job = JobEntry(cmd, new_job_id);
        job_list.push_back(new_job);
    }
    else{
        this->foreground->status=Stopped;
        job_list.push_back(*foreground);
        foreground=nullptr;
    }
}

void JobsList::removeJobById(int jobId) {
    list<JobEntry>::const_iterator i;
    for(i=this->job_list.begin();i!=this->job_list.end();i++)
        if(i->job_id==jobId)
            job_list.erase(i);
}

void JobsList::printJobsList() {
    list<JobEntry>::const_iterator i;
    list<JobEntry> temp = job_list;
    temp.sort();
    for (i = temp.begin(); i != temp.end(); ++i){
        std::cout<<(*i)<<"\n";
    }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){
    JobEntry last = job_list.back();
    *lastJobId = last.job_id;
    return &last;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
    list<JobEntry>::iterator i;
    for (i = job_list.end(); i != job_list.begin(); --i){
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
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}