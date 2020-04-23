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
    if (smash.getJobList()->getFgPipe()){
        if(-1 == kill(fg, SIGTSTP)) {
            perror("smash error: kill failed");
            return;
        }
    }
    else {
        if (-1 == kill(fg, SIGSTOP)) {
            perror("smash error: kill failed");
            return;
        }
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
        if (smash.getJobList()->getFgPipe()){
            if(-1 == kill(fg, SIGINT)) {
                perror("smash error: kill failed");
                return;
            }
        }
        else if (-1 == kill(fg,SIGKILL)) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << fg << " was killed" << endl;
        smash.getJobList()->setForeground(-1);
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

void ctrlCPipeHandler(int sig_num) {
    if (-1 == killpg(0, SIGKILL)){
        perror("smash error: killpg failed");
    }
}

void ctrlZPipeHandler(int sig_num) {
    if (-1 == killpg(0, SIGSTOP))
        perror("smash error: killpg failed");
}

void sigcontPipeHandler(int sig_num) {
    if (-1 == killpg(0, SIGCONT))
        perror("smash error: killpg failed");
}
