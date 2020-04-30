#include <iostream>
#include <signal.h>
#include <wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z" << endl;
    pid_t fg = smash.getJobList()->getForeground();
    if (-1 == fg){
        return;
    }
    JobsList::JobEntry* job = smash.getJobList()->getJobByPid(fg);
    job->setStatus(Stopped);
    if(-1 == kill(-1*fg, SIGSTOP)) {
        perror("smash error: kill failed");
        return;
    }
    job->setTime(time(NULL));
    smash.getJobList()->setForeground(-1);
    cout << "smash: process " << fg << " was stopped" << endl;
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;
    pid_t fg = smash.getJobList()->getForeground();
    if (fg != -1){
        if (-1 == kill(-1*fg,SIGKILL)) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << fg << " was killed" << endl;
        smash.getJobList()->setForeground(-1);
    }
}

void alarmHandler(int sig_num) {
    SmallShell& smash=SmallShell::getInstance();
    JobsList* jobs_list=smash.getJobList();
    jobs_list->removeFinishedJobs();
    TimeoutJobEntry& entry=smash.getTimeoutList()->front();
    pid_t pid=entry.getPid();
    JobsList::JobEntry* job=jobs_list->getJobByPid(pid);
    cout<<"smash: got an alarm"<<endl;
    if(job != nullptr){
        if (-1 == kill(-1*pid,SIGKILL)) {
            perror("smash error: kill failed");
            return;
        }
        cout<<"smash: "<<job->getJobCommandLine()<<" timed out!"<<endl;
    }
    smash.getTimeoutList()->pop_front();
    smash.setNewAlarm();
}