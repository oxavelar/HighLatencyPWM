#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include "PWM.hh"


PWM *Pwm;

void _cleanup(int signum)
{
    std::cout << "\nINFO: Caught signal " << signum << std::endl;
    delete Pwm;
    exit(signum);
}


int main(int argc, char **argv)
{
    int pin;
    PWM::Period period_ns;
    PWM::Duty duty_ns;
    
    if(argc != 4) {
        std::cout << "ERROR: Usage is \"" << "pwm"
                  << " number period_ns dutycycle\"" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    /* Hardcoded arguments */
    pin = std::stoi(argv[1]);
    period_ns = std::strtoull(argv[2], NULL, 10);
    double duty = std::stod(argv[3]);
    
    /* Calculate the duty_ns based from percent */
    duty_ns = (double)period_ns * duty / (double)100;

    /* Finally make use of the library abstraction */
    Pwm = new PWM(pin, period_ns, duty_ns);
    
    /* Register a signal handler to exit gracefully and killing the PWM pin */
    signal(SIGINT, _cleanup);
    
    /* Turn on the PWM */
    Pwm->setState(PWM::State::ENABLED);

    for(;;);

    return EXIT_SUCCESS;
}

