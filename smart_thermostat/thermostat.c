//<Path to buildroot>/output/host/usr/bin/arm-linux-gcc -o thermostat -lcurl -uClibc -lc -ld thermostat.c 
// ^ Compilation Command

//***************************
// ECE 531 Thermostat Daemon
//       Final Project
//     Nathaniel Miller    
//***************************
/* This thermostat reads temperature from a thermocouplm, reads/writes Command & Control data from a server, and turns on/off a heater system.  For more details on how to run, use ./thermostat -h.*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <curl/curl.h>
#include <math.h>

#define true 1
#define false 0

#define DAEMON_NAME "thermostat"
#define ERROR_FORMAT "Received Errno %s"

#define DEBUG 1
#undef DEBUG //comment to turn on debugging print statements

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif //Debug macro from Allen Holub's book "Enough Rope to Shoot Yourself in the Foot: Rules for C and C++ Programming"

#define THERMOCOUPLE_FILE "/tmp/temp" //predefined
#define HEATER_POWER_FILE "/tmp/status"
#define CONFIG_SLEEP 2 //time between reading data and processing

enum error_type{OK=0, ERR_FORK, ERR_SETSID, ERR_CHDIR, ERR_NO_LOG_FILE, ERR_DISABLE_PRINT, ERR_USLEEP, ERR_TIME, ERR_SAVE_VAR, INIT_ERR, HTTP_ERR, INPUT_ERR, ERR_WTF};
enum DayEnum{Monday=1, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday};

struct MemoryStruct {
  char *memory;
  size_t size;
};
struct MemoryStruct chunk;

struct StatusStruct {
  int id;
  unsigned long time_last_programmed;
  unsigned long time_last_update;
  int curr_temp;
  int set_temp;
  int power;
  int new_temp;
};
struct StatusStruct status;
struct StatusStruct thermostat_status;

struct ScheduleStruct {
  int id;
  enum DayEnum day;
  unsigned long time;
  int temperature;
  struct ScheduleStruct *next;
};

struct ScheduleStruct *schedule_head = NULL;
struct ScheduleStruct *schedule_end = NULL;
struct ScheduleStruct *schedule_current = NULL;

char AWS_STATUS_SERVER_URL[200]="";
char AWS_SCHEDULE_SERVER_URL[200]="";
char LOG_FILE[200]="";
char CONFIG_FILE[200]="";

long return_code=-1;

CURL *curl;
CURLcode res;

double thermocouple_temperature = 0.0;

int overide_temp = -200; //-200 = NULL
int time_delta; //differnce between device time and server time on boot
time_t sec;

static void read_server_status(void);
static void read_server_schedule(void);
static void read_thermocouple(void);
static void curl_init(char* url);
static void curl_run(void);
static void curl_cleanup(void);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static void extract_server_status(void);
static void extract_server_schedule(void);
static void add_schedule(struct ScheduleStruct new_schedule);
static void delete_schedule(void);
static void print_schedule(void);
static void update_server(void);
static void set_heater_power(void);
static void set_temp(void);
static void write_heater_file(void);
static void process_config_file(void);
static void process_arguments(int argc, char** argv);
static void print_help(void);
static void setup_log(void);

//read_server_status()
//input - none
//output - none
//Uses curl to get data from AWS Status Server
static void read_server_status(void){
  D(syslog(LOG_INFO, "Read server config\n"));

  curl_init(AWS_STATUS_SERVER_URL);
  curl_run();

  sec = time(NULL);
  time_delta = (int)status.time_last_update - sec; //start tracking time to discipline clock using server
  D(syslog(LOG_INFO, "%i = %i - %li\n", time_delta, (int)status.time_last_update, time(NULL)));

  extract_server_status();
  curl_cleanup();
}

//read_server_schedule()
//input - none
//output - none
//Uses curl to get data from AWS Schedule Server
static void read_server_schedule(void){
  D(syslog(LOG_INFO, "Read server schedule\n"));

  curl_init(AWS_SCHEDULE_SERVER_URL);
  curl_run();
  extract_server_schedule();
  curl_cleanup();
}

//extract_server_status()
//input - none
//output - none
//Parses AWS Status Server response into variables and saves to structure
static void extract_server_status(void){
  D(syslog(LOG_INFO, "Extract server config\n"));

  char * token = strtok(chunk.memory, "[");

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.id = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.time_last_programmed = (unsigned long)strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.time_last_update = (unsigned long)strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.curr_temp = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.set_temp = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    status.power = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    D(syslog(LOG_INFO, "~~~~~~~~~~~~%s\n", token));
    if(strcmp("null}]", token) == 0){
      status.new_temp = -200;
    } else {
      status.new_temp = strtol(token, NULL, 10);
    }

    D(syslog(LOG_INFO, "ID                   = %i", status.id));
    D(syslog(LOG_INFO, "TIME_LAST_PROGRAMMED = %lu", status.time_last_programmed));
    D(syslog(LOG_INFO, "TIME_LAST_UPDATE     = %lu", status.time_last_update));
    D(syslog(LOG_INFO, "CURR_TEMP            = %i", status.curr_temp));
    D(syslog(LOG_INFO, "SET_TEMP             = %i", status.set_temp));
    D(syslog(LOG_INFO, "POWER                = %i", status.power));
    D(syslog(LOG_INFO, "NEW_TEMP             = %i", status.new_temp));
}

//extract_server_schedule()
//input - none
//output - none
//Parses response from AWS Schedule Server and places into schedule struct array
static void extract_server_schedule(void){
  D(syslog(LOG_INFO, "Extract server schedule\n"));

  delete_schedule();

  char * token = strtok(chunk.memory, "[");

  struct ScheduleStruct temp;

  token = strtok(NULL, ":"); token = strtok(NULL, "\"");

  while(token != NULL){
    temp.id = strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    if(strcmp(token, "MONDAY")==0)
      temp.day = Monday;
    else if(strcmp(token, "TUESDAY")==0)
      temp.day = Tuesday;
    else if(strcmp(token, "WEDNESDAY")==0)
      temp.day = Wednesday;
    else if(strcmp(token, "THURSDAY")==0)
      temp.day = Thursday;
    else if(strcmp(token, "FRIDAY")==0)
      temp.day = Friday;
    else if(strcmp(token, "SATURDAY")==0)
      temp.day = Saturday;
    else if(strcmp(token, "SUNDAY")==0)
      temp.day = Sunday;

    token = strtok(NULL, ":"); token = strtok(NULL, ":");
    memmove(token, token+1, strlen(token));
    temp.time = ((unsigned long)strtol(token, NULL, 10))*3600;

    token = strtok(NULL, ":");
    temp.time += ((unsigned long)strtol(token, NULL, 10))*60;

    token = strtok(NULL, "\"");
    temp.time += (unsigned long)strtol(token, NULL, 10);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
    temp.temperature = strtol(token, NULL, 10);

    add_schedule(temp);

    token = strtok(NULL, ":"); token = strtok(NULL, "\"");
  }

  print_schedule();

}

//add_schedule()
//input - none
//output - none
//Add a new schedule struct to linked list of structs, update pointers
static void add_schedule(struct ScheduleStruct new_schedule){
  D(syslog(LOG_INFO, "Add schedule\n"));

  struct ScheduleStruct *ptr = (struct ScheduleStruct*) malloc(sizeof(struct ScheduleStruct));

  ptr->id = new_schedule.id;
  ptr->day = new_schedule.day;
  ptr->time = new_schedule.time;
  ptr->temperature = new_schedule.temperature;
  ptr->next = NULL;

  if(schedule_head == NULL){
    schedule_head = ptr;
    schedule_end = ptr;
  } else {
    schedule_end->next = ptr;
    schedule_end = ptr;
  }
}

//print_schedule()
//input - none
//output - none
//Print linked list of schedule structures if debug is turned on
static void print_schedule(void){
  D(syslog(LOG_INFO, "Print schedule\n"));
  schedule_current = schedule_head;

  while(schedule_current != NULL){
    D(syslog(LOG_INFO, "ID          = %i", schedule_current->id));
    D(syslog(LOG_INFO, "DAY         = %i", schedule_current->day));
    D(syslog(LOG_INFO, "TIME        = %ld", schedule_current->time));
    D(syslog(LOG_INFO, "TEMPERATURE = %i", schedule_current->temperature));
    schedule_current = schedule_current->next;
  }
}

//delete_schedule()
//input - none
//output - none
//Delete entire linked list of schedule structs and free memory
static void delete_schedule(void){
  D(syslog(LOG_INFO, "Delete schedule\n"));

  while(schedule_head != NULL){
    schedule_current = schedule_head;
    schedule_head = schedule_head->next;
    free(schedule_current);
  }

  schedule_head = NULL;
  schedule_current = NULL;
  schedule_end = NULL;
}

//set_heater_power()
//input - none
//output - none
//Determines and sets power for the heater
static void set_heater_power(void){

  thermostat_status.time_last_programmed = status.time_last_programmed; //save server data to local struct
  thermostat_status.time_last_update = status.time_last_update;

  status.curr_temp = round(thermocouple_temperature); //set current temp
  thermostat_status.curr_temp = status.curr_temp;

  set_temp(); //determine what set temp should be

  status.set_temp = thermostat_status.set_temp; //set temp in structs
  status.power = thermostat_status.power;

  write_heater_file(); //toggle heater power
}

//set_temp()
//input - none
//output - none
//Determine what set temp should be, from schedule or override
static void set_temp(void){
  D(syslog(LOG_INFO, "Set temp\n"));

  int time_now = time(NULL) + time_delta;

  D(syslog(LOG_INFO, "time() %li\n", time(NULL)));
  D(syslog(LOG_INFO, "*!*!*!**!*!*!*!**!Current time %d\n", time_now));

  setenv("TZ", "MST7EDT", 1); //use MST
  tzset();

  time_t rawtime = time_now; //create vars for time
  struct tm *local = localtime(&rawtime);
  D(syslog(LOG_INFO, "%d:%d:%d %d %d, %d\n", local->tm_hour, local->tm_min, local->tm_sec, local->tm_mon+1, local->tm_mday, local->tm_year + 1900));

  int today = local->tm_wday; //get today's weekday
  if(today == 0){
    today = 7;
  }
  D(syslog(LOG_INFO, "Today is %i\n", today));

  int totime = (local->tm_hour * 3600) + (local->tm_min * 60) + local->tm_sec; //get current 24 hr time in sec
 
  schedule_current = schedule_head; //start at first schedule entry

  struct ScheduleStruct set_schedule;
  set_schedule = *schedule_current;

  if(schedule_current == NULL){
    //no program
     thermostat_status.set_temp = 70; //default thermostat temp
  } else {
    while (schedule_current != NULL){ //walk through schedule struct
      D(syslog(LOG_INFO, "Try id %i\n", schedule_current->id));
      D(syslog(LOG_INFO, "Schedule day %i today %i",schedule_current->day, today )); 

      if(schedule_current->day > today){ //if looking at tomorrow's schedule, use todays -> break
        D(syslog(LOG_INFO, "Stop as %i > %i\n", schedule_current->day, today));
        break;
      }

      if((schedule_current->day >= today) && (schedule_current->time > totime)){ //if looking at future schedule, break
        D(syslog(LOG_INFO, "Stop as %i >= %i && %li > %i", schedule_current->day, today, schedule_current->time, totime));
        break;
      }
      
      set_schedule = *schedule_current; //go to next schedule
      schedule_current = schedule_current->next;
      D(syslog(LOG_INFO, "Try next schedule\n"));
    }

    D(syslog(LOG_INFO, "Decided on id %i temp %i\n", set_schedule.id, set_schedule.temperature));
    thermostat_status.set_temp = set_schedule.temperature; //use last valid schedule
  }

  //check override
  if(status.new_temp != -200){ //got new override temp
    thermostat_status.new_temp = status.new_temp;
    thermostat_status.id = status.id; //current schedule id, will reset when it changes
    D(syslog(LOG_INFO, "Got NEW TEMP %i\n", status.new_temp));
  }

  if( (thermostat_status.new_temp != -200) && (thermostat_status.id == status.id) ){ //in override state
    D(syslog(LOG_INFO, "OVERRIDE MODE Temp = %i\n", thermostat_status.new_temp));   
    thermostat_status.set_temp = thermostat_status.new_temp;
  } else if (thermostat_status.id != status.id){ //new schedule id, forget override
    D(syslog(LOG_INFO, "REMOVE OVERRIDE\n"));
    thermostat_status.new_temp = -200;
    thermostat_status.id = status.id;
  }

  if(thermostat_status.curr_temp < thermostat_status.set_temp){ //determine if heater should be on or off
    thermostat_status.power = 1;
  } else {
    thermostat_status.power = 0;
  }
}

//write_heater_file()
//input - none
//output - none
//Write to heater file to toggle power on/off
static void write_heater_file(void){
  D(syslog(LOG_INFO, "Write Heater File\n"));

  FILE *file = fopen(HEATER_POWER_FILE, "w");
  if(file == NULL){
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log session error
    return;
  }

  if(thermostat_status.power == 1){ //turn on
    D(syslog(LOG_INFO, "POWER ON\n"));
    fprintf(file, "ON:%i",status.time_last_update); //write state and time
  }else{
    D(syslog(LOG_INFO, "POWER_OFF\n")); //turn off
    fprintf(file, "OFF:%i",status.time_last_update); //write state and time
  }

  fclose(file);
}

//update_server()
//input - none
//output - none
//Write status to AWS status server
static void update_server(void){
  D(syslog(LOG_INFO, "Update server\n"));

  char temp[1024];

  snprintf(temp, sizeof(temp), "%s/?id=%i&&curr_temp=%i&&set_temp=%i&&power=%i", AWS_STATUS_SERVER_URL, status.id, status.curr_temp, status.set_temp, status.power); //format url to send via curl

  D(syslog(LOG_INFO, "~~~SEND URL %s\n", temp));


  curl_init(temp);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); //configure curl to ask for PUT request
  curl_run();
  curl_cleanup();
}

//WriteMemoryCallback()
//input - void *contents - new response string
//      - size_t size - size of new response string
//      - size_t nmemb - size of new response string
//      - void *userp - struct of response data
//output - size_t
//HTTP delete request
//From curl.se/libcurl/c/getinmemory.html
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb; //set size of new response string
  struct MemoryStruct *mem = (struct MemoryStruct *)userp; //populate memory struct

  char *ptr = realloc(mem->memory, mem->size + realsize + 1); //add space for new response string
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr; //save larger space
  memcpy(&(mem->memory[mem->size]), contents, realsize); //copy new response to larger space
  mem->size += realsize;  //update size of string
  mem->memory[mem->size] = 0; //reset struct

  return realsize;
}

//curl_init()
//input - none
//output - none
//Initializes curl, sets curl url, sets curl to use function to collect return data
static void curl_init(char* url){
  D(syslog(LOG_INFO, "Curl init\n"));

  chunk.memory = malloc(1);  //initialize memory to store curl response
  chunk.size = 0;    //set intial response to zero

  curl = curl_easy_init();
  if(!curl){
    syslog(LOG_INFO, "CURL ERROR\n");
    exit(INIT_ERR);
  }

  if(url == NULL){
    syslog(LOG_INFO, "MISSING URL\n");
    exit(INPUT_ERR);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); //callback function to save curl response to memory
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk); //memory to store curl response
}

//curl_run()
//input - none
//output - none
//Runs curl command, collects response code, prints response code and return data
static void curl_run(void){
  D(syslog(LOG_INFO, "Curl run\n"));
  res = curl_easy_perform(curl);
  if(res != CURLE_OK){
    exit(HTTP_ERR);
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code); //get curl response code

  D(syslog(LOG_INFO, "RESPONSE CODE: "));
  D(syslog(LOG_INFO, "%ld\n", return_code));

  D(syslog(LOG_INFO, "RESPONSE DATA:\n"));
  D(syslog(LOG_INFO, "%s\n", chunk.memory));
}

//curl_cleanup()
//input - none
//output - none
//Run curl cleanup command
static void curl_cleanup(void){
  D(syslog(LOG_INFO, "Curl cleanup\n"));
  curl_easy_cleanup(curl);

  free(chunk.memory); //clear allocated memory
}

//read_thermocouple()
//input - none
//output - none
//Reads thermocouple file and sets status structure
static void read_thermocouple(void){
  D(syslog(LOG_INFO, "Read thermocouple\n"));

  FILE* file_ptr;
  
  if((file_ptr = fopen(THERMOCOUPLE_FILE, "r")) == NULL){
    syslog(LOG_INFO, "Unable to read Thermocouple file, will try again\n");
    return;
  }

  fscanf(file_ptr, "%lf", &thermocouple_temperature);

  D(syslog(LOG_INFO, "Got thermocouple temperature = %f\n", thermocouple_temperature));

  fclose(file_ptr);
}

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
//Function runs indefinitely, calls main thermostat functions
static void _do_work(void){
  time_t sec;
  sec = time(NULL); //initialize time

  syslog(LOG_INFO, "Initial Time since Epoch = %ld\n", sec);
  
  while(true){ //infinite loop

    //read config from server
    read_server_status();

    //read data from thermocouple
    read_thermocouple();

    //determine heater power state
    read_server_schedule();

    //set heater power
    set_heater_power();

    //write data to server
    update_server();

    //sleep
    sleep(CONFIG_SLEEP);
  }
}

//process_arguments()
//input - none
//output - none
//Parses command line arguments
static void process_arguments(int argc, char** argv){
  D(syslog(LOG_INFO, "process_arguments %i\n",argc)); 

  if(argc < 1){ 
    fprintf(stderr, "TOO FEW ARGUMENTS\n"); 
    print_help(); 
    exit(INPUT_ERR); 
  } 
 
  for(int i=1; i<argc; i++){ 
    D(syslog(LOG_INFO, "%d %s\n", i, argv[i])); 
 
    if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ){ 
      print_help(); 
      exit(OK); 
 
    }else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config_file") ){ 
      //config file 
      i++; 
      if(i >= argc){ 
        fprintf(stderr, "MISSING CONFIG FILE\n"); 
        print_help(); 
        exit(INPUT_ERR); 
      } 
 
      D(syslog(LOG_INFO, "%d %s\n", i, argv[i])); 
      strcpy(CONFIG_FILE, argv[i]); 
      D(syslog(LOG_INFO, "Saved CONFIG FILE: %s\n", CONFIG_FILE)); 
 
    }else{
      fprintf(stderr, "INVALID ARGUMENT INPUT\n");
      print_help();
      exit(INPUT_ERR);
    }
  }

  if(strcmp(CONFIG_FILE, "")==0)
    strcpy(CONFIG_FILE,"/etc/thermostat/thermostat.config"); //default config file

  D(syslog(LOG_INFO, "FINAL CONFIG_FILE %s\n", CONFIG_FILE)); 
}

//print_help()
//input - none
//output - none
//Prints the help information about how to use thermostat
void print_help(void){
  fprintf(stderr, "Usage: thermostat [-h] [-c config_file_location]\n");
  fprintf(stderr, "    -h, --help     Print help text\n");
  fprintf(stderr, "    -c, --config   specify config file location, otherwise default ./thermostat.config\n");
}

//process_config_file()
//input - none
//output - none
//Parces and saves config file values to memory
static void process_config_file(void){
  D(syslog(LOG_INFO, "process_config_file\n"));

  D(syslog(LOG_INFO, "STATUS_SERVER_URL %s\n", AWS_STATUS_SERVER_URL)); 
  D(syslog(LOG_INFO, "SCHEDULE_SERVER_URL %s\n", AWS_SCHEDULE_SERVER_URL)); 
  D(syslog(LOG_INFO, "LOG_FILE %s\n", LOG_FILE)); 

  FILE *file = NULL;

  if((file = fopen(CONFIG_FILE, "r")) == NULL){
    syslog(LOG_INFO, "Unable to read Thermocouple file, will try again\n");
    return;
  }

  fscanf(file, "STATUS_SERVER_URL=%s\n", AWS_STATUS_SERVER_URL);
  fscanf(file, "SCHEDULE_SERVER_URL=%s\n", AWS_SCHEDULE_SERVER_URL);
  fscanf(file, "LOG_FILE=%s\n", LOG_FILE);

  D(syslog(LOG_INFO, "STATUS_SERVER_URL %s", AWS_STATUS_SERVER_URL)); //configs read from config file
  D(syslog(LOG_INFO, "SCHEDULE_SERVER_URL %s", AWS_SCHEDULE_SERVER_URL)); 
  D(syslog(LOG_INFO, "LOG_FILE %s", LOG_FILE)); 

  if(strcmp(AWS_STATUS_SERVER_URL, "")==0) //if empty config, use default urls
    strcpy(AWS_STATUS_SERVER_URL, "http://ec2-18-119-152-234.us-east-2.compute.amazonaws.com/thermostat_status_server.php");
  if(strcmp(AWS_SCHEDULE_SERVER_URL,"")==0)
    strcpy(AWS_SCHEDULE_SERVER_URL, "http://ec2-18-119-152-234.us-east-2.compute.amazonaws.com/thermostat_schedule_server.php");

  D(syslog(LOG_INFO, "FINAL STATUS_SERVER_URL %s\n", AWS_STATUS_SERVER_URL)); 
  D(syslog(LOG_INFO, "FINAL SCHEDULE_SERVER_URL %s\n", AWS_SCHEDULE_SERVER_URL)); 
  D(syslog(LOG_INFO, "FINAL LOG_FILE %s\n", LOG_FILE)); 

  fclose(file);
}

//setup_log()
//input - none
//output - none
//Close old log and setup new log with specified log file - Currently not working
void setup_log(void){
  D(syslog(LOG_INFO, "setup_log")); 

  closelog();
  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL1); //configure syslog information and parameters

  FILE *file = NULL;

  if((file = fopen("/etc/syslog.conf", "a")) == NULL){
    syslog(LOG_INFO, "Unable to read Thermocouple file, will try again\n");
    return;
  }

  char var[200]= "";
  strcat(var, "local1.info ");
  strcat(var, LOG_FILE);

  D(syslog(LOG_INFO, "Redirect syslog %s\n", var));

  system("pkill -HUP syslogd");
}

//main()
//input - none
//output - int exit status
//Initial daemon config, calls work function
int main(int argc, char **argv){

  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON); //configure syslog information and parameters

  process_arguments(argc, argv); //process command line arguments
  process_config_file(); //process config file

  syslog(LOG_INFO, "Starting thermostat\n");

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

//  if(access(LOG_FILE_NAME, F_OK) != 0){ //check if log file exists - not currently implemented
//    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno)); //log error and exit
//    return ERR_NO_LOG_FILE;
//  }

  signal(SIGTERM, _signal_handler); //send signals to _signal_handler function
  signal(SIGHUP, _signal_handler);

  _do_work(); //run main loop

  return ERR_WTF; //main loop crashed
}
