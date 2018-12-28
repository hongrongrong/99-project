#include "msstd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <dirent.h> 
#include <unistd.h>
#include <fcntl.h>

static int is_dir_existed(const char* dir)
{
	struct stat buf = {0};
	if (!dir) 
		return 0;
	
	if (lstat(dir, &buf) < 0)  
		return 0;
	
	if (!S_ISDIR(buf.st_mode)) 
		return 0;
	
	return 1;	
}

int ms_create_dir(const char* dir)
{
	char buffer[256] = {0};
	if (!dir) 
		return -1;
	
	snprintf(buffer, sizeof(buffer), "%s", dir);
	char* p = NULL;
	char* s = NULL;
	int   c = 1;
	p = buffer + 1;
	s = buffer;
	
	while (c) 
	{
		p = strchr(p, '/');
		if (p) 
			*p = 0;
		else   
			c = 0; 
		if (!is_dir_existed(s))
		{ 
			if (mkdir(s, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) 
				return -1;
		}
		if (c) 
			*p++ = '/'; 
	}
	
	return 0;	
}

int ms_remove_dir(const char* dir)
{
	if (!dir)
		return -1;
	char cmd[128] = {0};
#if 0 //add by mjx.milesight 2014.12.16
	ms_system("mkdir /tmp/rsync_blank");
	snprintf(cmd, sizeof(cmd), "rsync --delete-before -a -H /tmp/rsync_blank/ %s", dir);
	printf("\n======== mjx test ms_remove_dir:%s begin==============", cmd);
	ms_system(cmd);
	printf("\n======== mjx test ms_remove_dir:%s end==============", cmd);
	ms_system("rm -rf /tmp/rsync_blank");
#else
	snprintf(cmd, sizeof(cmd), "/bin/rm -rf %s", dir);
	ms_system(cmd);
#endif
	
	return 0;
}
int ms_file_existed(const char* file)
{
	if (!file) 
		return 0;
	
	return !access(file, F_OK);	
}

/*int ms_read_conf(const char* file, const char* key, char value[128])
{
	if (!file || !key || !value) return -1;
	FILE* fp = fopen(file, "rb");
	if (!fp) return -1;
	int result = -1;
	while(1){
		char data[256] = {0};
		if (!fgets(data, sizeof(data), fp)) break;
		ms_string_strip(data, ' ');
		ms_string_strip(data, '\n');
		char keyinfo[64] = {0};
		snprintf(keyinfo, sizeof(keyinfo), "%s=", key);
		if (strncmp(data, keyinfo, strlen(keyinfo)) != 0) continue;
		snprintf(value, 128, "%s", data + strlen(keyinfo));
		result = 0;
		break;
	}
	fclose(fp);
	return result;
}*/

//return value: mem size (KBytes)
#if 0
unsigned int ms_get_mem(int type)
{
	int fd_mem = -1;
	char buffer[256];
	char tmp_buf[64];
	char *pStr = NULL;
	unsigned int freeInKb = 0;
	char delima_buf[] = ":\r\n ";
	char cmp_str[64] = {0};
	
	fd_mem = open("/proc/meminfo", O_RDONLY);
	if(fd_mem < 0)
	{
		goto MEM_END;
	}

	if(read(fd_mem, buffer, sizeof(buffer)-1) <= 0)
	{
		printf("read device error !!");
		goto MEM_END;
	}

	buffer[sizeof(buffer)-1] = '\0';
	pStr = strtok(buffer, delima_buf );
	if(type == MEM_TYPE_FREE)
		snprintf(cmp_str, sizeof(cmp_str), "MemFree");
	else if(type == MEM_TYPE_TOTAL)
		snprintf(cmp_str, sizeof(cmp_str), "MemTotal");

	while(pStr != NULL)
	{
		sscanf(pStr, "%s", tmp_buf);
		if(strncmp(tmp_buf, cmp_str, strlen(cmp_str)) == 0)
		{
			pStr = strtok(NULL, delima_buf);
			sscanf(pStr,"%u", &freeInKb);
			goto MEM_END;
		}

		pStr = strtok(NULL, delima_buf);
	}

MEM_END:

	if( fd_mem >= 0 )
		close(fd_mem);

	return freeInKb;
}
#else
unsigned int ms_get_mem(int type)
{
    FILE *pfd = NULL;
 	unsigned int freeInKb = 0;
 	unsigned int totalInKb = 0;
    char tmp[128];
    
    pfd = ms_vpopen("cat /proc/meminfo", "r");
    if (pfd)
    {
        while (fgets(tmp, sizeof(tmp), pfd) != NULL )
        {
            if(type == MEM_TYPE_FREE)
            {
                if (strstr(tmp, "MemFree"))
                {
                    sscanf(tmp, "%*[^0-9]%u", &freeInKb);
                    ms_vpclose(pfd);
                    return freeInKb;
                }

            }
                
            else if(type == MEM_TYPE_TOTAL)
            {
                if (strstr(tmp, "MemTotal"))
                {
                    sscanf(tmp, "%*[^0-9]%u", &totalInKb);
                    ms_vpclose(pfd);
                    return totalInKb;
                }

            }
            else if(type == MEM_TYPE_PERCENT)
            {
                if (strstr(tmp, "MemFree"))
                {
                    sscanf(tmp, "%*[^0-9]%u", &freeInKb);
                }
                if (strstr(tmp, "MemTotal"))
                {
                    sscanf(tmp, "%*[^0-9]%u", &totalInKb);
                }

                if (freeInKb == 0 || totalInKb == 0)
                    continue;
                ms_vpclose(pfd);
                return (freeInKb * 100 / (totalInKb+1));
            }

        }
        ms_vpclose(pfd);
        return 0;
    }
}
#endif

unsigned long ms_get_file_size(const char *path)
{
	unsigned long filesize = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

