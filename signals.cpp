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
    if(-1 == kill(fg, SIGSTOP)) {
        perror("smash error: kill failed");
        return;
    }
    smash.getJobList()->setForeground(-1);
    cout << "smash: process " << fg << " was stopped" << endl;
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;
    pid_t fg = smash.getJobList()->getForeground();
    if (fg != -1){
        if (-1 == kill(fg,SIGKILL)) {
            perror("smash error: kill failed");
            return;
        }
        if (-1 == waitpid(fg,NULL,0)){
            perror("smash error: waitpid failed");
        }
        smash.getJobList()->removeJobByPid(fg);
        cout << "smash: process " << fg << " was killed" << endl;
        smash.getJobList()->setForeground(-1);
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}