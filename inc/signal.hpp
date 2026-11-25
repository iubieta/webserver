#ifndef SIGNAL_HPP
# define SIGNAL_HPP

#include <signal.h>
#include <unistd.h>
#include <cstdio>

extern volatile sig_atomic_t stop_requested;


void handle_sigint(int signal_number);
int signalHandler();

#endif