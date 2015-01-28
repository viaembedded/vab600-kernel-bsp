/*
    Input Parser for Board Support Driver
    
    Read input from file write interface and parse command.
    Output result to file read interface.
    Read / Write environment variable via WMT api.
    
    How-To:
     - Write "selenv xxx" to /dev/bs to select environment setting by name 
       in serial flash. In this example, variable named xxx will be selected.
     - Write "setenv aaa" to /dev/bs to set variable named xxx to aaa.
     - Read from /dev/bs to get value of variable xxx.
     - If no variable selected, no output is appear from /dev/bs.
*/
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "board_support.h"

#define BSPI_PROC_LEN 256

struct bs_parse_dat_t {
	char *sel_name;
	char *new_env;
};

static struct bs_parse_dat_t gBSParseDat;

/* parse state */
#define BSPS_IDLE 0
#define BSPS_WHITESPACE 1
#define BSPS_TOKEN 2

#define BSPS_CMD 3
#define BSPS_ARG 4

/* current func in parsing */
#define BSPF_NULL 0
#define BSPF_SELENV 1
#define BSPF_SETENV 2

/* send args to specific command */
int _bs_parse_func(int func, char *arg, int arglen)
{
	if(func == BSPF_SELENV){
		if(gBSParseDat.sel_name != NULL){
			ERR("BS: selenv already done, current selected: %s\n", 
				gBSParseDat.sel_name);
			return -1;
		}
		gBSParseDat.sel_name = kmalloc(arglen+1, GFP_KERNEL);
		if(gBSParseDat.sel_name == NULL){
			ERR("BS: alloc for selenv failed\n");
			return -1;
		}
		strncpy(gBSParseDat.sel_name, arg, arglen);
		gBSParseDat.sel_name[arglen] = 0;
		DBG("BS: selenv: %s\n", gBSParseDat.sel_name);
		
	}else if(func == BSPF_SETENV){
		if(gBSParseDat.sel_name == NULL){
			ERR("BS: no selected env name\n");
			return -1;
		}
		if(gBSParseDat.new_env != NULL){
			ERR("BS: too many args for setenv\n");
			return -1;
		}
		gBSParseDat.new_env = kmalloc(arglen+1, GFP_KERNEL);
		if(gBSParseDat.new_env == NULL){
			ERR("BS: alloc for selenv failed\n");
			return -1;
		}
		strncpy(gBSParseDat.new_env, arg, arglen);
		gBSParseDat.new_env[arglen] = 0;
		if(wmt_setsyspara(gBSParseDat.sel_name, gBSParseDat.new_env) != 0){
			ERR("BS: setenv: %s=%s failed\n", 
				gBSParseDat.sel_name, gBSParseDat.new_env);
		}else{
			DBG("BS: setenv: %s=%s ok\n", 
				gBSParseDat.sel_name, gBSParseDat.new_env);
		}
	}
	return 0;
}

/* when first token is got and it is a command, do init for this command */
int _bs_parse_func_init(int func)
{
	if(func == BSPF_SELENV){
		if(gBSParseDat.sel_name != NULL){
			kfree(gBSParseDat.sel_name);
			gBSParseDat.sel_name = NULL;
		}
	}else if(func == BSPF_SETENV){
		if(gBSParseDat.new_env != NULL){
			kfree(gBSParseDat.new_env);
			gBSParseDat.new_env = NULL;
		}		
	}
	return 0;
}

/* parse a line of command */
int bs_parse_input(const char __user *buf, size_t count, loff_t *pos)
{
	char pbuf[BSPI_PROC_LEN];
	char tokbuf[BSPI_PROC_LEN];
	char *tokstart = NULL;
	int i, tok_len, parse_stat, parse_func, cmd_got;
	
	if(count >= BSPI_PROC_LEN){
		ERR("BS: input too long\n");
		return -1;
	}else{
		if(copy_from_user(pbuf, buf, count) != 0){
			ERR("BS: fail to copy\n");
			return -1;
		}
		pbuf[count] = 0;
	}
	//DBG("BS: in=\"%s\"\n", pbuf);
	
	parse_stat = BSPS_IDLE;
	parse_func = BSPF_NULL;
	tok_len = 0;
	cmd_got = 0;
	for(i=0; i<count; i++){
		switch(parse_stat){
		case BSPS_IDLE:
			if(pbuf[i] == ' ' || pbuf[i] == '\n'){
				continue;
			}
			/* treat first token as command */
			if(cmd_got == 0){
				parse_stat = BSPS_CMD;
			}else{
				parse_stat = BSPS_ARG;
			}
			tokstart = &pbuf[i];
			tok_len = 1;
			break;
		
		case BSPS_CMD:
			if(pbuf[i] == ' ' || pbuf[i] == '\n' || pbuf[i] == 0){
				parse_stat = BSPS_IDLE;
				/* copy out this token */
				strncpy(tokbuf, tokstart, tok_len);
				tokbuf[tok_len] = 0;
				DBG("BS: cmd=\"%s\"\n", tokbuf);
				/* process command */
				if(strcmp(tokbuf, "selenv") == 0){
					parse_func = BSPF_SELENV;
					_bs_parse_func_init(BSPF_SELENV);
					
				}else if(strcmp(tokbuf, "setenv") == 0){
					parse_func = BSPF_SETENV;
					_bs_parse_func_init(BSPF_SETENV);
					
				}else{
					DBG("BS: unknown cmd\n");
					return -1;
				}
				cmd_got = 1;
			}else{
				tok_len++;
				continue;
			}
			break;
		
		case BSPS_ARG:
			/* now all word except 0 and new line after here is arg */
			if(pbuf[i] != '\n' && pbuf[i] != 0){
				tok_len++;
				if(i < count - 1){
					/* don't miss nonzero end string */
					continue;
				}
			}
			strncpy(tokbuf, tokstart, tok_len);
			tokbuf[tok_len] = 0;
			DBG("BS: arg=\"%s\"\n", tokbuf);
			if(parse_func != BSPF_NULL){
				return _bs_parse_func(parse_func, tokbuf, tok_len);
			}else{
				return -1;
			}
			break;
			
		default:
			DBG("BS: we should not get here\n");
			return -1;
		}
	}

/* Code below is used to parse input as many token. 
   This is not suitable for us to write whitespace as setting value. */
#if 0
	for(i=0; i<=count; i++){
		switch(parse_stat){
		case BSPS_TOKEN:
			if(pbuf[i] == ' ' || pbuf[i] == '\n' || pbuf[i] == 0){
				parse_stat = BSPS_WHITESPACE;
				/* copy out this token */
				strncpy(tokbuf, tokstart, tok_len);
				tokbuf[tok_len] = 0;
				DBG("BS: token=\"%s\"\n", tokbuf);
				if(parse_func != BSPF_NULL){
					_bs_parse_func(parse_func, tokbuf, tok_len);
					continue;
				}
				/* process command */
				if(strcmp(tokbuf, "selenv") == 0){
					parse_func = BSPF_SELENV;
					_bs_parse_func_init(BSPF_SELENV);
					
				}else if(strcmp(tokbuf, "setenv") == 0){
					parse_func = BSPF_SETENV;
					_bs_parse_func_init(BSPF_SETENV);
					
				}else{
					DBG("BS: unknown cmd\n");
					return -1;
				}

			}else{
				tok_len++;
				continue;
			}
			break;
			
		case BSPS_WHITESPACE:
			if(pbuf[i] == ' ' || pbuf[i] == '\n'){
				continue;
			}else{
				parse_stat = BSPS_TOKEN;
			}
		
		case BSPS_IDLE:
			if(pbuf[i] == ' ' || pbuf[i] == '\n'){
				continue;
			}else{
				parse_stat = BSPS_TOKEN;
				tokstart = &pbuf[i];
				tok_len = 1;
			}
			/* might be white space */
			break;
		
		default:
			DBG("BS: we should not get here\n");
			return -1;
		}
	}
#endif
	return 0;
}

/* get selected environment variable's name */
char* bs_parse_input_get_selected_name(void)
{
	return gBSParseDat.sel_name;
}


