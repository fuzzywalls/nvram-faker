#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nvram-faker.h"
//include before ini.h to override ini.h defaults
#include "nvram-faker-internal.h"
#include "ini.h"

#define RED_ON "\033[22;31m"
#define RED_OFF "\033[22;00m"
#define DEFAULT_KV_PAIR_LEN 1024

static int kv_count=0;
static int key_value_pair_len=DEFAULT_KV_PAIR_LEN;
static char **key_value_pairs=NULL;
static char *EMPTY_STR = "";

static int extend_list(void **user)
{
    char **new_kv = NULL;
    int old_kv_len = 0;
    int i = 0;
    char **kv;

    kv = **((char ****)user);
    if(NULL == kv)
    {
        LOG_PRINTF("kv is NULL\n");
        return 0;
    }

    old_kv_len=key_value_pair_len;
    key_value_pair_len=(key_value_pair_len * 2);
    new_kv=(char **)malloc(key_value_pair_len * sizeof(char **));
    if(NULL == *kv)
    {
        LOG_PRINTF("Failed to reallocate key value array.\n");
        return 0;
    }
    for(i=0;i<old_kv_len;i++)
    {
        new_kv[i] = (kv)[i];
    }
    free(**(char ****)user);
    kv=new_kv;
    **(char ****)user=kv;
    return 0;
}

static int ini_handler(void *user, const char *section, const char *name,const char *value)
{
    if(NULL == user || NULL == section || NULL == name || NULL == value)
    {
        DEBUG_PRINTF("bad parameter to ini_handler\n");
        return 0;
    }

    DEBUG_PRINTF("kv_count: %d, key_value_pair_len: %d\n", kv_count,key_value_pair_len);
    if(kv_count >= key_value_pair_len)
    {
        extend_list(&user);
    }
    DEBUG_PRINTF("Got %s:%s\n",name,value);
    (*((char ***)user))[kv_count++]=strdup(name);
    (*((char ***)user))[kv_count++]=strdup(value);
    
    return 1;
}

void initialize_ini(void)
{
    int ret;
    DEBUG_PRINTF("Initializing.\n");
    if (NULL == key_value_pairs)
    {
        key_value_pairs=malloc(key_value_pair_len * sizeof(char **));
    }
    if(NULL == key_value_pairs)
    {
        LOG_PRINTF("Failed to allocate memory for key value array. Terminating.\n");
        exit(1);
    }
    
    ret = ini_parse(INI_FILE_PATH,ini_handler,(void *)&key_value_pairs);
    if (0 != ret)
    {
        LOG_PRINTF("ret from ini_parse was: %d\n",ret);
        LOG_PRINTF("INI parse failed. Terminating\n");
        free(key_value_pairs);
        key_value_pairs=NULL;
        exit(1);
    }else
    {
        DEBUG_PRINTF("ret from ini_parse was: %d\n",ret);
    }
    
    return;
    
}

void end(void)
{
    int i;
    for (i=0;i<kv_count;i++)
    {
        free(key_value_pairs[i]);
    }
    free(key_value_pairs);
    key_value_pairs=NULL;
    
    return;
}

void nvram_init(void *fp)
{
    LOG_PRINTF("Init called.\n");
    return;
}

void nvram_commit(void *unused, void *unused2)
{
    LOG_PRINTF("Commit called.\n");
    return;
}

void nvram_clear(void *unused)
{
    LOG_PRINTF("Clear called.\n");
}

void nvram_close(void *unused)
{
    LOG_PRINTF("Close called.\n");
    return;
}

char *get(const char *key)
{
    int i;
    int found=0;
    char *value;
    char *ret;
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            LOG_PRINTF("%s=%s\n",key,key_value_pairs[i+1]);
            found = 1;
            value=key_value_pairs[i+1];
            break;
        }
    }

    ret = NULL;
    if(!found)
    {
            LOG_PRINTF( RED_ON"%s=Unknown\n"RED_OFF,key);
            return EMPTY_STR;
    }else
    {

            ret=strdup(value);
    }
    return ret;
}

char *nvram_get(const void *nothing, const char *key)
//char *nvram_get(const char *key)
{
    return get(key);
}


char *nvram_bufget(const void *nothing, const char *key)
{
    return get(key);
}

int set(const char *key, const char *value)
{
    int i;
    int found=0;

    if(NULL == key)
    {
        LOG_PRINTF("Set called with NULL key.\n");
    }

    if (NULL == value)
    {
        LOG_PRINTF("Set called with NULL value for %s.\n",key);
    }

    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            LOG_PRINTF("Changing %s=%s to ",key,key_value_pairs[i+1]);
            found = 1;
            free(key_value_pairs[i+1]);
            key_value_pairs[i+1] = strdup(value);
            LOG_PRINTF("%s=%s\n",key,key_value_pairs[i+1]);
            break;
        }
    }

    if(!found)
    {
        if(kv_count >= key_value_pair_len)
        {
            extend_list((void **)&key_value_pairs);
        }
        key_value_pairs[kv_count++] = strdup(key);
        key_value_pairs[kv_count++] = strdup(value);
        LOG_PRINTF("Setting %s=%s\n",key_value_pairs[i],key_value_pairs[i+1]);
    }

    return 0;
}

//int nvram_set(const void *unused, const char *key, const char *value)
int nvram_set(const char *key, const char *value)
{
    return set(key, value);
}

int nvram_bufset(const void *unused, const char *key, const char *value)
{
    return set(key, value);
}

int getNvramIndex(const char *key)
{
    // Looking for an NVRAM index, anything other than -1 is acceptable.
    // Most times this value is passed to init which is ignored anyway.
    return 1;
}

int getNvramNum()
{
    // Return value specific to the current router I'm looking at.
    return 4;
}

char *getNvramName(const int index)
{
    if(0 > index)
    {
        LOG_PRINTF("Invalid index provided.\n");
        return NULL;
    }

    if(index > kv_count)
    {
        LOG_PRINTF("Index %d out of range\n", index);
        return NULL;
    }

    return key_value_pairs[index];
}
