#include "commandToString.h"

const char* commandToString(CommandType cmd) {
    switch (cmd) {
        case CMD_STOP: return "CMD_STOP";
        case CMD_UP_SLOW: return "CMD_UP_SLOW";
        case CMD_UP_MEDIUM: return "CMD_UP_MEDIUM";
        case CMD_UP_FAST: return "CMD_UP_FAST";
        case CMD_DOWN_SLOW: return "CMD_DOWN_SLOW";
        case CMD_DOWN_MEDIUM: return "CMD_DOWN_MEDIUM";
        case CMD_DOWN_FAST: return "CMD_DOWN_FAST";
        case CMD_MOVE_TO: return "CMD_MOVE_TO";
        case CMD_MOVE_TO_DOWN_LIMIT: return "CMD_MOVE_TO_DOWN_LIMIT";
        case CMD_DOWN_LIMIT_STATUS: return "CMD_DOWN_LIMIT_STATUS";
        case CMD_REQUEST_DOWN_STOP: return "CMD_REQUEST_DOWN_STOP";
        case CMD_GET_POSITION: return "CMD_GET_POSITION";
        case CMD_RESET: return "CMD_RESET";
        case CMD_ACK: return "CMD_ACK";
        case CMD_POSITION: return "CMD_POSITION";
        case CMD_UP_LIMIT_TRIP: return "CMD_UP_LIMIT_TRIP";
        case CMD_UP_LIMIT_OK: return "CMD_UP_LIMIT_OK";
        case CMD_DOWN_LIMIT_TRIP: return "CMD_DOWN_LIMIT_TRIP";
        case CMD_DOWN_LIMIT_OK: return "CMD_DOWN_LIMIT_OK";
        default: return "UNKNOWN_CMD";
    }
}
