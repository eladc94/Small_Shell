#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

void ctrlZHandler(int sig_num);
void ctrlCHandler(int sig_num);
void alarmHandler(int sig_num);
void ctrlCPipeHandler(int sig_num);
void ctrlZPipeHandler(int sig_num);
void sigcontPipeHandler(int sig_num);

#endif //SMASH__SIGNALS_H_
