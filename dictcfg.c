#include"dictcfg.h"
#include"logprintf.h"

#include<ctype.h>
#include<stdlib.h>
#include<string.h>

static void dictcfg_section_remove(void *value)
{
	dict_delete(value);
}

static int issym(int chr)
{
	return
		(chr >= 'A' && chr <= 'Z') ||
		(chr >= 'a' && chr <= 'z') ||
		(chr >= '0' && chr <= '9') ||
		chr == '_' ||
		chr >= 128;
}

static char *AllocCopy(const char *Str)
{
	size_t l = strlen(Str);
	char *NewAlloc = malloc(l + 1);
	if (!NewAlloc) return NULL;
	memcpy(NewAlloc, Str, l + 1);
	return NewAlloc;
}

dict_p dictcfg_load(const char *cfg_path, FILE *fp_log)
{
	dict_p d_cfg = NULL;
	dict_p d_sect = NULL;
	FILE *fp = NULL;
	size_t LineNo = 0;

	fp = fopen(cfg_path, "r");
	if (!fp) goto FailExit;

	d_cfg = dict_create();
	if (!d_cfg)
	{
		log_printf(fp_log, "Create new dictionary failed.\n");
		goto FailExit;
	}

	while (!feof(fp))
	{
		char LineBuf[256];
		char *ch = &LineBuf[0];
		char *ch2, *ch3;
		LineNo++;
		if (!fgets(LineBuf, sizeof LineBuf, fp)) break;

		while (isspace(*ch)) ch++;

		switch (ch[0])
		{
		case ';':
		case '#':
		case '\0': continue;
		case '[':
			ch2 = strchr(ch + 1, ']');
			if (!ch2)
			{
				log_printf(fp_log, "Line %zu: '%s': ']' expected.\n", LineNo, ch);
				goto FailExit;
			}
			ch2[1] = '\0';
			d_sect = dict_create();
			if (!d_sect)
			{
				log_printf(fp_log, "Line %zu: Create new sub dictionary failed.\n", LineNo);
				goto FailExit;
			}
			switch (dict_insert(d_cfg, ch, d_sect))
			{
			case ds_ok: break;
			case ds_nomemory:
				log_printf(fp_log, "Line %zu: No enough memory.\n", LineNo);
				dict_delete(d_sect);
				goto FailExit;
			case ds_alreadyexists:
				log_printf(fp_log, "Line %zu: '%s' already defined.\n", LineNo, ch);
				dict_delete(d_sect);
				goto FailExit;
			default:
				log_printf(fp_log, "Line %zu: dictionary error.\n", LineNo);
				dict_delete(d_sect);
				goto FailExit;
			}
			break;
		default:
			if (!d_sect)
			{
				log_printf(fp_log, "Line %zu: '[' expected.\n", LineNo);
				goto FailExit;
			}
			ch2 = ch + 1;
			while (issym(*ch2)) ch2++;
			while (isspace(*ch2)) *ch2++ = '\0';
			if (*ch2 != '=')
			{
				log_printf(fp_log, "Line %zu: '%s': '=' expected.\n", LineNo, ch2);
				goto FailExit;
			}
			*ch2++ = '\0';
			while (isspace(*ch2)) ch2++;

			ch3 = strchr(ch2, '#'); if (ch3) *ch3 = '\0';
			ch3 = strchr(ch2, ';'); if (ch3) *ch3 = '\0';
			ch3 = strchr(ch2, '\n'); if (ch3) *ch3 = '\0';
			ch2 = AllocCopy(ch2);
			if (!ch2)
			{
				log_printf(fp_log, "Line %zu: No enough memory.\n", LineNo);
				goto FailExit;
			}
			switch (dict_insert(d_sect, ch, ch2))
			{
			case ds_ok: break;
			case ds_nomemory:
				log_printf(fp_log, "Line %zu: No enough memory.\n", LineNo);
				goto FailExit;
			case ds_alreadyexists:
				log_printf(fp_log, "Line %zu: '%s' already assigned.\n", LineNo, ch2);
				free(ch2);
				goto FailExit;
			default:
				log_printf(fp_log, "Line %zu: dictionary error.\n", LineNo);
				free(ch2);
				goto FailExit;
			}
			break;
		}
	}

	fclose(fp);

	return d_cfg;
FailExit:
	dict_delete(d_cfg);
	if (fp) fclose(fp);
	return NULL;
}

dict_p dictcfg_section(dict_p cfg, const char *name)
{
	return dict_search(cfg, name);
}

int dictcfg_getint(dict_p section, const char *key, int def)
{
	char *val = dict_search(section, key);
	if (!val) return def;
	return atoi(val);
}

char *dictcfg_getstr(dict_p section, const char *key, char *def)
{
	char *val = dict_search(section, key);
	if (!val) return def;
	return val;
}

double dictcfg_getfloat(dict_p section, const char *key, double def)
{
	char *val = dict_search(section, key);
	if (!val) return def;
	return atof(val);
}

int dictcfg_getbool(dict_p section, const char *key, int def)
{
	char *val = dict_search(section, key);
	if (!val) return def;
	if (!stricmp(val, "yes")) return 1;
	if (!stricmp(val, "true")) return 1;
	if (!stricmp(val, "no")) return 0;
	if (!stricmp(val, "false")) return 0;
	return -1;
}
