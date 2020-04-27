#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    SmallShell& smash = SmallShell::getInstance();
    struct sigaction alarm_handler;
    alarm_handler.sa_handler=alarmHandler;
    alarm_handler.sa_flags=SA_RESTART;
    if(sigaction(SIGALRM,&alarm_handler,NULL) == -1){
        perror("smash error: failed to set alarm handler");
    }
    while(true) {
        std::cout << smash.getPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}