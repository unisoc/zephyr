#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include "uki_utlis.h"
#include "uki_config.h"

#include <fs.h>
#include <ff.h>
#define FATFS_MNTP "/NAND:"
/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
    .type = FS_FATFS,
    .mnt_point = FATFS_MNTP,
    .fs_data = &fat_fs,
};

char *uki_strtok_r(char *str, const char *delim, char **saveptr)
{
  char *pbegin;
  char *pend = NULL;

  /* Decide if we are starting a new string or continuing from
   * the point we left off.
   */

  if (str)
    {
      pbegin = str;
    }
  else if (saveptr && *saveptr)
    {
      pbegin = *saveptr;
    }
  else
    {
      return NULL;
    }

  /* Find the beginning of the next token */

  for (;
       *pbegin && strchr(delim, *pbegin) != NULL;
       pbegin++);

  /* If we are at the end of the string with nothing
   * but delimiters found, then return NULL.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  /* Find the end of the token */

  for (pend = pbegin + 1;
       *pend && strchr(delim, *pend) == NULL;
       pend++);


  /* pend either points to the end of the string or to
   * the first delimiter after the string.
   */

  if (*pend)
    {
      /* Turn the delimiter into a null terminator */

      *pend++ = '\0';
    }

  /* Save the pointer where we left off and return the
   * beginning of the token.
   */

  if (saveptr)
    {
      *saveptr = pend;
    }
  return pbegin;
}

char *uki_fgets(char *dst, int max, struct fs_file_t *file)
{
    char *p;
    char  ch;
    int len = 0;
    /* get max bytes or upto a newline */

    for (p = dst, max--; max > 0; max--) {
        len = fs_read(file, &ch, 1);
        if (len <= 0)
            break;
        *p++ = ch;
        if (ch == '\n')
            break;
    }
    *p = 0;
    if (p == dst || len <= 0)
        return NULL;
    return (p);
}

static void parse_number(char *p_conf_name, char *p_conf_value, void *buf,
                         int len, int size)
{
    uint8_t *dest = (uint8_t *)buf;
    char *sub_value, *p;
    uint32_t value;
    ARG_UNUSED(p_conf_name);
    sub_value = uki_strtok_r(p_conf_value, CONF_VALUES_PARTITION, &p);
    do {
        if (sub_value == NULL) {
            break;
        }

        if (sub_value[0] == '0' && (sub_value[1] == 'x' || sub_value[1] == 'X')) {
            value = strtoul(sub_value, 0, 16) & 0xFFFFFFFF;
        } else {
            value = strtoul(sub_value, 0, 10) & 0xFFFFFFFF;
        }

        switch (size) {
        case sizeof(uint8_t):
            *dest = value & 0xFF;
            dest += size;
            break;

        case sizeof(uint16_t):
            *((uint16_t *)dest) = value & 0xFFFF;
            dest += size;
            break;

        case sizeof(uint32_t):
            *((uint32_t *)dest) = value & 0xFFFFFFFF;
            dest += size;
            break;

        default:
            break;
        }
        sub_value = uki_strtok_r(NULL, CONF_VALUES_PARTITION, &p);
    } while (--len);
}

void uki_config_init(void)
{
	int ret = fs_mount(&fatfs_mnt);
	if (ret < 0 && ret!=-EBUSY) {
		BTE("Error mounting fs [%d]\n", ret);
		return -1;
	}
}

void vnd_load_configure(const char *p_path, const conf_entry_t *entry, unsigned int mask)
{
    struct fs_file_t p_file;
    char *p_name, *p_value, *p;
    conf_entry_t *p_entry;
    char line[CONF_MAX_LINE_LEN + 1]; /* add 1 for \0 char */

	if (mask)
	    BTI("config: Load conf %s\n", p_path);

    if (0 == fs_open(&p_file, p_path)) {
        /* read line by line */
        while (uki_fgets(line, CONF_MAX_LINE_LEN + 1, &p_file) != NULL) {
            if (line[0] == CONF_COMMENT) continue;

            p_name = uki_strtok_r(line, CONF_DELIMITERS, &p);

            if (NULL == p_name) {
                continue;
            }

            p_value = uki_strtok_r(NULL, CONF_VALUES_DELIMITERS, &p);

            if (NULL == p_value) {
					BTE("%s, vnd_load_conf: missing value for name: %s\n", __func__, p_name);
                continue;
            }

            p_entry = (conf_entry_t*)entry;

            while (p_entry->conf_entry != NULL) {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0) {
                    if (p_entry->p_action) {
                        p_entry->p_action(p_name, p_value, p_entry->buf, p_entry->len,
                                          p_entry->size);
                    } else {
						if (mask)
	                        BTI("config: %s -> %s\n", p_name, p_value);
                        parse_number(p_name, p_value, p_entry->buf, p_entry->len,
                                    p_entry->size);
                    }
                    break;
                }

                p_entry++;
            }
        }

        fs_close(&p_file);
    } else {
        BTE("%s, vnd_load_conf file >%s< not found\n", __func__, p_path);
    }
}

