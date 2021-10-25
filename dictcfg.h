#ifndef _DICTCFG_H_
#define _DICTCFG_H_ 1

#include <stdio.h>
#include "dict.h"

dict_p dictcfg_load(const char *cfg_path, FILE *fp_log);
dict_p dictcfg_section(dict_p cfg,const char *name);

int dictcfg_getint(dict_p section, const char *key, int def);
char *dictcfg_getstr(dict_p section, const char *key, char *def);
double dictcfg_getfloat(dict_p section, const char *key, double def);
int dictcfg_getbool(dict_p section, const char *key, int def);

// NOTE
// dictcfg_getbool() returns 1 if text is 'ok' or 'yes',
// returns 0 if text is 'false' or 'no',
// otherwise, it returns -1.

#endif
