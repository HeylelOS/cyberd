#include "command_reader.h"

enum command_reader
command_reader_next(enum command_reader current, char c) {

	switch(current) {
	case COMMAND_READER_START:
		if(c == 'd') {
			return COMMAND_READER_D;
		} else if(c == 's') {
			return COMMAND_READER_S;
		} else if(c == 'c') {
			return COMMAND_READER_C;
		} break;
	case COMMAND_READER_D:
		if(c == 's') {
			return COMMAND_READER_DS;
		} else if(c == 'r') {
			return COMMAND_READER_DR;
		} else if(c == 'e') {
			return COMMAND_READER_DE;
		} break;
	case COMMAND_READER_DS:
		if(c == 't') {
			return COMMAND_READER_DST;
		} break;
	case COMMAND_READER_DST:
		if(c == 't') {
			return COMMAND_READER_DAEMON_START;
		} else if(c == 'p') {
			return COMMAND_READER_DAEMON_STOP;
		} break;
	case COMMAND_READER_DR:
		if(c == 'l') {
			return COMMAND_READER_DRL;
		} break;
	case COMMAND_READER_DRL:
		if(c == 'd') {
			return COMMAND_READER_DAEMON_RELOAD;
		} break;
	case COMMAND_READER_DE:
		if(c == 'n') {
			return COMMAND_READER_DEN;
		} break;
	case COMMAND_READER_DEN:
		if(c == 'd') {
			return COMMAND_READER_DAEMON_END;
		} break;
	case COMMAND_READER_S:
		if(c == 'p') {
			return COMMAND_READER_SP;
		} else if(c == 'h') {
			return COMMAND_READER_SH;
		} else if(c == 'r') {
			return COMMAND_READER_SR;
		} else if(c == 's') {
			return COMMAND_READER_SS;
		} break;
	case COMMAND_READER_SP:
		if(c == 'w') {
			return COMMAND_READER_SPW;
		} break;
	case COMMAND_READER_SPW:
		if(c == 'f') {
			return COMMAND_READER_SYSTEM_POWEROFF;
		} break;
	case COMMAND_READER_SH:
		if(c == 'l') {
			return COMMAND_READER_SHL;
		} break;
	case COMMAND_READER_SHL:
		if(c == 't') {
			return COMMAND_READER_SYSTEM_HALT;
		} break;
	case COMMAND_READER_SR:
		if(c == 'b') {
			return COMMAND_READER_SRB;
		} break;
	case COMMAND_READER_SRB:
		if(c == 't') {
			return COMMAND_READER_SYSTEM_REBOOT;
		} break;
	case COMMAND_READER_SS:
		if(c == 's') {
			return COMMAND_READER_SSS;
		} break;
	case COMMAND_READER_SSS:
		if(c == 'p') {
			return COMMAND_READER_SYSTEM_SUSPEND;
		} break;
	case COMMAND_READER_C:
		if(c == 'c') {
			return COMMAND_READER_CC;
		} break;
	case COMMAND_READER_CC:
		if(c == 't') {
			return COMMAND_READER_CCT;
		} break;
	case COMMAND_READER_CCT:
		if(c == 'l') {
			return COMMAND_READER_CREATE_CONTROLLER;
		} break;
	/* All ending's nexts discarded */
	default:
		break;
	}

	return COMMAND_READER_DISCARDING;
}

