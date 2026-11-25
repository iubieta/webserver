#include "../inc/signal.hpp"

volatile sig_atomic_t stop_requested = 0;

void handle_sigint(int signal_number)
{
	if(SIGINT == signal_number)
		stop_requested = 1;
}



int signalHandler()
{
	struct sigaction action = {};

    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

	if (sigaction(SIGINT, &action, NULL) == -1)
    {
        std::perror("sigaction");
        return -1;
    }
	return 0;	
}
