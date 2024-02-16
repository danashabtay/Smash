#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
  cout << "got ctrl-C" << endl;
  SmallShell &smash = SmallShell::getInstance();
  if (smash.current_job != nullptr){
    kill(smash.current_job->getPid(), SIGKILL);
    cout << "process " << smash.current_job->getPid() << " was killed" << endl;
    smash.current_job = nullptr;
    // is it in jobslist? if so remove.
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}
