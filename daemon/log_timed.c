//<Path to buildroot>/output/host/usr/bin/arm-linux-gcc -o log_timed -uClibc -lc log_timed.c 

// ^ Compilation Command

//***************************
// ECE 531 Module 4 Daemon
//     Nathaniel Miller    
//***************************
/* This daemon writes the system time to syslog once per second.*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define true 1
#define false 0

enum error_type{OK=0, ERR_FORK, ERR_SETSID, ERR_CHDIR, ERR_NO_LOG_FILE, ERR_DISABLE_PRINT, ERR_USLEEP, ERR_TIME, ERR_SAVE_VAR, ERR_WTF};

#define DAEMON_NAME "log_timed"
#define ERROR_FORMAT "Received Errno %s"
#define ERROR_TIME_FORMAT "Received time errno %S"
#define ERROR_USLEEP_FORMAT "Received usleep errno %S"

#define LOG_FILE_NAME "/var/log/messages"

//_signal_handler()
//input - const int signal
//output - none
//Processes signal, closes syslog and exits when SIGTERM is received.
static void _signal_handler(const int signal){
  switch(signal){
    case SIGHUP: //continue
      break;
    case SIGTERM:
      syslog(LOG_INFO, "received SIGTERM, exiting.");
      closelog(); //close syslog
      exit(OK);
      break;
    default:
      syslog(LOG_INFO, "received unhandled signal"); //note unknown signal
  }
}

//_do_work()
//input - none
//output - none
//Function runs indefinitely, printing system time to syslog once per second.
static void _do_work(void){
  time_t sec, prev_sec;
  sec = time(NULL); //initialize time
  prev_sec = sec;

  syslog(LOG_INFO, "Initial Time since Epoch = %ld\n", sec);

  for(int i=0; true; i++){ //infinite loop
    if(usleep(999000) != 0){ //sleep for just under 1 second and confirm sucess
      syslog(LOG_ERR, ERROR_USLEEP_FORMAT, strerror(errno)); //log error  and exit
      exit(ERR_USLEEP);
    }

    sec = time(NULL);
    if((sec < prev_sec) || (sec > prev_sec+1)){ //check if returned time is valid
      syslog(LOG_ERR, ERROR_TIME_FORMAT, strerror(errno));  //log error and exit
      exit(ERR_TIME);
    }

    while(sec < (prev_sec+1)){ //while current time is not 1 second past previous time
      if(usleep(1000) != 0){ //sleep for 1000th of a second and confirm sucess
        syslog(LOG_ERR, ERROR_USLEEP_FORMAT, strerror(errno)); //log error  and exit
        exit(ERR_USLEEP);
      }

      sec = time(NULL);
      if((sec < prev_sec) || (sec > prev_sec+1)){//check if returned time is valid
        syslog(LOG_ERR, ERROR_TIME_FORMAT, strerror(errno));  //log error and exit
        exit(ERR_TIME);
      }

    }

    prev_sec = sec; //save current time to previous
    if(prev_sec != sec){
      syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
      exit(ERR_SAVE_VAR);
    }

    syslog(LOG_INFO, "Time = %ld\n", sec); //log current time
  }
}

int main(void){

  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON); //configure syslog information and parameters
  syslog(LOG_INFO, "Starting log_timed\n");

  pid_t pid=fork(); //fork to prevent blocking syslogd or initd

  if(pid<0) { //fork failed
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
    return ERR_FORK;
  } 

  if(pid>0){ //exit if parent
    return OK;
  }

  if(setsid() < -1){ //start new session and ensure leader
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log session error
    return ERR_SETSID;
  }

  //disable print functionality
  if((close(STDIN_FILENO) !=0) || (close(STDOUT_FILENO) !=0) || (close(STDERR_FILENO)!=0)){
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log disable print error
    return ERR_DISABLE_PRINT;
  }

  umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //set file permissions

  if(chdir("/")<0){ //set root directory
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log error setting root directory
    return ERR_CHDIR;
  }

  if(access(LOG_FILE_NAME, F_OK) != 0){ //check if log file exists
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log error and exit
    return ERR_NO_LOG_FILE;
  }

  signal(SIGTERM, _signal_handler); //send signals to _signal_handler function
  signal(SIGHUP, _signal_handler);

  _do_work(); //run main loop

  return ERR_WTF; //main loop crashed
}
