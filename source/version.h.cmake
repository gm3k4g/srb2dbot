// Exports CMake variables into C++.
#define PROJECT_NAME "${PROJECT_NAME}"
#define PROJECT_AUTHOR "${PROJECT_AUTHOR}"
#define PROJECT_DESCRIPTION "${PROJECT_DESCRIPTION}"
#define PROJECT_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}"
#define PROJECT_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}"


// Commandline arguments.
#define ARG_HELP "-h"
#define ARG_HELP_GNU "--help"
#define ARG_RUN "-r"
#define ARG_RUN_GNU "--run"
#define ARG_VERSION "-v"
#define ARG_VERSION_GNU "--version"
#define ARG_PARAMS "-p"
#define ARG_PARAMS_GNU "--parameters"
#define ARG_CONFIG "-c"
#define ARG_CONFIG_GNU "--config"
#define ARG_NOCONFIG "-n"
#define ARG_NOCONFIG_GNU "--noconfig"
#define ARG_EXECUTABLE "-e"
#define ARG_EXECUTABLE_GNU "--executable"
#define ARG_HOMEDIR "-h"
#define ARG_HOMEDIR_GNU "--home"
#define SRB2_ARGUMENTS_LIST "https://wiki.srb2.org/wiki/Command_line_parameters"

#define USAGE \
PROJECT_NAME " v" PROJECT_VERSION_MAJOR "." PROJECT_VERSION_MINOR "\n"\
"AUTHOR: " PROJECT_AUTHOR "\n"\
"\n"\
"DESCRIPTION: " PROJECT_DESCRIPTION "\n"\
"\n"\
"To print this message again, you can either call " PROJECT_NAME " with no arguments, or call it with the " ARG_HELP_GNU " or " ARG_HELP " arguments.\n"\
"\n"\
"USAGE: \n"\
"\t" PROJECT_NAME " " ARG_RUN " " ARG_RUN_GNU "\n"\
"\t" PROJECT_NAME " " ARG_HELP " " ARG_HELP_GNU "\n"\
"\t" PROJECT_NAME " " ARG_VERSION " " ARG_VERSION_GNU "\n"\
"\n"\
"(WIP:)\n"\
"\t" PROJECT_NAME " " ARG_PARAMS " " ARG_PARAMS_GNU "\n"\
"\t" PROJECT_NAME " " ARG_CONFIG " " ARG_CONFIG_GNU "\n"\
"\n"\
"ARGUMENTS: \n"\
"\t <COMMAND-LINE ARGUMENTS>\n"\
"\t\tA list of arguments that SRB2 can process before the game starts. This can be one argument or many. It's advised to visit the SRB2 wiki (" << SRB2_ARGUMENTS_LIST << ") to look at what commandline arguments you can provide to SRB2.\n"\
"\t\tFor example, to start a dedicated server on port 5029 and have your server show up in the standard rooms, execute the program like so: \n"\
"\t\t\n"\
"\t\t`" PROJECT_NAME " " ARG_PARAMS " -dedicated -port 5029 -room 38` \n"\
"\n"\
"OPTIONS: \n"\
"\t" ARG_PARAMS ", " ARG_PARAMS_GNU "\n"\
"\t\tThis option is used to indicate that everything following this option will be parameters to give to SRB2 before it starts.\n"\
"\n"\
"\t\tNote that this on its own will not run the program, as `{ARG_RUN_S} | {ARG_RUN}` are required to do that.\n"\
"\t\t\n"\
"\t" ARG_HOMEDIR ", " ARG_HOMEDIR_GNU "\n"\
"\t\tSpecify the home directory of SRB2. If not specified, it will use \"${HOME}/.srb2/srb2dbot.d\" by default.\n"\
"\t\t(On windows you might have to specify it regardless)\n"\
"\t\tThe directory will be created when srb2dbot runs, if it doesn't already exist.\n"\
"\t" ARG_CONFIG ", " ARG_CONFIG_GNU "\n"\
"\t\tIf specified on its own, it opens the config at the default path and directly reads the arguments from there. If one doesn't already exist, it will be created.\n"\
"\t\tIf you have a config at a specific path, then you may specify this path (e.g. -c /path/to/my/file.sh ).\n"\
 "\t\tNote that this on its own will not run the program, as `{ARG_RUN_S} | {ARG_RUN}` are required to do that.\n"\
"\t\t\n"\
// TODO: get rid of "extra" parameters and instead make them\
// CLI arguments.\
"\t" ARG_RUN ", " ARG_RUN_GNU "\n"\
"\t\tIf specified on its own, immediately starts the program with the default parameters. That is to say, it will look for a default config and read for any potential arguments before starting.\n"\
"\t\tThis is primarily stacked with the above parameters (e.g. config/parameters)\n"\
"\t\tThere are some extra parameters you can provide here, and they are as follows:\n"\
"\t\t\tNOCONFIG: This will tell the program to just start without reading from any default config. (e.g. --start NOCONFIG )\n"\
"\t\t\tRUNSRB2 : This will tell the program to run the program, but to also run SRB2. You can have more control over SRB2 this way.\n"\
"\t\t\t\tHere you can also specify a path to your SRB2 executable. (e.g. --start RUNSRB2 /path/to/srb2)\n"\
"\t\t\n"\
"\t" ARG_HELP ", " ARG_HELP_GNU "\n"\
"\t\tShow the help message.\n"\
"\t\t\n"\
"\t" ARG_VERSION ", " ARG_VERSION_GNU "\n"\
"\t\tShow program version.\n"


//void usage(char* progName)
//{
//  cout << progName << "[options]" << endl <<
//      "Options:" << endl <<
//      "-h | --help        Print this help" << endl <<
//      "-v | --version     Print the SVN version" << endl <<
//      "-V | --Version     Print the proxy version" << endl <<
//      "-d | --daemonize   Run as daemon" << endl <<
//      "-P | --pidfile     Path to PID file (default: " <<
//        WPASUP_PROXY_DEFAULT_PID_FILE << ")" << endl <<
//      "-l | --logging     Path to logging file (default: " <<
//        WPASUP_PROXY_DEFAULT_LOGGING << ")" << endl <<
//      "-i | --ip          The IP address of the main application (default: " <<
//        WPASUP_PROXY_MAIN_APP_IP << ")" << endl <<
//      "-p | --port        The port number of the main application (default: " <<
//        WPASUP_PROXY_DEFAULT_MAIN_APP_PORT << ")" << endl <<
//      "-w | --wpa_cli     Path to wpa_cli program (default: " <<
//        WPASUP_PROXY_DEFAULT_WPA_CLI << ")" << endl;
//}

// NOTE:
// For now, there are increased security concerns with the code.
// Suspicious people can possibly inject code and perform arbitrary code execution.
// The attack surface is reduced via trust for the moment.
// Eventually though, these security concerns will be addressed.

// Discord command definitions
#define CMD_GET_SCRIPT "get_script"

#define CMD_LIST_FILES "listfiles"
#define CMD_ADDFILE_UPLOAD "addfile_upload"
#define CMD_ADDFILE_LINK "addfile_link"
#define CMD_REMOVE_FILE "removefile"
#define CMD_RENAME_FILE "renamefile"

#define CMD_FIND_LINE "find_line"
#define CMD_INSERT_LINE "insert_line"
#define CMD_REMOVE_LINE "remove_line"
#define CMD_CHANGE_LINE "change_line"
#define CMD_MOVE_LINE "move_line"
#define CMD_INSPECT_LINE "inspect_line"

#define CMD_RESTART_SERVER "restart_server"
#define CMD_STOP_SERVER "stop_server"

#define CMD_SERVER_DO "server_do"
#define CMD_SERVER_SAY "server_say"
#define CMD_KICK_PLAYER "kick_player"
#define CMD_BAN_PLAYER "ban_player"
