// Exports CMake variables into C++.
#define PROJECT_NAME "${PROJECT_NAME}"
#define PROJECT_AUTHOR "${PROJECT_AUTHOR}"
#define PROJECT_DESCRIPTION "${PROJECT_DESCRIPTION}"
#define PROJECT_URL "${PROJECT_HOMEPAGE_URL}"
#define PROJECT_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}"
#define PROJECT_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}"
#define PROJECT_COMMIT "${PROJECT_COMMIT}"

// Discord command definitions
#define CMD_GET_SCRIPT "get_script"

#define CMD_SEARCH_WADS "search_wads"
#define CMD_LIST_WADS "list_wads"
#define CMD_ADDFILE_UPLOAD "addfile_upload"
#define CMD_ADDFILE_LINK "addfile_link"

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
