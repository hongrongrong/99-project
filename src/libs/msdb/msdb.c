#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "ccsqlite.h"
#include "msdefs.h"
#include "msdb.h"
#include "msstd.h"
#include <stddef.h>

#define FILE_LOCK_NAME "/tmp/msdb.fl"
#define FILE_TZ_NAME "/tmp/tzdb.fl"
hook_print global_debug_hook = 0;
unsigned int global_debug_mask = 0;
static pthread_rwlock_t global_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static int translate_pwd(char *dst, const char *src, int len)
{
    int a = 0, b = 0;

    while (b < len)
	{
        if (src[b] == '\'')
		{
			dst[a++] = '\'';
			dst[a++] = '\'';
			b++;
        }
        else
		{
			dst[a++] = src[b++];
		}
    }
    dst[a++] = '\0';
    return (a);
}

int FileLock(const char *pFilePath, int mode, pthread_rwlock_t *lock)
{
	if(!pFilePath){
		system("echo \"===FileLock Faild===00===\" >> /mnt/nand/err_log.txt");
		return -1;
	}
	int nFd = open(pFilePath, O_CREAT|O_RDWR);
	if(-1 == nFd){
		system("echo \"===FileLock Faild===11===\" >> /mnt/nand/err_log.txt");
		return -1;
	}
	if (mode == 'r')
		pthread_rwlock_rdlock(lock);
	else
		pthread_rwlock_wrlock(lock);
	lockf(nFd, F_LOCK, 0);
	return nFd;
}

void FileUnlock(int nFd, pthread_rwlock_t *lock)
{
	if(nFd <= -1)
		return ;
	pthread_rwlock_unlock(lock);
	lockf(nFd, F_ULOCK, 0);
	close(nFd);
}

int db_run_sql(const char *db_file, const char *sql)
{
	if(!db_file || !sql)
		return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(db_file, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	sqlite_execute(hConn, sql);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int db_query_sql(const char *db_file, const char *sql, char *str_result, int size)
{
	if(!db_file || !sql || !str_result || size <= 0)
		return -1;
	int i;
	char value[256] = {0};
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(db_file, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}	
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	int nResult = sqlite_query_record(hConn, sql, &hStmt, &nColumnCnt);
	if(nResult != 0 || nColumnCnt==0)
	{
	    if(hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	snprintf(str_result+strlen(str_result), size-strlen(str_result), "\n");
	for(i = 0; i < nColumnCnt; i++)
	{
		const char *name = sqlite_query_column_name(hConn, hStmt, i);
		if(name)
			snprintf(str_result+strlen(str_result), size-strlen(str_result), "%-16s", name);
	}
	do
	{
		//msprintf("nColumnCnt:%d", nColumnCnt);
		snprintf(str_result+strlen(str_result), size-strlen(str_result), "\n");
		for(i = 0; i < nColumnCnt; i++)
		{
			sqlite_query_column_text(hStmt, i, value, sizeof(value));
			if(i == nColumnCnt-1)
				snprintf(str_result+strlen(str_result), size-strlen(str_result), "%s", value);
			else
				snprintf(str_result+strlen(str_result), size-strlen(str_result), "%-16s", value);
		}
	}while (sqlite_query_next(hStmt) == 0);
	snprintf(str_result+strlen(str_result), size-strlen(str_result), "\n");

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
int read_osd(const char *pDbFile, struct osd *osd, int chn_id)
{
	if(!pDbFile || !osd || chn_id<0) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[512] = {0};
    int nColumnCnt = 0;
	snprintf(sQuery, sizeof(sQuery), "select id, name, show_name, show_date, show_week, date_format, time_format, date_pos_x,date_pos_y,name_pos_x,name_pos_y,alpha from osd where id=%d;", chn_id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0 || nColumnCnt==0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &osd->id);
    sqlite_query_column_text(hStmt, 1, osd->name, sizeof(osd->name));
    sqlite_query_column(hStmt, 2, &osd->show_name);
    sqlite_query_column(hStmt, 3, &osd->show_date);
    sqlite_query_column(hStmt, 4, &osd->show_week);
    sqlite_query_column(hStmt, 5, &osd->date_format);
    sqlite_query_column(hStmt, 6, &osd->time_format);
    sqlite_query_column_double(hStmt, 7, &osd->date_pos_x);
    sqlite_query_column_double(hStmt, 8, &osd->date_pos_y);
    sqlite_query_column_double(hStmt, 9, &osd->name_pos_x);
    sqlite_query_column_double(hStmt, 10, &osd->name_pos_y);
    sqlite_query_column(hStmt, 11, &osd->alpha);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_osd_name(const char *pDbFile, char *name, int chn_id)
{
	if(!pDbFile || !name || chn_id<0) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[512] = {0};
    int nColumnCnt = 0;
	snprintf(sQuery, sizeof(sQuery), "select name from osd where id=%d;", chn_id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0 || nColumnCnt==0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column_text(hStmt, 0, name, 64);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_osd(const char *pDbFile, struct osd *osd)
{
	if(!pDbFile || !osd) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	//david.milesight
	char osdNameStr[64] = {0};
	char osdNameSqlStr[128] = {0};
	int i = 0 , j=0;
	snprintf(osdNameStr,sizeof(osdNameStr),"%s",osd->name);
	for(i =0 ;i<strlen(osdNameStr);i++)
	{
		if(osdNameStr[i] == '\'')
		{
			osdNameSqlStr[j++] = '\'';
			osdNameSqlStr[j++] = '\'';
		}
		else
		{
			osdNameSqlStr[j++] = osdNameStr[i];
		}
	}
	osdNameSqlStr[j] = '\0';	
	snprintf(sExec, sizeof(sExec), "update osd set name = '%s', show_name=%d, show_date=%d,show_week=%d,date_format=%d,time_format=%d,date_pos_x=%f,date_pos_y=%f,name_pos_x=%f,name_pos_y=%f,alpha=%d where id=%d;", osdNameSqlStr /*osd->name*/, osd->show_name, osd->show_date, osd->show_week, osd->date_format, osd->time_format, osd->date_pos_x, osd->date_pos_y, osd->name_pos_x, osd->name_pos_y, osd->alpha, osd->id);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int insert_osd(const char *pDbFile, struct osd *osd)
{
	if(!pDbFile || !osd) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[1024] = {0};
	snprintf(sExec, sizeof(sExec), "insert into osd(id,name,show_name,show_date,show_week,date_format,time_format,date_pos_x,date_pos_y,name_pos_x,name_pos_y,alpha) values(%d,'%s',%d,%d,%d,%d,%d,%f,%f,%f,%f,%d);", osd->id, osd->name, osd->show_name, osd->show_date, osd->show_week, osd->date_format, osd->time_format, osd->date_pos_x, osd->date_pos_y, osd->name_pos_x, osd->name_pos_y, osd->alpha);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


int read_osds(const char *pDbFile, struct osd osds[], int *pCount)
{
	if(!pDbFile || !osds) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id, name, show_name, show_date, show_week, date_format, time_format, date_pos_x,date_pos_y,name_pos_x,name_pos_y,alpha from osd order by(id);");
	int i = 0;
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0 || nColumnCnt==0)
	{
		*pCount = i;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	for(i=0; i<MAX_CAMERA; i++)
	{
        sqlite_query_column(hStmt, 0, &osds[i].id);
        sqlite_query_column_text(hStmt, 1, osds[i].name, sizeof(osds[i].name));
        sqlite_query_column(hStmt, 2, &osds[i].show_name);
        sqlite_query_column(hStmt, 3, &osds[i].show_date);
        sqlite_query_column(hStmt, 4, &osds[i].show_week);
        sqlite_query_column(hStmt, 5, &osds[i].date_format);
        sqlite_query_column(hStmt, 6, &osds[i].time_format);
        sqlite_query_column_double(hStmt, 7, &osds[i].date_pos_x);
        sqlite_query_column_double(hStmt, 8, &osds[i].date_pos_y);
        sqlite_query_column_double(hStmt, 9, &osds[i].name_pos_x);
        sqlite_query_column_double(hStmt, 10, &osds[i].name_pos_y);
        sqlite_query_column(hStmt, 11, &osds[i].alpha);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
	}
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	*pCount = i;
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_motion(const char *pDbFile, struct motion *move, int id)
{
    if(!pDbFile || !move) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,sensitivity,tri_channels,tri_alarms, motion_table,buzzer_interval,ipc_sched_enable,tri_channels_ex,email_enable,email_buzzer_interval from motion where id=%d;", id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &move->id);
    sqlite_query_column(hStmt, 1, &move->enable);
    sqlite_query_column(hStmt, 2, &move->sensitivity);
    sqlite_query_column(hStmt, 3, (int*)&move->tri_channels);
    sqlite_query_column(hStmt, 4, (int*)&move->tri_alarms);
    sqlite_query_column_text(hStmt, 5, (char *)move->motion_table, sizeof(move->motion_table));
    sqlite_query_column(hStmt, 6, &move->buzzer_interval);
    sqlite_query_column(hStmt, 7, &move->ipc_sched_enable);
	sqlite_query_column_text(hStmt, 8, move->tri_channels_ex, sizeof(move->tri_channels_ex));
	sqlite_query_column(hStmt, 9, &move->email_enable);
	sqlite_query_column(hStmt, 10, &move->email_buzzer_interval);
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_motions(const char *pDbFile, struct motion moves[], int *pCnt)
{
    if(!pDbFile || !moves) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,sensitivity,tri_channels,tri_alarms, motion_table, buzzer_interval,ipc_sched_enable,tri_channels_ex,email_enable,email_buzzer_interval from motion;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &moves[i].id);
        sqlite_query_column(hStmt, 1, &moves[i].enable);
        sqlite_query_column(hStmt, 2, &moves[i].sensitivity);
        sqlite_query_column(hStmt, 3, (int*)&moves[i].tri_channels);
        sqlite_query_column(hStmt, 4, (int*)&moves[i].tri_alarms);
        sqlite_query_column_text(hStmt, 5, (char *)moves[i].motion_table, sizeof(moves[i].motion_table));
        sqlite_query_column(hStmt, 6, &moves[i].buzzer_interval);
        sqlite_query_column(hStmt, 7, &moves[i].ipc_sched_enable);
		sqlite_query_column_text(hStmt, 8, moves[i].tri_channels_ex, sizeof(moves[i].tri_channels_ex));
		sqlite_query_column(hStmt, 9, &moves[i].email_enable);
		sqlite_query_column(hStmt, 10, &moves[i].email_buzzer_interval);
		
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_motion(const char *pDbFile, struct motion *move)
{
    if(!pDbFile || !move) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    move->motion_table[sizeof(move->motion_table) - 1] = '\0';
    snprintf(sExec, sizeof(sExec), "insert into motion(id,enable,sensitivity,tri_channels,tri_alarms,motion_table,buzzer_interval,ipc_sched_enable,tri_channels_ex,email_enable,email_buzzer_interval) values(%d,%d,%d,%d,%d,'%s',%d,%d,'%s',%d,%d);",
    		move->id, move->enable, move->sensitivity, move->tri_channels, move->tri_alarms, move->motion_table, move->buzzer_interval, move->ipc_sched_enable,move->tri_channels_ex,move->email_enable, move->email_buzzer_interval);
//    printf("......write_motion: %s\n", sExec);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion(const char *pDbFile, struct motion *move)
{
    if(!pDbFile || !move) return -1;
	//printf("move->tri_channels_ex:%s,sizeof(move->motion_table):%d\n",move->tri_channels_ex,sizeof(move->motion_table));
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    move->motion_table[sizeof(move->motion_table) - 1] = '\0';
    snprintf(sExec, sizeof(sExec), "update motion set enable=%d, sensitivity=%d, tri_channels=%u, tri_alarms=%d, motion_table='%s',buzzer_interval=%d,ipc_sched_enable=%d,tri_channels_ex='%s',email_enable=%d,email_buzzer_interval=%d where id=%d;",
    		move->enable, move->sensitivity, move->tri_channels, move->tri_alarms, move->motion_table, move->buzzer_interval, move->ipc_sched_enable,move->tri_channels_ex, move->email_enable, move->email_buzzer_interval, move->id);
    //printf("......write_motion: %s\n", sExec);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_motion_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id)
{
    if(!pDbFile || !motionSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from motion_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int delete_motion_schedule(const char *pDbFile, int chn_id)
{
    if(!pDbFile || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into motion_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_motion_audible_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id)
{
    if(!pDbFile || !motionSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from motion_audible_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int delete_motion_audible_schedule(const char *pDbFile, int chn_id)
{
    if(!pDbFile || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_audible_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion_audible_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_audible_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into motion_audible_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_out(const char *pDbFile, struct alarm_out *alarm, int alarm_id)
{
    if(!pDbFile || !alarm || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn)!=0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    int nColumnCnt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select id,name,type,enable,duration_time from alarm_out where id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt ==0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    sqlite_query_column(hStmt, 0, &alarm->id);
    sqlite_query_column_text(hStmt, 1, alarm->name, sizeof(alarm->name));
    sqlite_query_column(hStmt, 2, &alarm->type);
    sqlite_query_column(hStmt, 3, &alarm->enable);
    sqlite_query_column(hStmt, 4, &alarm->duration_time);
//    sqlite_query_column(hStmt, 5, &alarm->buzzer_interval);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_outs(const char *pDbFile, struct alarm_out alarms[], int *pCount)
{
    if(!pDbFile || !alarms) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn)!=0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    int nColumnCnt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select id,name,type,enable,duration_time from alarm_out order by(id);");
    int i = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt==0)
    {
        *pCount = i;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    for(i=0; i<MAX_ALARM_OUT; i++)
    {
        sqlite_query_column(hStmt, 0, &alarms[i].id);
        sqlite_query_column_text(hStmt, 1, alarms[i].name, sizeof(alarms[i].name));
        sqlite_query_column(hStmt, 2, &alarms[i].type);
        sqlite_query_column(hStmt, 3, &alarms[i].enable);
        sqlite_query_column(hStmt, 4, &alarms[i].duration_time);
//        sqlite_query_column(hStmt, 4, &alarms[i].buzzer_interval);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    *pCount = i;
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarm_out(const char *pDbFile, struct alarm_out *alarm)
{
    if(!pDbFile || !alarm) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update alarm_out set name='%s',type=%d,enable=%d,duration_time=%d where id=%d;", alarm->name, alarm->type, alarm->enable, alarm->duration_time, alarm->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_out_schedule(const char *pDbFile, struct alarm_out_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from alarm_out_schedule where alarm_id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 0;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarm_out_schedule(const char *pDbFile, struct alarm_out_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from alarm_out_schedule where alarm_id=%d;", alarm_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into alarm_out_schedule values(%d,%d,%d,'%s','%s',%d);", alarm_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_video_lost(const char *pDbFile, struct video_loss *videolost, int chn_id)
{
    if(!pDbFile || !videolost) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,buzzer_interval,email_enable,email_buzzer_interval from video_loss where id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &videolost->id);
    sqlite_query_column(hStmt, 1, &videolost->enable);
    sqlite_query_column(hStmt, 2, (int*)&videolost->tri_alarms);
    sqlite_query_column(hStmt, 3, &videolost->buzzer_interval);
	sqlite_query_column(hStmt, 4, (int*)&videolost->email_enable);
    sqlite_query_column(hStmt, 5, &videolost->email_buzzer_interval);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_video_losts(const char *pDbFile, struct video_loss videolosts[], int *pCnt)
{
    if(!pDbFile || !videolosts) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,buzzer_interval,email_enable,email_buzzer_interval from video_loss;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &videolosts[i].id);
        sqlite_query_column(hStmt, 1, &videolosts[i].enable);
        sqlite_query_column(hStmt, 2, (int*)&videolosts[i].tri_alarms);
        sqlite_query_column(hStmt, 3, &videolosts[i].buzzer_interval);
		sqlite_query_column(hStmt, 4, (int*)&videolosts[i].email_enable);
        sqlite_query_column(hStmt, 5, &videolosts[i].email_buzzer_interval);
        if(sqlite_query_next(hStmt) !=0 ){i++; break;}
    }

    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_video_lost(const char *pDbFile, struct video_loss *videolost)
{
    if(!pDbFile || !videolost) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update video_loss set enable=%d,tri_alarms=%d,buzzer_interval=%d,email_enable=%d,email_buzzer_interval=%d where id=%d;",
    		videolost->enable, videolost->tri_alarms,videolost->buzzer_interval, videolost->email_enable, videolost->email_buzzer_interval, videolost->id);

    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_video_lost(const char *pDbFile, struct video_loss *videolost)
{
    if(!pDbFile || !videolost) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "insert into video_loss(id,enable,tri_alarms,buzzer_interval,email_enable,email_buzzer_interval) values(%d,%d,%d,%d,%d,%d);",
    		videolost->id, videolost->enable, videolost->tri_alarms,videolost->buzzer_interval, videolost->email_enable, videolost->email_buzzer_interval);

    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_video_loss_schedule(const char *pDbFile, struct video_loss_schedule  *videolostSchedule, int chn_id)
{
    if(!pDbFile || !videolostSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from video_loss_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_video_loss_schedule(const char *pDbFile, struct video_loss_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from video_loss_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into video_loss_schedule values(%d,%d,%d,'%s','%s',%d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_videoloss_audible_schedule(const char *pDbFile, struct video_loss_schedule  *videolostSchedule, int chn_id)
{
    if(!pDbFile || !videolostSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from videoloss_audible_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(videolostSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_videoloss_audible_schedule(const char *pDbFile, struct video_loss_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from videoloss_audible_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into videoloss_audible_schedule values(%d,%d,%d,'%s','%s',%d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int disable_ip_camera(const char *pDbFile, int nCamId)
{
    if(!pDbFile || nCamId<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update camera set enable=0 where id=%d;", nCamId);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int clear_cameras(const char *pDbFile)
{
    if(!pDbFile) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    snprintf(sExec, sizeof(sExec), "update camera set type=1,enable=0,covert_on=0,brightness=0,contrast=0,saturation=0,username='',password='',ip_addr='',main_rtsp_port=0,main_source_path='',sub_rtsp_enable=0,sub_rtsp_port=0,sub_source_path='',manage_port=0,camera_protocol=0,transmit_protocol=0,play_stream=0,record_stream=0,sync_time=0,main_rtspurl='',sub_rtspurl='',poe_channel=0,physical_port=0,mac='';");
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_cameras_type(const char *pDbFile, int types[])
{
    if(!pDbFile || !types) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select type from camera;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &types[i]);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_camera(const char *pDbFile, struct camera *cam, int chn_id)
{
    if(!pDbFile || !cam) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[2048] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,type,enable,covert_on,brightness,contrast,saturation, ip_addr,manage_port,camera_protocol,play_stream,record_stream,username,password,transmit_protocol,main_rtsp_port,main_source_path,sub_rtsp_enable,sub_rtsp_port,sub_source_path,sync_time,codec,main_rtspurl,sub_rtspurl,poe_channel,physical_port,mac from camera where id = %d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &cam->id);
    sqlite_query_column(hStmt, 1, &cam->type);
    sqlite_query_column(hStmt, 2, &cam->enable);
    sqlite_query_column(hStmt, 3, &cam->covert_on);
    sqlite_query_column(hStmt, 4, &cam->brightness);
    sqlite_query_column(hStmt, 5, &cam->contrast);
    sqlite_query_column(hStmt, 6, &cam->saturation);

    sqlite_query_column_text(hStmt, 7, cam->ip_addr, sizeof(cam->ip_addr));
    sqlite_query_column(hStmt, 8, &cam->manage_port);
    sqlite_query_column(hStmt, 9, &cam->camera_protocol);
    sqlite_query_column(hStmt, 10, &cam->play_stream);
    sqlite_query_column(hStmt, 11, &cam->record_stream);
    sqlite_query_column_text(hStmt, 12, cam->username, sizeof(cam->username));
    sqlite_query_column_text(hStmt, 13, cam->password, sizeof(cam->password));
    sqlite_query_column(hStmt, 14, &cam->transmit_protocol);
    sqlite_query_column(hStmt, 15, &cam->main_rtsp_port);
    sqlite_query_column_text(hStmt, 16, cam->main_source_path, sizeof(cam->main_source_path));
    sqlite_query_column(hStmt, 17, &cam->sub_rtsp_enable);
    sqlite_query_column(hStmt, 18, &cam->sub_rtsp_port);
    sqlite_query_column_text(hStmt, 19, cam->sub_source_path, sizeof(cam->sub_source_path));
    sqlite_query_column(hStmt, 20, &cam->sync_time);
	sqlite_query_column(hStmt, 21, &cam->codec);
	sqlite_query_column_text(hStmt, 22, cam->main_rtspurl, sizeof(cam->main_rtspurl));
	sqlite_query_column_text(hStmt, 23, cam->sub_rtspurl, sizeof(cam->sub_rtspurl));
	
    sqlite_query_column(hStmt, 24, &cam->poe_channel);
	sqlite_query_column(hStmt, 25, &cam->physical_port);
	sqlite_query_column_text(hStmt, 26, cam->mac_addr, sizeof(cam->mac_addr));
	//sqlite_query_column(hStmt, 27, &cam->connect_mode);
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_cameras(const char *pDbFile, struct camera cams[], int *cnt)
{
    if(!pDbFile || !cams) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[2048] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,type,enable,covert_on,brightness,contrast,saturation,ip_addr,manage_port,camera_protocol,play_stream,record_stream,username,password,transmit_protocol,main_rtsp_port,main_source_path,sub_rtsp_enable,sub_rtsp_port,sub_source_path,sync_time,codec,main_rtspurl,sub_rtspurl,poe_channel,physical_port,mac from camera;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *cnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &cams[i].id);
        sqlite_query_column(hStmt, 1, &cams[i].type);
        sqlite_query_column(hStmt, 2, &cams[i].enable);
        sqlite_query_column(hStmt, 3, &cams[i].covert_on);
        sqlite_query_column(hStmt, 4, &cams[i].brightness);
        sqlite_query_column(hStmt, 5, &cams[i].contrast);
        sqlite_query_column(hStmt, 6, &cams[i].saturation);

        sqlite_query_column_text(hStmt, 7, cams[i].ip_addr, sizeof(cams[i].ip_addr));
        sqlite_query_column(hStmt, 8, &cams[i].manage_port);
        sqlite_query_column(hStmt, 9, &cams[i].camera_protocol);
        sqlite_query_column(hStmt, 10, &cams[i].play_stream);
        sqlite_query_column(hStmt, 11, &cams[i].record_stream);
        sqlite_query_column_text(hStmt, 12, cams[i].username, sizeof(cams[i].username));
        sqlite_query_column_text(hStmt, 13, cams[i].password, sizeof(cams[i].password));
        sqlite_query_column(hStmt, 14, &cams[i].transmit_protocol);
        sqlite_query_column(hStmt, 15, &cams[i].main_rtsp_port);
        sqlite_query_column_text(hStmt, 16, cams[i].main_source_path, sizeof(cams[i].main_source_path));
        sqlite_query_column(hStmt, 17, &cams[i].sub_rtsp_enable);
        sqlite_query_column(hStmt, 18, &cams[i].sub_rtsp_port);
        sqlite_query_column_text(hStmt, 19, cams[i].sub_source_path, sizeof(cams[i].sub_source_path));
        sqlite_query_column(hStmt, 20, &cams[i].sync_time);
		sqlite_query_column(hStmt, 21, &cams[i].codec);
		sqlite_query_column_text(hStmt, 22, cams[i].main_rtspurl, sizeof(cams[i].main_rtspurl));
		sqlite_query_column_text(hStmt, 23, cams[i].sub_rtspurl, sizeof(cams[i].sub_rtspurl));	

		sqlite_query_column(hStmt, 24, &cams[i].poe_channel);
		sqlite_query_column(hStmt, 25, &cams[i].physical_port);
		sqlite_query_column_text(hStmt, 26, cams[i].mac_addr, sizeof(cams[i].mac_addr));
		//sqlite_query_column(hStmt, 27, &cams[i].connect_mode);
		
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *cnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_poe_cameras(const char *pDbFile, struct camera cams[], int *cnt)
{
    if(!pDbFile || !cams) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[2048] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,type,enable,covert_on,brightness,contrast,saturation,ip_addr,manage_port,camera_protocol,play_stream,record_stream,username,password,transmit_protocol,main_rtsp_port,main_source_path,sub_rtsp_enable,sub_rtsp_port,sub_source_path,sync_time,codec,main_rtspurl,sub_rtspurl,poe_channel,physical_port,mac from camera where poe_channel = 1;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *cnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<nResult && i < MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &cams[i].id);
        sqlite_query_column(hStmt, 1, &cams[i].type);
        sqlite_query_column(hStmt, 2, &cams[i].enable);
        sqlite_query_column(hStmt, 3, &cams[i].covert_on);
        sqlite_query_column(hStmt, 4, &cams[i].brightness);
        sqlite_query_column(hStmt, 5, &cams[i].contrast);
        sqlite_query_column(hStmt, 6, &cams[i].saturation);

        sqlite_query_column_text(hStmt, 7, cams[i].ip_addr, sizeof(cams[i].ip_addr));
        sqlite_query_column(hStmt, 8, &cams[i].manage_port);
        sqlite_query_column(hStmt, 9, &cams[i].camera_protocol);
        sqlite_query_column(hStmt, 10, &cams[i].play_stream);
        sqlite_query_column(hStmt, 11, &cams[i].record_stream);
        sqlite_query_column_text(hStmt, 12, cams[i].username, sizeof(cams[i].username));
        sqlite_query_column_text(hStmt, 13, cams[i].password, sizeof(cams[i].password));
        sqlite_query_column(hStmt, 14, &cams[i].transmit_protocol);
        sqlite_query_column(hStmt, 15, &cams[i].main_rtsp_port);
        sqlite_query_column_text(hStmt, 16, cams[i].main_source_path, sizeof(cams[i].main_source_path));
        sqlite_query_column(hStmt, 17, &cams[i].sub_rtsp_enable);
        sqlite_query_column(hStmt, 18, &cams[i].sub_rtsp_port);
        sqlite_query_column_text(hStmt, 19, cams[i].sub_source_path, sizeof(cams[i].sub_source_path));
        sqlite_query_column(hStmt, 20, &cams[i].sync_time);
		sqlite_query_column(hStmt, 21, &cams[i].codec);
		sqlite_query_column_text(hStmt, 22, cams[i].main_rtspurl, sizeof(cams[i].main_rtspurl));
		sqlite_query_column_text(hStmt, 23, cams[i].sub_rtspurl, sizeof(cams[i].sub_rtspurl));	

		sqlite_query_column(hStmt, 24, &cams[i].poe_channel);
		sqlite_query_column(hStmt, 25, &cams[i].physical_port);
		sqlite_query_column_text(hStmt, 26, cams[i].mac_addr, sizeof(cams[i].mac_addr));
		//sqlite_query_column(hStmt, 27, &cams[i].connect_mode);
		
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *cnt = nResult;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_camera(const char *pDbFile, struct camera *cam)
{
    if(!pDbFile || !cam) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    snprintf(sExec, sizeof(sExec), "insert into camera(id,type,enable,covert_on,brightness,brightness,saturation,username,password,ip_addr,main_rtsp_port,main_source_path,sub_rtsp_enable,sub_rtsp_port,sub_source_path,manage_port,camera_protocol,transmit_protocol,play_stream,record_stream,sync_time,main_rtspurl,sub_rtspurl,poe_channel,physical_port,mac) values(%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s',%d,'%s',%d,%d,'%s',%d,%d,%d,%d,%d,%d,'%s','%s',%d,%d,'%s');", cam->id, cam->type, cam->enable, cam->covert_on, cam->brightness, cam->contrast, cam->saturation, cam->username, cam->password, cam->ip_addr, cam->main_rtsp_port, cam->main_source_path, cam->sub_rtsp_enable, cam->sub_rtsp_port, cam->sub_source_path, cam->manage_port, cam->camera_protocol, cam->transmit_protocol, cam->play_stream, cam->record_stream, cam->sync_time, cam->main_rtspurl, cam->sub_rtspurl, cam->poe_channel, cam->physical_port, cam->mac_addr);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;	
}

int write_camera_codec(const char *pDbFile, struct camera *cam)
{
    if(!pDbFile || !cam) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update camera set codec=%d where id=%d;", cam->codec, cam->id);
	//printf("camera sql:%s\n", sExec);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_camera(const char *pDbFile, struct camera *cam)
{
    if(!pDbFile || !cam) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    snprintf(sExec, sizeof(sExec), "update camera set type=%d,enable=%d,covert_on=%d,brightness=%d,contrast=%d,saturation=%d,username='%s',password='%s',ip_addr='%s',main_rtsp_port=%d,main_source_path='%s',sub_rtsp_enable=%d,sub_rtsp_port=%d,sub_source_path='%s',manage_port=%d,camera_protocol=%d,transmit_protocol=%d,play_stream=%d,record_stream=%d,sync_time=%d,main_rtspurl='%s',sub_rtspurl='%s',poe_channel=%d,physical_port=%d,mac='%s' where id=%d;", cam->type, cam->enable, cam->covert_on, cam->brightness, cam->contrast, cam->saturation, cam->username, cam->password, cam->ip_addr, cam->main_rtsp_port, cam->main_source_path, cam->sub_rtsp_enable, cam->sub_rtsp_port, cam->sub_source_path, cam->manage_port, cam->camera_protocol, cam->transmit_protocol, cam->play_stream, cam->record_stream, cam->sync_time, cam->main_rtspurl, cam->sub_rtspurl, cam->poe_channel, cam->physical_port, cam->mac_addr, cam->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_cameras(const char *pDbFile, struct camera cams[])
{
    if(!pDbFile || !cams) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
    int i = 0;
    for(i=0; i<MAX_CAMERA; i++)
    {
        memset(sExec, 0, sizeof(sExec));
        snprintf(sExec, sizeof(sExec), "update camera set type=%d,enable=%d,covert_on=%d,brightness=%d,contrast=%d,saturation=%d,username='%s',password='%s',ip_addr='%s',main_rtsp_port=%d,main_source_path='%s',sub_rtsp_enable=%d,sub_rtsp_port=%d,sub_source_path='%s',manage_port=%d,camera_protocol=%d,transmit_protocol=%d,play_stream=%d,record_stream=%d,sync_time=%d,main_rtspurl='%s',sub_rtspurl='%s',poe_channel=%d,physical_port=%d,mac='%s' where id=%d;", cams[i].type, cams[i].enable, cams[i].covert_on, cams[i].brightness, cams[i].contrast, cams[i].saturation, cams[i].username, cams[i].password, cams[i].ip_addr, cams[i].main_rtsp_port, cams[i].main_source_path, cams[i].sub_rtsp_enable, cams[i].sub_rtsp_port, cams[i].sub_source_path, cams[i].manage_port, cams[i].camera_protocol, cams[i].transmit_protocol, cams[i].play_stream, cams[i].record_stream, cams[i].sync_time, cams[i].main_rtspurl, cams[i].sub_rtspurl, cams[i].poe_channel, cams[i].physical_port, cams[i].mac_addr, cams[i].id);
        sqlite_execute(hConn, sExec);
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_record(const char *pDbFile, struct record *record, int chn_id)
{
    if(!pDbFile || !record || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,codec_type,iframe_interval,bitrate_type,bitrate,fps,resolution,audio_enable,audio_codec_type,audio_samplerate,prev_record_on,prev_record_duration,post_record_on,post_record_duration,input_video_signal_type,mode,record_expiration_date,photo_expiration_date from record where id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &record->id);
    sqlite_query_column(hStmt, 1, &record->codec_type);
    sqlite_query_column(hStmt, 2, &record->iframe_interval);
    sqlite_query_column(hStmt, 3, &record->bitrate_type);
    sqlite_query_column(hStmt, 4, &record->kbps);
    sqlite_query_column(hStmt, 5, &record->fps);
    sqlite_query_column(hStmt, 6, &record->resolution);

    sqlite_query_column(hStmt, 7, &record->audio_enable);
    sqlite_query_column(hStmt, 8, &record->audio_codec_type);
    sqlite_query_column(hStmt, 9, &record->audio_samplerate);

    sqlite_query_column(hStmt, 10, &record->prev_record_on);
    sqlite_query_column(hStmt, 11, &record->prev_record_duration);
    sqlite_query_column(hStmt, 12, &record->post_record_on);
    sqlite_query_column(hStmt, 13, &record->post_record_duration);
    sqlite_query_column(hStmt, 14, &record->input_video_signal_type);
	sqlite_query_column(hStmt, 15, &record->mode);
	sqlite_query_column(hStmt, 16, &record->record_expiration_date);
	sqlite_query_column(hStmt, 17, &record->photo_expiration_date);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_records(const char *pDbFile, struct record records[], int *pCnt)
{
    if(!pDbFile || !records) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,codec_type,iframe_interval,bitrate_type,bitrate,fps,resolution,audio_enable,audio_codec_type,audio_samplerate,prev_record_on,prev_record_duration,post_record_on,post_record_duration,input_video_signal_type,mode,record_expiration_date,photo_expiration_date from record;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &records[i].id);
        sqlite_query_column(hStmt, 1, &records[i].codec_type);
        sqlite_query_column(hStmt, 2, &records[i].iframe_interval);
        sqlite_query_column(hStmt, 3, &records[i].bitrate_type);
        sqlite_query_column(hStmt, 4, &records[i].kbps);
        sqlite_query_column(hStmt, 5, &records[i].fps);
        sqlite_query_column(hStmt, 6, &records[i].resolution);

        sqlite_query_column(hStmt, 7, &records[i].audio_enable);
        sqlite_query_column(hStmt, 8, &records[i].audio_codec_type);
        sqlite_query_column(hStmt, 9, &records[i].audio_samplerate);

        sqlite_query_column(hStmt, 10, &records[i].prev_record_on);
        sqlite_query_column(hStmt, 11, &records[i].prev_record_duration);
        sqlite_query_column(hStmt, 12, &records[i].post_record_on);
        sqlite_query_column(hStmt, 13, &records[i].post_record_duration);

        sqlite_query_column(hStmt, 14, &records[i].input_video_signal_type);
		sqlite_query_column(hStmt, 15, &records[i].mode);
		//david modify
		sqlite_query_column(hStmt, 16, &records[i].record_expiration_date);
		sqlite_query_column(hStmt, 17, &records[i].photo_expiration_date);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_records_ex(const char *pDbFile, struct record records[], long long changeFlag)
{
    if(!pDbFile || !records) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    int i = 0;
    for(i=0; i<MAX_CAMERA; i++)
   {
    	if((changeFlag >> i) & 0x01)
    	{
    		hStmt = 0;
    		memset(sQuery,0,sizeof(sQuery));
			snprintf(sQuery, sizeof(sQuery), "select id,codec_type,iframe_interval,bitrate_type,bitrate,fps,resolution,audio_enable,audio_codec_type,audio_samplerate,prev_record_on,prev_record_duration,post_record_on,post_record_duration,input_video_signal_type,mode,record_expiration_date from record where id=%d;",i);
			int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
			if(nResult !=0 || nColumnCnt == 0)
			{
				if(hStmt) sqlite_clear_stmt(hStmt);
				continue;
			}
			sqlite_query_column(hStmt, 0, &records[i].id);
			sqlite_query_column(hStmt, 1, &records[i].codec_type);
			sqlite_query_column(hStmt, 2, &records[i].iframe_interval);
			sqlite_query_column(hStmt, 3, &records[i].bitrate_type);
			sqlite_query_column(hStmt, 4, &records[i].kbps);
			sqlite_query_column(hStmt, 5, &records[i].fps);
			sqlite_query_column(hStmt, 6, &records[i].resolution);

			sqlite_query_column(hStmt, 7, &records[i].audio_enable);
			sqlite_query_column(hStmt, 8, &records[i].audio_codec_type);
			sqlite_query_column(hStmt, 9, &records[i].audio_samplerate);

			sqlite_query_column(hStmt, 10, &records[i].prev_record_on);
			sqlite_query_column(hStmt, 11, &records[i].prev_record_duration);
			sqlite_query_column(hStmt, 12, &records[i].post_record_on);
			sqlite_query_column(hStmt, 13, &records[i].post_record_duration);

			sqlite_query_column(hStmt, 14, &records[i].input_video_signal_type);
			sqlite_query_column(hStmt, 15, &records[i].mode);
			sqlite_query_column(hStmt, 16, &records[i].record_expiration_date);
			sqlite_clear_stmt(hStmt);
    	}
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_records(const char *pDbFile,struct record records[],long long changeFlag)
{
    if(!pDbFile || !records) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    int i = 0 ;
    for(i = 0; i < MAX_CAMERA;i++ )
    {
    	if((changeFlag >> i) & 0x01)
    	{
        	memset(sExec,0,sizeof(sExec));
        	snprintf(sExec, sizeof(sExec), "update record set codec_type=%d,iframe_interval=%d,bitrate_type=%d,bitrate=%d,fps=%d,resolution=%d,audio_enable=%d,audio_codec_type=%d,audio_samplerate=%d,prev_record_on=%d,prev_record_duration=%d,post_record_on=%d,post_record_duration=%d,input_video_signal_type=%d,mode=%d,record_expiration_date=%d where id=%d;", records[i].codec_type, records[i].iframe_interval, records[i].bitrate_type, records[i].kbps, records[i].fps, records[i].resolution, records[i].audio_enable, records[i].audio_codec_type, records[i].audio_samplerate, records[i].prev_record_on, records[i].prev_record_duration, records[i].post_record_on, records[i].post_record_duration, records[i].input_video_signal_type,records[i].mode,records[i].record_expiration_date, records[i].id);
        	sqlite_execute(hConn, sExec);
    	}
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
//end

int write_record(const char *pDbFile, struct record *record)
{
    if(!pDbFile || !record) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update record set codec_type=%d,iframe_interval=%d,bitrate_type=%d,bitrate=%d,fps=%d,resolution=%d,audio_enable=%d,audio_codec_type=%d,audio_samplerate=%d,prev_record_on=%d,prev_record_duration=%d,post_record_on=%d,post_record_duration=%d,input_video_signal_type=%d,mode=%d,record_expiration_date=%d,photo_expiration_date=%d where id=%d;", 
		record->codec_type, record->iframe_interval, record->bitrate_type, record->kbps, record->fps, record->resolution, record->audio_enable, record->audio_codec_type, record->audio_samplerate, record->prev_record_on, record->prev_record_duration, record->post_record_on, record->post_record_duration, record->input_video_signal_type,record->mode,record->record_expiration_date,record->photo_expiration_date, record->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_record(const char *pDbFile, struct record *record)
{
    if(!pDbFile || !record) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[1024] = {0};
    snprintf(sExec, sizeof(sExec), "insert into record(id,codec_type,iframe_interval,bitrate_type,bitrate,fps,resolution,audio_enable,audio_codec_type,audio_samplerate,prev_record_on,prev_record_duration,post_record_on,post_record_duration,input_video_signal_type,mode,record_expiration_date,photo_expiration_date) values(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);", 
		record->id, record->codec_type, record->iframe_interval, record->bitrate_type, record->kbps, record->fps, record->resolution, record->audio_enable, record->audio_codec_type, record->audio_samplerate, record->prev_record_on, record->prev_record_duration, record->post_record_on, record->post_record_duration, record->input_video_signal_type,record->mode,record->record_expiration_date,record->photo_expiration_date);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_record_prev_post(const char *pDbFile, struct record *record)
{
    if(!pDbFile || !record) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update record set prev_record_on=%d, prev_record_duration=%d, post_record_on=%d, post_record_duration=%d;", record->prev_record_on, record->prev_record_duration, record->post_record_on, record->post_record_duration);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_record_schedule(const char *pDbFile, struct record_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select enable,wday_id,wholeday_enable,wholeday_action_type,plan_id,start_time,end_time,action_type from record_schedule where chn_id=%d;", chn_id);

    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY*MAX_WND_NUM;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &schedule->enable);
        sqlite_query_column(hStmt, 1, &nWeekId);
        sqlite_query_column(hStmt, 2, &schedule->schedule_day[nWeekId].wholeday_enable);
        sqlite_query_column(hStmt, 3, &schedule->schedule_day[nWeekId].wholeday_action_type);
        sqlite_query_column(hStmt, 4, &nPlanId);
        sqlite_query_column_text(hStmt, 5, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 6, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 7, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_record_schedule(const char *pDbFile, struct record_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "delete from record_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);

    int i = 0;
    int j = 0;
    int nNotInsNum = 0;
    int nHasWholeDayEnable = 0;
	int t;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        nNotInsNum = 0;
        nHasWholeDayEnable = 0;
        if(schedule->schedule_day[i].wholeday_enable != 0)
        {
            nHasWholeDayEnable = 1;
        }
		t=0;
        for(j=0; j<MAX_PLAN_NUM_PER_DAY*MAX_WND_NUM; j++)
        {
            if(schedule->schedule_day[i].schedule_item[j].action_type==NONE || strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
            {
                nNotInsNum++;
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into record_schedule values(%d,%d,%d,%d,%d,'%s','%s',%d,%d);", chn_id, i, schedule->schedule_day[i].wholeday_enable, schedule->schedule_day[i].wholeday_action_type, t, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type, schedule->enable);
			t++;
            sqlite_execute(hConn, sExec);
        }

        if(nHasWholeDayEnable == 1 && nNotInsNum == MAX_PLAN_NUM_PER_DAY)
        {
            snprintf(sExec, sizeof(sExec), "insert into record_schedule values(%d,%d,%d,%d,%d,'%s','%s',%d,%d);", chn_id, i, schedule->schedule_day[i].wholeday_enable, schedule->schedule_day[i].wholeday_action_type, 0, "00:00:00", "00:00:00", 0, schedule->enable);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int reset_record_schedule(const char *pDbFile, int chn_id)
{
    if(!pDbFile || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "delete from record_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
	
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_holidays(const char *pDbFile, struct holiday holidays[], int *pCnt)
{
    if(!pDbFile || !holidays) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,name,type,start_year,start_mon,start_mday,start_mweek,start_wday,end_year,end_mon,end_mday,end_mweek,end_wday from holiday;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &holidays[i].id);
        sqlite_query_column(hStmt, 1, &holidays[i].enable);
        sqlite_query_column_text(hStmt, 2, holidays[i].name, sizeof(holidays[i].name));
        sqlite_query_column(hStmt, 3, &holidays[i].type);
        sqlite_query_column(hStmt, 4, &holidays[i].start_year);
        sqlite_query_column(hStmt, 5, &holidays[i].start_mon);
        sqlite_query_column(hStmt, 6, &holidays[i].start_mday);
        sqlite_query_column(hStmt, 7, &holidays[i].start_mweek);
        sqlite_query_column(hStmt, 8, &holidays[i].start_wday);
        sqlite_query_column(hStmt, 9, &holidays[i].end_year);
        sqlite_query_column(hStmt, 10, &holidays[i].end_mon);
        sqlite_query_column(hStmt, 11, &holidays[i].end_mday);
        sqlite_query_column(hStmt, 12, &holidays[i].end_mweek);
        sqlite_query_column(hStmt, 13, &holidays[i].end_wday);

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_holiday(const char *pDbFile, struct holiday *holiday)
{
    if(!pDbFile || !holiday) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update holiday set enable=%d,name='%s',type=%d,start_year=%d,start_mon=%d,start_mday=%d,start_mweek=%d,start_wday=%d,end_year=%d,end_mon=%d,end_mday=%d,end_mweek=%d,end_wday=%d where id=%d;", holiday->enable, holiday->name, holiday->type, holiday->start_year, holiday->start_mon, holiday->start_mday, holiday->start_mweek, holiday->start_wday, holiday->end_year, holiday->end_mon, holiday->end_mday, holiday->end_mweek, holiday->end_wday, holiday->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_ptz_port(const char *pDbFile, struct ptz_port *port, int chn_id)
{
    if(!pDbFile || !port || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id, baudrate,data_bit,stop_bit,parity_type,protocol,address,com_type,connect_type from ptz_port where id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &port->id);
    sqlite_query_column(hStmt, 1, &port->baudrate);
    sqlite_query_column(hStmt, 2, &port->data_bit);
    sqlite_query_column(hStmt, 3, &port->stop_bit);
    sqlite_query_column(hStmt, 4, &port->parity_type);
    sqlite_query_column(hStmt, 5, &port->protocol);
    sqlite_query_column(hStmt, 6, &port->address);
    sqlite_query_column(hStmt, 7, &port->com_type);
    sqlite_query_column(hStmt, 8, &port->connect_type);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_ptz_port(const char *pDbFile, struct ptz_port *port)
{
    if(!pDbFile || !port) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update ptz_port set baudrate=%d,data_bit=%d,stop_bit=%d,parity_type=%d,protocol=%d,address=%d,com_type=%d,connect_type=%d where id=%d;", port->baudrate, port->data_bit, port->stop_bit, port->parity_type, port->protocol, port->address, port->com_type, port->connect_type, port->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_ptz_ports(const char *pDbFile, struct ptz_port ports[], int *pCnt)
{
    if(!pDbFile || !ports) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,baudrate,data_bit,stop_bit,parity_type,protocol,address,com_type,connect_type from ptz_port;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &ports[i].id);
        sqlite_query_column(hStmt, 1, &ports[i].baudrate);
        sqlite_query_column(hStmt, 2, &ports[i].data_bit);
        sqlite_query_column(hStmt, 3, &ports[i].stop_bit);
        sqlite_query_column(hStmt, 4, &ports[i].parity_type);
        sqlite_query_column(hStmt, 5, &ports[i].protocol);
        sqlite_query_column(hStmt, 6, &ports[i].address);
        sqlite_query_column(hStmt, 7, &ports[i].com_type);
        sqlite_query_column(hStmt, 8, &ports[i].connect_type);

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_in(const char *pDbFile, struct alarm_in *alarm, int alarm_id)
{
    if(!pDbFile || !alarm || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id, enable,name,type,tri_channels,tri_alarms,buzzer_interval,tri_channels_ex,acto_ptz_channel,acto_ptz_preset_enable,acto_ptz_preset,acto_ptz_patrol_enable,acto_ptz_patrol,email_enable,email_buzzer_interval,Names,tri_channels_snapshot from alarm_in where id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &alarm->id);
    sqlite_query_column(hStmt, 1, &alarm->enable);
    sqlite_query_column_text(hStmt, 2, alarm->name, sizeof(alarm->name));
    sqlite_query_column(hStmt, 3, &alarm->type);
    sqlite_query_column(hStmt, 4, (int*)&alarm->tri_channels);
    sqlite_query_column(hStmt, 5, (int*)&alarm->tri_alarms);
    sqlite_query_column(hStmt, 6, &alarm->buzzer_interval);
	sqlite_query_column_text(hStmt, 7, alarm->tri_channels_ex, sizeof(alarm->tri_channels_ex));
	sqlite_query_column(hStmt, 8, &alarm->acto_ptz_channel);
	sqlite_query_column(hStmt, 9, &alarm->acto_ptz_preset_enable);
	sqlite_query_column(hStmt, 10, &alarm->acto_ptz_preset);
	sqlite_query_column(hStmt, 11, &alarm->acto_ptz_patrol_enable);
	sqlite_query_column(hStmt, 12, &alarm->acto_ptz_patrol);
	sqlite_query_column(hStmt, 13, (int*)&alarm->email_enable);
    sqlite_query_column(hStmt, 14, &alarm->email_buzzer_interval);
	sqlite_query_column_text(hStmt, 15, alarm->name, sizeof(alarm->name));
	sqlite_query_column_text(hStmt, 16, alarm->tri_channels_snapshot, sizeof(alarm->tri_channels_snapshot));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_ins(const char *pDbFile, struct alarm_in alarms[], int *pCnt)
{
    if(!pDbFile || !alarms) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,name,type,tri_channels,tri_alarms,buzzer_interval,tri_channels_ex,email_enable,email_buzzer_interval,Names,tri_channels_snapshot from alarm_in;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_ALARM_IN; i++)
    {
        sqlite_query_column(hStmt, 0, &alarms[i].id);
        sqlite_query_column(hStmt, 1, &alarms[i].enable);
        sqlite_query_column_text(hStmt, 2, alarms[i].name, sizeof(alarms[i].name));
        sqlite_query_column(hStmt, 3, &alarms[i].type);
        sqlite_query_column(hStmt, 4, (int*)&alarms[i].tri_channels);
        sqlite_query_column(hStmt, 5, (int*)&alarms[i].tri_alarms);
        sqlite_query_column(hStmt, 6, &alarms[i].buzzer_interval);
		sqlite_query_column_text(hStmt, 7, alarms[i].tri_channels_ex, sizeof(alarms[i].tri_channels_ex));
		sqlite_query_column(hStmt, 8, (int*)&alarms[i].email_enable);
        sqlite_query_column(hStmt, 9, &alarms[i].email_buzzer_interval);
		sqlite_query_column_text(hStmt, 10, alarms[i].name, sizeof(alarms[i].name));
		sqlite_query_column_text(hStmt, 11, alarms[i].tri_channels_snapshot, sizeof(alarms[i].tri_channels_snapshot));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarm_in(const char *pDbFile, struct alarm_in *alarm)
{
    if(!pDbFile || !alarm) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update alarm_in set enable=%d,name='%s',type=%d, tri_channels=%u,tri_alarms=%u, buzzer_interval =%d,tri_channels_ex='%s', acto_ptz_channel=%d, acto_ptz_preset_enable=%d ,acto_ptz_preset=%d ,acto_ptz_patrol_enable=%d ,acto_ptz_patrol =%d ,email_enable=%d, email_buzzer_interval=%d,Names='%s',tri_channels_snapshot='%s' where id=%d;",
    		alarm->enable, alarm->name, alarm->type, alarm->tri_channels, alarm->tri_alarms, alarm->buzzer_interval,alarm->tri_channels_ex,alarm->acto_ptz_channel,alarm->acto_ptz_preset_enable,alarm->acto_ptz_preset,alarm->acto_ptz_patrol_enable,alarm->acto_ptz_patrol,alarm->email_enable, alarm->email_buzzer_interval, alarm->name, alarm->tri_channels_snapshot, alarm->id);
	//printf("write_alarm_in sExec:%s len:%d\n", sExec, strlen(sExec));
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_in_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select wday_id,plan_id,start_time,end_time,action_type from alarm_in_schedule where alarm_id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 0;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarm_in_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from alarm_in_schedule where alarm_id=%d;", alarm_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into alarm_in_schedule values(%d,%d,%d,'%s','%s',%d);", alarm_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
int read_alarmin_audible_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from alarmin_audible_schedule where chn_id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 0;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarmin_audible_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from alarmin_audible_schedule where chn_id=%d;", alarm_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into alarmin_audible_schedule values(%d,%d,%d,'%s','%s',%d);", alarm_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarmin_ptz_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from alarmin_ptz_schedule where chn_id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 0;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarmin_ptz_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id)
{
    if(!pDbFile || !schedule || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from alarmin_ptz_schedule where chn_id=%d;", alarm_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into alarmin_ptz_schedule values(%d,%d,%d,'%s','%s',%d);", alarm_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_user(const char *pDbFile, struct db_user *user, int id)
{
    if(!pDbFile || !user || id<0 || id>10) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,enable,username,password,type,permission,remote_permission,local_live_view,local_playback,remote_live_view,remote_playback,local_live_view_ex,local_playback_ex,remote_live_view_ex,remote_playback_ex,password_ex from user where id=%d;", id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &(user->id));
    sqlite_query_column(hStmt, 1, &(user->enable));
    sqlite_query_column_text(hStmt, 2, user->username, sizeof(user->username));
    sqlite_query_column_text(hStmt, 3, user->password, sizeof(user->password));
    sqlite_query_column(hStmt, 4, &(user->type));
    sqlite_query_column(hStmt, 5, &(user->permission));
    sqlite_query_column(hStmt, 6, &(user->remote_permission));
	sqlite_query_column(hStmt, 7, (int*)&(user->local_live_view));
	sqlite_query_column(hStmt, 8, (int*)&(user->local_playback));
	sqlite_query_column(hStmt, 9, (int*)&(user->remote_live_view));
	sqlite_query_column(hStmt, 10, (int*)&(user->remote_playback));
	sqlite_query_column_text(hStmt, 11, user->local_live_view_ex, sizeof(user->local_live_view_ex));
    sqlite_query_column_text(hStmt, 12, user->local_playback_ex, sizeof(user->local_playback_ex));
	sqlite_query_column_text(hStmt, 13, user->remote_live_view_ex, sizeof(user->remote_live_view_ex));
    sqlite_query_column_text(hStmt, 14, user->remote_playback_ex, sizeof(user->remote_playback_ex));
	sqlite_query_column_text(hStmt, 15, user->password_ex, sizeof(user->password_ex));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_user_by_name(const char *pDbFile, struct db_user *user, char *username)
{
	if(!pDbFile || !user || !username) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,enable,username,password,type,permission,remote_permission,local_live_view,local_playback,remote_live_view,remote_playback,local_live_view_ex,local_playback_ex,remote_live_view_ex,remote_playback_ex,password_ex from user where username='%s';", username);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &(user->id));
    sqlite_query_column(hStmt, 1, &(user->enable));
    sqlite_query_column_text(hStmt, 2, user->username, sizeof(user->username));
    sqlite_query_column_text(hStmt, 3, user->password, sizeof(user->password));
    sqlite_query_column(hStmt, 4, &(user->type));
    sqlite_query_column(hStmt, 5, &(user->permission));
    sqlite_query_column(hStmt, 6, &(user->remote_permission));
	sqlite_query_column(hStmt, 7, (int*)&(user->local_live_view));
	sqlite_query_column(hStmt, 8, (int*)&(user->local_playback));
	sqlite_query_column(hStmt, 9, (int*)&(user->remote_live_view));
	sqlite_query_column(hStmt, 10, (int*)&(user->remote_playback));
	sqlite_query_column_text(hStmt, 11, user->local_live_view_ex, sizeof(user->local_live_view_ex));
    sqlite_query_column_text(hStmt, 12, user->local_playback_ex, sizeof(user->local_playback_ex));
	sqlite_query_column_text(hStmt, 13, user->remote_live_view_ex, sizeof(user->remote_live_view_ex));
    sqlite_query_column_text(hStmt, 14, user->remote_playback_ex, sizeof(user->remote_playback_ex));
	sqlite_query_column_text(hStmt, 15, user->password_ex, sizeof(user->password_ex));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_users(const char *pDbFile, struct db_user users[], int *pCnt)
{
    if(!pDbFile || !users) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,username,password,type,permission,remote_permission,local_live_view,local_playback,remote_live_view,remote_playback,local_live_view_ex,local_playback_ex,remote_live_view_ex,remote_playback_ex,password_ex from user;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_USER; i++)
    {
        sqlite_query_column(hStmt, 0, &(users[i].id));
        sqlite_query_column(hStmt, 1, &(users[i].enable));
        sqlite_query_column_text(hStmt, 2, users[i].username, sizeof(users[i].username));
        sqlite_query_column_text(hStmt, 3, users[i].password, sizeof(users[i].password));
        sqlite_query_column(hStmt, 4, &(users[i].type));
        sqlite_query_column(hStmt, 5, &(users[i].permission));
        sqlite_query_column(hStmt, 6, &(users[i].remote_permission));
		sqlite_query_column(hStmt, 7, (int*)&(users[i].local_live_view));
		sqlite_query_column(hStmt, 8, (int*)&(users[i].local_playback));
		sqlite_query_column(hStmt, 9, (int*)&(users[i].remote_live_view));
		sqlite_query_column(hStmt, 10, (int*)&(users[i].remote_playback));
		sqlite_query_column_text(hStmt, 11, users[i].local_live_view_ex, sizeof(users[i].local_live_view_ex));
	    sqlite_query_column_text(hStmt, 12, users[i].local_playback_ex, sizeof(users[i].local_playback_ex));
		sqlite_query_column_text(hStmt, 13, users[i].remote_live_view_ex, sizeof(users[i].remote_live_view_ex));
	    sqlite_query_column_text(hStmt, 14, users[i].remote_playback_ex, sizeof(users[i].remote_playback_ex));
		sqlite_query_column_text(hStmt, 15, users[i].password_ex, sizeof(users[i].password_ex));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int add_user(const char *pDbFile, struct db_user *user)
{
	if(!pDbFile || !user) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
    int nId = 0;
	char sQuery[1024] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id from user order by(enable);");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &nId);
	sqlite_clear_stmt(hStmt);

    char buf1[MAX_PWD_LEN*2+2] = {0};
	char buf2[MAX_PWD_LEN*2+2] = {0};
	translate_pwd(buf1,user->password,strlen(user->password));
	translate_pwd(buf2,user->password_ex,strlen(user->password_ex));

    snprintf(sQuery, sizeof(sQuery), "update user set enable=%d,username='%s',password='%s',type=%d,permission=%d,remote_permission=%d,local_live_view=%ld,local_playback=%ld,remote_live_view=%ld,remote_playback=%ld,local_live_view_ex='%s',local_playback_ex='%s',remote_live_view_ex='%s',remote_playback_ex='%s',password_ex='%s' where id=%d;", user->enable, user->username, buf1, user->type, user->permission, user->remote_permission, user->local_live_view, user->local_playback, user->remote_live_view, user->remote_playback, user->local_live_view_ex, user->local_playback_ex, user->remote_live_view_ex, user->remote_playback_ex,buf2, nId);
    sqlite_execute(hConn, sQuery);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_user(const char *pDbFile, struct db_user *user)
{
	if(!pDbFile || !user) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	char buf1[MAX_PWD_LEN*2+2] = {0};
	char buf2[MAX_PWD_LEN*2+2] = {0};
	translate_pwd(buf1,user->password,strlen(user->password));
	translate_pwd(buf2,user->password_ex,strlen(user->password_ex));
	
	char sExec[1024] = {0};
	//if(user->password_ex[0] == '\0')
	//	snprintf(user->password_ex, sizeof(user->password_ex), "%s", "ms1234");
	snprintf(sExec, sizeof(sExec), "update user set enable=%d, username='%s',password='%s',type=%d,permission=%d,remote_permission=%d,local_live_view=%ld,local_playback=%ld,remote_live_view=%ld,remote_playback=%ld,local_live_view_ex='%s',local_playback_ex='%s',remote_live_view_ex='%s',remote_playback_ex='%s',password_ex='%s' where id=%d;", user->enable, user->username, buf1, user->type, user->permission, user->remote_permission, user->local_live_view, user->local_playback, user->remote_live_view, user->remote_playback, user->local_live_view_ex, user->local_playback_ex, user->remote_live_view_ex, user->remote_playback_ex,buf2, user->id);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_user_count(const char *pDbFile, int *pCnt)
{
    if(!pDbFile || !pCnt) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select count(*) from user where enable=1;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    sqlite_query_column(hStmt, 0, pCnt);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_display(const char *pDbFile, struct display *display)
{
	if(!pDbFile || !display) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select main_resolution,main_seq_enable,main_seq_interval,audio_out_on,hdmi_audio_on,volume,stereo,sub_resolution,sub_seq_enable,sub_seq_interval,spot_out_channel,spot_resolution,border_line_on,date_format,show_channel_name,sub_enable,camera_info,start_screen,page_info,eventPop_screen,eventPop_time from display;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &(display->main_resolution));
    sqlite_query_column(hStmt, 1, &(display->main_seq_enable));
    sqlite_query_column(hStmt, 2, &(display->main_seq_interval));
    sqlite_query_column(hStmt, 3, &(display->audio_output_on));
    sqlite_query_column(hStmt, 4, &(display->hdmi_audio_on));
    sqlite_query_column(hStmt, 5, &(display->volume));
    sqlite_query_column(hStmt, 6, &(display->stereo));

    sqlite_query_column(hStmt, 7, &(display->sub_resolution));
    sqlite_query_column(hStmt, 8, &(display->sub_seq_enable));
    sqlite_query_column(hStmt, 9, &(display->sub_seq_interval));
    sqlite_query_column(hStmt, 10, &(display->spot_output_channel));
    sqlite_query_column(hStmt, 11, &(display->spot_resolution));

    sqlite_query_column(hStmt, 12, &(display->border_line_on));
    sqlite_query_column(hStmt, 13, &(display->date_format));
    sqlite_query_column(hStmt, 14, &(display->show_channel_name));
	sqlite_query_column(hStmt, 15, &(display->sub_enable));
	sqlite_query_column(hStmt, 16, &(display->camera_info));
	sqlite_query_column(hStmt, 17, &(display->start_screen));
	sqlite_query_column(hStmt, 18, &(display->page_info));
	sqlite_query_column(hStmt, 19, &(display->eventPop_screen));
	sqlite_query_column(hStmt, 20, &(display->eventPop_time));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

//8.0.2-oem/////
int read_display_oem(const char *pDbFile, struct display *display)
{
	if(!pDbFile || !display) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select main_resolution,main_seq_enable,main_seq_interval,audio_out_on,hdmi_audio_on,volume,stereo,sub_resolution,sub_seq_enable,sub_seq_interval,spot_out_channel,spot_resolution,border_line_on,date_format,show_channel_name,sub_enable from display;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &(display->main_resolution));
    sqlite_query_column(hStmt, 1, &(display->main_seq_enable));
    sqlite_query_column(hStmt, 2, &(display->main_seq_interval));
    sqlite_query_column(hStmt, 3, &(display->audio_output_on));
    sqlite_query_column(hStmt, 4, &(display->hdmi_audio_on));
    sqlite_query_column(hStmt, 5, &(display->volume));
    sqlite_query_column(hStmt, 6, &(display->stereo));

    sqlite_query_column(hStmt, 7, &(display->sub_resolution));
    sqlite_query_column(hStmt, 8, &(display->sub_seq_enable));
    sqlite_query_column(hStmt, 9, &(display->sub_seq_interval));
    sqlite_query_column(hStmt, 10, &(display->spot_output_channel));
    sqlite_query_column(hStmt, 11, &(display->spot_resolution));

    sqlite_query_column(hStmt, 12, &(display->border_line_on));
    sqlite_query_column(hStmt, 13, &(display->date_format));
    sqlite_query_column(hStmt, 14, &(display->show_channel_name));
	sqlite_query_column(hStmt, 15, &(display->sub_enable));

	//////////for db-1014.txt
	display->camera_info = 0;
	display->start_screen = 0;
	display->page_info = 0;
	display->eventPop_screen = 0;
	display->eventPop_time = 11;
	display->main_seq_interval = 0;
	display->sub_seq_interval = 0;
	//////////end
	
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_display(const char *pDbFile, struct display *display)
{
	if(!pDbFile || !display) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update display set main_resolution=%d,main_seq_enable=%d,main_seq_interval=%d,audio_out_on=%d,hdmi_audio_on=%d,volume=%d,stereo=%d,sub_resolution=%d,sub_seq_enable=%d,sub_seq_interval=%d,spot_out_channel=%d,spot_resolution=%d,border_line_on=%d,date_format=%d,show_channel_name=%d,sub_enable=%d,camera_info=%d,start_screen=%d,page_info=%d,eventPop_screen=%d,eventPop_time=%d;", display->main_resolution, display->main_seq_enable, display->main_seq_interval, display->audio_output_on, display->hdmi_audio_on, display->volume, display->stereo, display->sub_resolution, display->sub_seq_enable, display->sub_seq_interval, display->spot_output_channel, display->spot_resolution, display->border_line_on, display->date_format, display->show_channel_name, display->sub_enable, display->camera_info, display->start_screen, display->page_info, display->eventPop_screen, display->eventPop_time);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_spots(const char *pDbFile, struct spot spots[], int *pCnt)
{
	if(!pDbFile || !spots) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
        *pCnt = 0;
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
    int i = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,seq_enable, seq_interval, input_channels from spot;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    for(i=0; i<DVR_SPOT_OUT_CH; i++)
    {
        sqlite_query_column(hStmt, 0, &(spots[i].id));
        sqlite_query_column(hStmt, 1, &(spots[i].seq_enable));
        sqlite_query_column(hStmt, 2, &(spots[i].seq_interval));
        sqlite_query_column(hStmt, 3, &(spots[i].input_channels));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_spot(const char *pDbFile, struct spot *spot)
{
	if(!pDbFile || !spot) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update spot set seq_enable=%d, seq_interval=%d,input_channels=%d where id=%d;", spot->seq_enable, spot->seq_interval, spot->input_channels, spot->id);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_time(const char *pDbFile, struct time *times)
{
	if(!pDbFile || !times) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select ntp_enable,dst_enable,time_zone,time_zone_name,ntp_server from time;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(times->ntp_enable));
    sqlite_query_column(hStmt, 1, &(times->dst_enable));
    sqlite_query_column_text(hStmt, 2, times->time_zone, sizeof(times->time_zone));
    sqlite_query_column_text(hStmt, 3, times->time_zone_name, sizeof(times->time_zone_name));
    sqlite_query_column_text(hStmt, 4, times->ntp_server, sizeof(times->ntp_server));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_time(const char *pDbFile, struct time *times)
{
	if(!pDbFile || !times) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update time set ntp_enable=%d,dst_enable=%d,time_zone='%s',time_zone_name='%s',ntp_server='%s';", times->ntp_enable, times->dst_enable, times->time_zone, times->time_zone_name, times->ntp_server);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_pppoe(const char *pDbFile, struct pppoe *pppoe)
{
	if(!pDbFile || !pppoe) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select enable,auto_connect,username,password from pppoe;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(pppoe->enable));
    sqlite_query_column(hStmt, 1, &(pppoe->auto_connect));
    sqlite_query_column_text(hStmt, 2, pppoe->username, sizeof(pppoe->username));
    sqlite_query_column_text(hStmt, 3, pppoe->password, sizeof(pppoe->password));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_pppoe(const char *pDbFile, struct pppoe *pppoe)
{
	if(!pDbFile || !pppoe) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update pppoe set enable=%d, auto_connect=%d, username='%s', password='%s';", pppoe->enable, pppoe->auto_connect, pppoe->username, pppoe->password);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_ddns(const char *pDbFile, struct ddns *ddns)
{
	if(!pDbFile || !ddns) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select enable, domain, username, password, host_name, free_dns_hash, update_freq, http_port, rtsp_port, ddnsurl from ddns;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(ddns->enable));
    sqlite_query_column_text(hStmt, 1, ddns->domain, sizeof(ddns->domain));
    sqlite_query_column_text(hStmt, 2, ddns->username, sizeof(ddns->username));
    sqlite_query_column_text(hStmt, 3, ddns->password, sizeof(ddns->password));
    sqlite_query_column_text(hStmt, 4, ddns->host_name, sizeof(ddns->host_name));
    sqlite_query_column_text(hStmt, 5, ddns->free_dns_hash, sizeof(ddns->free_dns_hash));
    sqlite_query_column(hStmt, 6, &(ddns->update_freq));
	sqlite_query_column(hStmt, 7, &(ddns->http_port));
	sqlite_query_column(hStmt, 8, &(ddns->rtsp_port));
	sqlite_query_column_text(hStmt, 9, ddns->ddnsurl, sizeof(ddns->ddnsurl));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_ddns(const char *pDbFile, struct ddns *ddns)
{
	if(!pDbFile || !ddns) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update ddns set enable=%d,domain='%s',username='%s', password='%s', host_name='%s', free_dns_hash='%s', update_freq=%d, http_port=%d, rtsp_port=%d, ddnsurl='%s';", ddns->enable, ddns->domain, ddns->username, ddns->password, ddns->host_name, ddns->free_dns_hash, ddns->update_freq, ddns->http_port, ddns->rtsp_port, ddns->ddnsurl);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_upnp(const char *pDbFile, struct upnp *upnp)
{
	if(!pDbFile || !upnp) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select enable, type, name, extern_ip, http_port, http_status, rtsp_port, rtsp_status from upnp;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(upnp->enable));
    sqlite_query_column(hStmt, 1, &(upnp->type));
    sqlite_query_column_text(hStmt, 2, upnp->name, sizeof(upnp->name));
    sqlite_query_column_text(hStmt, 3, upnp->extern_ip, sizeof(upnp->extern_ip));
    sqlite_query_column(hStmt, 4, &(upnp->http_port));
    sqlite_query_column(hStmt, 5, &(upnp->http_status));
    sqlite_query_column(hStmt, 6, &(upnp->rtsp_port));
	sqlite_query_column(hStmt, 7, &(upnp->rtsp_status));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


int write_upnp(const char *pDbFile, struct upnp *upnp)
{
	if(!pDbFile || !upnp) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update upnp set enable=%d,type=%d,name='%s', extern_ip='%s', http_port=%d, http_status=%d, rtsp_port=%d, rtsp_status=%d;", upnp->enable, upnp->type, upnp->name, upnp->extern_ip, upnp->http_port, upnp->http_status, upnp->rtsp_port, upnp->rtsp_status);
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


int read_email(const char *pDbFile, struct email *email)
{
	if(!pDbFile || !email) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select username, password, smtp_server, port, sender_addr, sender_name, enable_tls, enable_attach, capture_interval from email;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column_text(hStmt, 0, email->username, sizeof(email->username));
    sqlite_query_column_text(hStmt, 1, email->password, sizeof(email->password));
    sqlite_query_column_text(hStmt, 2, email->smtp_server, sizeof(email->smtp_server));
    sqlite_query_column(hStmt, 3, &(email->port));
    sqlite_query_column_text(hStmt, 4, email->sender_addr, sizeof(email->sender_addr));
    sqlite_query_column_text(hStmt, 5, email->sender_name, sizeof(email->sender_name));
    sqlite_query_column(hStmt, 6, &(email->enable_tls));
    sqlite_query_column(hStmt, 7, &(email->enable_attach));
    sqlite_query_column(hStmt, 8, &(email->capture_interval));
	sqlite_clear_stmt(hStmt);

    memset(sQuery, 0, sizeof(sQuery));
	snprintf(sQuery, sizeof(sQuery), "select address, name from email_receiver;");
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    int i = 0;
    for(i=0; i<EMAIL_RECEIVER_NUM; i++)
    {
        sqlite_query_column_text(hStmt, 0, email->receiver[i].address, sizeof(email->receiver[i].address));
        sqlite_query_column_text(hStmt, 1, email->receiver[i].name, sizeof(email->receiver[i].name));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_email(const char *pDbFile, struct email *email)
{
	if(!pDbFile || !email) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update email set username='%s', password='%s',smtp_server='%s',port=%d,sender_addr='%s',sender_name='%s',enable_tls=%d,enable_attach=%d,capture_interval=%d;", email->username, email->password, email->smtp_server, email->port, email->sender_addr, email->sender_name, email->enable_tls, email->enable_attach, email->capture_interval);
    sqlite_execute(hConn, sExec);

    memset(sExec, 0, sizeof(sExec));
    int i = 0;
    for(i=0; i<EMAIL_RECEIVER_NUM; i++)
    {
        snprintf(sExec, sizeof(sExec), "update email_receiver set address='%s', name='%s' where id=%d;", email->receiver[i].address, email->receiver[i].name, i);
        sqlite_execute(hConn, sExec);
    }
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_network(const char *pDbFile, struct network *net)
{
	int mode = 0;//
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[2048] = {0};
	//snprintf(sQuery, sizeof(sQuery), "select mode,host_name,miimon,bond0_primary_net, bond0_enable, bond0_type, bond0_ip_address, bond0_netmask, bond0_gateway, bond0_primary_dns, bond0_second_dns, bond0_mtu,lan1_enable, lan1_type, lan1_ip_address, lan1_netmask, lan1_gateway, lan1_primary_dns, lan1_second_dns, lan1_mtu, lan2_enable, lan2_type, lan2_ip_address, lan2_netmask, lan2_gateway, lan2_primary_dns, lan2_second_dns, lan2_mtu, lan1_dhcp_gateway, lan2_dhcp_gateway from network;");
	snprintf(sQuery, sizeof(sQuery), "select mode,host_name,miimon,bond0_primary_net, bond0_enable, bond0_type, bond0_ip_address, bond0_netmask, bond0_gateway, bond0_primary_dns, bond0_second_dns, bond0_mtu,lan1_enable, lan1_type, lan1_ip_address, lan1_netmask, lan1_gateway, lan1_primary_dns, lan1_second_dns, lan1_mtu, lan2_enable, lan2_type, lan2_ip_address, lan2_netmask, lan2_gateway, lan2_primary_dns, lan2_second_dns, lan2_mtu, lan1_dhcp_gateway, lan2_dhcp_gateway, lan1_ip6_address, lan1_ip6_netmask, lan1_ip6_gateway, lan2_ip6_address, lan2_ip6_netmask, lan2_ip6_gateway, lan1_ip6_dhcp, lan2_ip6_dhcp, bond0_ip6_dhcp, bond0_ip6_address, bond0_ip6_netmask, bond0_ip6_gateway from network;");//hrz.milesight
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
		if(hStmt)
		{
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
		}
		snprintf(sQuery, sizeof(sQuery), "select mode,host_name,miimon,bond0_primary_net, bond0_enable, bond0_type, bond0_ip_address, bond0_netmask, bond0_gateway, bond0_primary_dns, bond0_second_dns, bond0_mtu,lan1_enable, lan1_type, lan1_ip_address, lan1_netmask, lan1_gateway, lan1_primary_dns, lan1_second_dns, lan1_mtu, lan2_enable, lan2_type, lan2_ip_address, lan2_netmask, lan2_gateway, lan2_primary_dns, lan2_second_dns, lan2_mtu, lan1_dhcp_gateway, lan2_dhcp_gateway from network;");
		nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
		if (nResult != 0)
		{
		
	        if(hStmt) sqlite_clear_stmt(hStmt);
	        sqlite_disconn(hConn);
			FileUnlock(nFd, &global_rwlock);
			return -1;
		}

		mode = 1;
	}
    sqlite_query_column(hStmt, 0, &(net->mode));
    sqlite_query_column_text(hStmt, 1, net->host_name, sizeof(net->host_name));
    sqlite_query_column(hStmt, 2, &(net->miimon));

    sqlite_query_column(hStmt, 3, &(net->bond0_primary_net));
    sqlite_query_column(hStmt, 4, &(net->bond0_enable));
    sqlite_query_column(hStmt, 5, &(net->bond0_type));
    sqlite_query_column_text(hStmt, 6, net->bond0_ip_address, sizeof(net->bond0_ip_address));
    sqlite_query_column_text(hStmt, 7, net->bond0_netmask, sizeof(net->bond0_netmask));
    sqlite_query_column_text(hStmt, 8, net->bond0_gateway, sizeof(net->bond0_gateway));
    sqlite_query_column_text(hStmt, 9, net->bond0_primary_dns, sizeof(net->bond0_primary_dns));
    sqlite_query_column_text(hStmt, 10, net->bond0_second_dns, sizeof(net->bond0_second_dns));
    sqlite_query_column(hStmt, 11, &(net->bond0_mtu));

    sqlite_query_column(hStmt, 12, &(net->lan1_enable));
    sqlite_query_column(hStmt, 13, &(net->lan1_type));
    sqlite_query_column_text(hStmt, 14, net->lan1_ip_address, sizeof(net->lan1_ip_address));
    sqlite_query_column_text(hStmt, 15, net->lan1_netmask, sizeof(net->lan1_netmask));
    sqlite_query_column_text(hStmt, 16, net->lan1_gateway, sizeof(net->lan1_gateway));
    sqlite_query_column_text(hStmt, 17, net->lan1_primary_dns, sizeof(net->lan1_primary_dns));
    sqlite_query_column_text(hStmt, 18, net->lan1_second_dns, sizeof(net->lan1_second_dns));
    sqlite_query_column(hStmt, 19, &(net->lan1_mtu));

    sqlite_query_column(hStmt, 20, &(net->lan2_enable));
    sqlite_query_column(hStmt, 21, &(net->lan2_type));
    sqlite_query_column_text(hStmt, 22, net->lan2_ip_address, sizeof(net->lan2_ip_address));
    sqlite_query_column_text(hStmt, 23, net->lan2_netmask, sizeof(net->lan2_netmask));
    sqlite_query_column_text(hStmt, 24, net->lan2_gateway, sizeof(net->lan2_gateway));
    sqlite_query_column_text(hStmt, 25, net->lan2_primary_dns, sizeof(net->lan2_primary_dns));
    sqlite_query_column_text(hStmt, 26, net->lan2_second_dns, sizeof(net->lan2_second_dns));
    sqlite_query_column(hStmt, 27, &(net->lan2_mtu));
	sqlite_query_column_text(hStmt, 28, net->lan1_dhcp_gateway, sizeof(net->lan1_dhcp_gateway));
	sqlite_query_column_text(hStmt, 29, net->lan2_dhcp_gateway, sizeof(net->lan2_dhcp_gateway));

	if (0 == mode)
	{
		//sqlite_query_column(hStmt, 30, &(net->tri_alarms));//hrz.milesight
		sqlite_query_column_text(hStmt, 30, net->lan1_ip6_address, sizeof(net->lan1_ip6_address));
		sqlite_query_column_text(hStmt, 31, net->lan1_ip6_netmask, sizeof(net->lan1_ip6_netmask));
		sqlite_query_column_text(hStmt, 32, net->lan1_ip6_gateway, sizeof(net->lan1_ip6_gateway));
		sqlite_query_column_text(hStmt, 33, net->lan2_ip6_address, sizeof(net->lan2_ip6_address));
		sqlite_query_column_text(hStmt, 34, net->lan2_ip6_netmask, sizeof(net->lan2_ip6_netmask));
		sqlite_query_column_text(hStmt, 35, net->lan2_ip6_gateway, sizeof(net->lan2_ip6_gateway));
	    sqlite_query_column(hStmt, 36, &(net->lan1_ip6_dhcp));
	    sqlite_query_column(hStmt, 37, &(net->lan2_ip6_dhcp));
	    sqlite_query_column(hStmt, 38, &(net->bond0_ip6_dhcp));
	    sqlite_query_column_text(hStmt, 39, net->bond0_ip6_address, sizeof(net->bond0_ip6_address));
	    sqlite_query_column_text(hStmt, 40, net->bond0_ip6_netmask, sizeof(net->bond0_ip6_netmask));
	    sqlite_query_column_text(hStmt, 41, net->bond0_ip6_gateway, sizeof(net->bond0_ip6_gateway));//end
	}

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_network(const char *pDbFile, struct network *net)
{
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	//david.milesight
	char sExec[2048] = {0};
	//snprintf(sExec, sizeof(sExec), "update network set mode=%d, host_name='%s', bond0_primary_net=%d, bond0_enable=%d, bond0_type=%d, bond0_ip_address='%s', bond0_netmask='%s', bond0_gateway='%s', bond0_primary_dns='%s', bond0_second_dns='%s', bond0_mtu=%d, lan1_enable=%d, lan1_type=%d, lan1_ip_address='%s', lan1_netmask='%s', lan1_gateway='%s',lan1_primary_dns='%s', lan1_second_dns='%s', lan1_mtu=%d, lan2_enable=%d, lan2_type=%d, lan2_ip_address='%s', lan2_netmask='%s', lan2_gateway='%s',lan2_primary_dns='%s', lan2_second_dns='%s', lan2_mtu=%d, lan1_dhcp_gateway='%s', lan2_dhcp_gateway='%s';", net->mode, net->host_name, net->bond0_primary_net, net->bond0_enable, net->bond0_type, net->bond0_ip_address, net->bond0_netmask, net->bond0_gateway, net->bond0_primary_dns, net->bond0_second_dns, net->bond0_mtu, net->lan1_enable, net->lan1_type, net->lan1_ip_address, net->lan1_netmask, net->lan1_gateway, net->lan1_primary_dns, net->lan1_second_dns, net->lan1_mtu, net->lan2_enable, net->lan2_type, net->lan2_ip_address, net->lan2_netmask, net->lan2_gateway, net->lan2_primary_dns, net->lan2_second_dns, net->lan2_mtu, net->lan1_dhcp_gateway, net->lan2_dhcp_gateway);
	snprintf(sExec, sizeof(sExec), "update network set mode=%d, host_name='%s', bond0_primary_net=%d, bond0_enable=%d, bond0_type=%d, bond0_ip_address='%s', bond0_netmask='%s', bond0_gateway='%s', bond0_primary_dns='%s', bond0_second_dns='%s', bond0_mtu=%d, lan1_enable=%d, lan1_type=%d, lan1_ip_address='%s', lan1_netmask='%s', lan1_gateway='%s',lan1_primary_dns='%s', lan1_second_dns='%s', lan1_mtu=%d, lan2_enable=%d, lan2_type=%d, lan2_ip_address='%s', lan2_netmask='%s', lan2_gateway='%s',lan2_primary_dns='%s', lan2_second_dns='%s', lan2_mtu=%d, lan1_dhcp_gateway='%s', lan2_dhcp_gateway='%s', lan1_ip6_address='%s', lan1_ip6_netmask='%s', lan1_ip6_gateway='%s', lan2_ip6_address='%s', lan2_ip6_netmask='%s', lan2_ip6_gateway='%s',lan1_ip6_dhcp=%d,lan2_ip6_dhcp=%d,bond0_ip6_dhcp=%d,bond0_ip6_address='%s',bond0_ip6_netmask='%s',bond0_ip6_gateway='%s';", net->mode, net->host_name, net->bond0_primary_net, net->bond0_enable, net->bond0_type, net->bond0_ip_address, net->bond0_netmask, net->bond0_gateway, net->bond0_primary_dns, net->bond0_second_dns, net->bond0_mtu, net->lan1_enable, net->lan1_type, net->lan1_ip_address, net->lan1_netmask, net->lan1_gateway, net->lan1_primary_dns, net->lan1_second_dns, net->lan1_mtu, net->lan2_enable, net->lan2_type, net->lan2_ip_address, net->lan2_netmask, net->lan2_gateway, net->lan2_primary_dns, net->lan2_second_dns, net->lan2_mtu, net->lan1_dhcp_gateway, net->lan2_dhcp_gateway, net->lan1_ip6_address, net->lan1_ip6_netmask, net->lan1_ip6_gateway, net->lan2_ip6_address, net->lan2_ip6_netmask, net->lan2_ip6_gateway, net->lan1_ip6_dhcp, net->lan2_ip6_dhcp, net->bond0_ip6_dhcp, net->bond0_ip6_address, net->bond0_ip6_netmask, net->bond0_ip6_gateway);//hrz.milesight

    if (sqlite_execute(hConn, sExec) != 0) {
		snprintf(sExec, sizeof(sExec), "update network set mode=%d, host_name='%s', bond0_primary_net=%d, bond0_enable=%d, bond0_type=%d, bond0_ip_address='%s', bond0_netmask='%s', bond0_gateway='%s', bond0_primary_dns='%s', bond0_second_dns='%s', bond0_mtu=%d, lan1_enable=%d, lan1_type=%d, lan1_ip_address='%s', lan1_netmask='%s', lan1_gateway='%s',lan1_primary_dns='%s', lan1_second_dns='%s', lan1_mtu=%d, lan2_enable=%d, lan2_type=%d, lan2_ip_address='%s', lan2_netmask='%s', lan2_gateway='%s',lan2_primary_dns='%s', lan2_second_dns='%s', lan2_mtu=%d, lan1_dhcp_gateway='%s', lan2_dhcp_gateway='%s';", net->mode, net->host_name, net->bond0_primary_net, net->bond0_enable, net->bond0_type, net->bond0_ip_address, net->bond0_netmask, net->bond0_gateway, net->bond0_primary_dns, net->bond0_second_dns, net->bond0_mtu, net->lan1_enable, net->lan1_type, net->lan1_ip_address, net->lan1_netmask, net->lan1_gateway, net->lan1_primary_dns, net->lan1_second_dns, net->lan1_mtu, net->lan2_enable, net->lan2_type, net->lan2_ip_address, net->lan2_netmask, net->lan2_gateway, net->lan2_primary_dns, net->lan2_second_dns, net->lan2_mtu, net->lan1_dhcp_gateway, net->lan2_dhcp_gateway);
		sqlite_execute(hConn, sExec);
	}
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_network_mode(const char *pDbFile, int *pMode)
{
	if(!pDbFile || !pMode) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select mode from network;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, pMode);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_network_bond(const char *pDbFile, struct network *net)
{
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[1024] = {0};
	//snprintf(sQuery, sizeof(sQuery), "select mode, host_name, bond0_primary_net, bond0_enable, bond0_type, bond0_ip_address, bond0_netmask, bond0_gateway, bond0_primary_dns, bond0_second_dns, bond0_mtu from network;");
	snprintf(sQuery, sizeof(sQuery), "select mode, host_name, bond0_primary_net, bond0_enable, bond0_type, bond0_ip_address, bond0_netmask, bond0_gateway, bond0_primary_dns, bond0_second_dns, bond0_mtu, bond0_ip6_dhcp, bond0_ip6_address, bond0_ip6_netmask, bond0_ip6_gateway from network;");//hrz.milesight
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(net->mode));
    sqlite_query_column_text(hStmt, 1, net->host_name, sizeof(net->host_name));
    sqlite_query_column(hStmt, 2, &(net->bond0_primary_net));
    sqlite_query_column(hStmt, 3, &(net->bond0_enable));
    sqlite_query_column(hStmt, 4, &(net->bond0_type));
    sqlite_query_column_text(hStmt, 5, net->bond0_ip_address, sizeof(net->bond0_ip_address));
    sqlite_query_column_text(hStmt, 6, net->bond0_netmask, sizeof(net->bond0_netmask));
    sqlite_query_column_text(hStmt, 7, net->bond0_gateway, sizeof(net->bond0_gateway));
    sqlite_query_column_text(hStmt, 8, net->bond0_primary_dns, sizeof(net->bond0_primary_dns));
    sqlite_query_column_text(hStmt, 9, net->bond0_second_dns, sizeof(net->bond0_second_dns));
    sqlite_query_column(hStmt, 10, &(net->bond0_mtu));
    
    sqlite_query_column(hStmt, 11, &(net->bond0_ip6_dhcp));//hrz.milesight
    sqlite_query_column_text(hStmt, 12, net->bond0_ip6_address, sizeof(net->bond0_ip6_address));
    sqlite_query_column_text(hStmt, 13, net->bond0_ip6_netmask, sizeof(net->bond0_ip6_netmask));
    sqlite_query_column_text(hStmt, 14, net->bond0_ip6_gateway, sizeof(net->bond0_ip6_gateway));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_network_muti(const char *pDbFile, struct network *net)
{
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[1024] = {0};
	//snprintf(sQuery, sizeof(sQuery), "select mode, host_name, lan1_enable, lan1_type, lan1_ip_address, lan1_netmask, lan1_gateway, lan1_primary_dns, lan1_second_dns, lan1_mtu, lan2_enable, lan2_type, lan2_ip_address, lan2_netmask, lan2_gateway, lan2_primary_dns, lan2_second_dns, lan2_mtu from network;");
	snprintf(sQuery, sizeof(sQuery), "select mode, host_name, lan1_enable, lan1_type, lan1_ip_address, lan1_netmask, lan1_gateway, lan1_primary_dns, lan1_second_dns, lan1_mtu, lan2_enable, lan2_type, lan2_ip_address, lan2_netmask, lan2_gateway, lan2_primary_dns, lan2_second_dns, lan2_mtu, lan1_ip6_address, lan1_ip6_netmask, lan1_ip6_gateway, lan2_ip6_address, lan2_ip6_netmask, lan2_ip6_gateway, lan1_ip6_dhcp, lan2_ip6_dhcp from network;");//hrz.milesight
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(net->mode));
    sqlite_query_column_text(hStmt, 1, net->host_name, sizeof(net->host_name));
    sqlite_query_column(hStmt, 2, &(net->lan1_enable));
    sqlite_query_column(hStmt, 3, &(net->lan1_type));
    sqlite_query_column_text(hStmt, 4, net->lan1_ip_address, sizeof(net->lan1_ip_address));
    sqlite_query_column_text(hStmt, 5, net->lan1_netmask, sizeof(net->lan1_netmask));
    sqlite_query_column_text(hStmt, 6, net->lan1_gateway, sizeof(net->lan1_gateway));
    sqlite_query_column_text(hStmt, 7, net->lan1_primary_dns, sizeof(net->lan1_primary_dns));
    sqlite_query_column_text(hStmt, 8, net->lan1_second_dns, sizeof(net->lan1_second_dns));
    sqlite_query_column(hStmt, 9, &(net->lan1_mtu));

    sqlite_query_column(hStmt, 10, &(net->lan2_enable));
    sqlite_query_column(hStmt, 11, &(net->lan2_type));
    sqlite_query_column_text(hStmt, 12, net->lan2_ip_address, sizeof(net->lan2_ip_address));
    sqlite_query_column_text(hStmt, 13, net->lan2_netmask, sizeof(net->lan2_netmask));
    sqlite_query_column_text(hStmt, 14, net->lan2_gateway, sizeof(net->lan2_gateway));
    sqlite_query_column_text(hStmt, 15, net->lan2_primary_dns, sizeof(net->lan2_primary_dns));
    sqlite_query_column_text(hStmt, 16, net->lan2_second_dns, sizeof(net->lan2_second_dns));
    sqlite_query_column(hStmt, 17, &(net->lan2_mtu));

	sqlite_query_column_text(hStmt, 18, net->lan1_ip6_address, sizeof(net->lan1_ip6_address));//hrz.milesight
	sqlite_query_column_text(hStmt, 19, net->lan1_ip6_netmask, sizeof(net->lan1_ip6_netmask));
	sqlite_query_column_text(hStmt, 20, net->lan1_ip6_gateway, sizeof(net->lan1_ip6_gateway));
	sqlite_query_column_text(hStmt, 21, net->lan2_ip6_address, sizeof(net->lan2_ip6_address));
	sqlite_query_column_text(hStmt, 22, net->lan2_ip6_netmask, sizeof(net->lan2_ip6_netmask));
	sqlite_query_column_text(hStmt, 23, net->lan2_ip6_gateway, sizeof(net->lan2_ip6_gateway));
    sqlite_query_column(hStmt, 24, &(net->lan1_ip6_dhcp));
    sqlite_query_column(hStmt, 25, &(net->lan2_ip6_dhcp));//end
    

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_network_bond(const char *pDbFile, struct network *net)
{
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[1024] = {0};
	snprintf(sExec, sizeof(sExec), "update network set mode=%d,host_name='%s', bond0_primary_net=%d, bond0_enable=%d, bond0_type=%d, bond0_ip_address='%s', bond0_netmask='%s', bond0_gateway='%s', bond0_primary_dns='%s', bond0_second_dns='%s', bond0_mtu=%d, bond0_ip6_dhcp=%d, bond0_ip6_address='%s', bond0_ip6_netmask='%s', bond0_ip6_gateway='%s';", net->mode, net->host_name, net->bond0_primary_net, net->bond0_enable, net->bond0_type, net->bond0_ip_address, net->bond0_netmask, net->bond0_gateway, net->bond0_primary_dns, net->bond0_second_dns, net->bond0_mtu, net->bond0_ip6_dhcp, net->bond0_ip6_address, net->bond0_ip6_netmask, net->bond0_ip6_gateway);//hrz.milesight

    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_network_muti(const char *pDbFile, struct network *net)
{
	if(!pDbFile || !net) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[1024] = {0};
	//snprintf(sExec, sizeof(sExec), "update network set mode=%d, host_name='%s', lan1_enable=%d, lan1_type=%d, lan1_ip_address='%s', lan1_netmask='%s', lan1_gateway='%s',lan1_primary_dns='%s', lan1_second_dns='%s', lan1_mtu=%d, lan2_enable=%d, lan2_type=%d, lan2_ip_address='%s', lan2_netmask='%s', lan2_gateway='%s',lan2_primary_dns='%s', lan2_second_dns='%s', lan2_mtu=%d ;", net->mode, net->host_name, net->lan1_enable, net->lan1_type, net->lan1_ip_address, net->lan1_netmask, net->lan1_gateway, net->lan1_primary_dns, net->lan1_second_dns, net->lan1_mtu, net->lan2_enable, net->lan2_type, net->lan2_ip_address, net->lan2_netmask, net->lan2_gateway, net->lan2_primary_dns, net->lan2_second_dns, net->lan2_mtu);
	snprintf(sExec, sizeof(sExec), "update network set mode=%d, host_name='%s', lan1_enable=%d, lan1_type=%d, lan1_ip_address='%s', lan1_netmask='%s', lan1_gateway='%s',lan1_primary_dns='%s', lan1_second_dns='%s', lan1_mtu=%d, lan2_enable=%d, lan2_type=%d, lan2_ip_address='%s', lan2_netmask='%s', lan2_gateway='%s',lan2_primary_dns='%s', lan2_second_dns='%s', lan2_mtu=%d, lan1_ip6_address='%s', lan1_ip6_netmask='%s', lan1_ip6_gateway='%s', lan2_ip6_address='%s', lan2_ip6_netmask='%s', lan2_ip6_gateway='%s', lan1_ip6_dhcp=%d, lan2_ip6_dhcp=%d;", net->mode, net->host_name, net->lan1_enable, net->lan1_type, net->lan1_ip_address, net->lan1_netmask, net->lan1_gateway, net->lan1_primary_dns, net->lan1_second_dns, net->lan1_mtu, net->lan2_enable, net->lan2_type, net->lan2_ip_address, net->lan2_netmask, net->lan2_gateway, net->lan2_primary_dns, net->lan2_second_dns, net->lan2_mtu, net->lan1_ip6_address, net->lan1_ip6_netmask, net->lan1_ip6_gateway, net->lan2_ip6_address, net->lan2_ip6_netmask, net->lan2_ip6_gateway,net->lan1_ip6_dhcp, net->lan2_ip6_dhcp);//hrz.milesight

    	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_port(const char *pDbFile, struct network_more *more)
{
	if(!pDbFile || !more) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select enable_ssh, ssh_port, http_port, rtsp_port, sdk_port, url, url_enable, https_port from network_more;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(more->enable_ssh));
    sqlite_query_column(hStmt, 1, &(more->ssh_port));
    sqlite_query_column(hStmt, 2, &(more->http_port));
    sqlite_query_column(hStmt, 3, &(more->rtsp_port));
    sqlite_query_column(hStmt, 4, &(more->sdk_port));
    sqlite_query_column_text(hStmt, 5, more->url, sizeof(more->url));
    sqlite_query_column(hStmt, 6, &(more->url_enable));
    sqlite_query_column(hStmt, 7, &(more->https_port));
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_port(const char *pDbFile, struct network_more *more)
{
	if(!pDbFile || !more) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update network_more set enable_ssh=%d, ssh_port=%d, http_port=%d, rtsp_port=%d, sdk_port=%d, url='%s', url_enable=%d, https_port=%d;",
			more->enable_ssh, more->ssh_port, more->http_port, more->rtsp_port, more->sdk_port, more->url, more->url_enable,more->https_port);

    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_snmp(const char *pDbFile, struct snmp *snmp)
{
	if(!pDbFile || !snmp) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[2048] = {0};
 
	snprintf(sQuery, sizeof(sQuery), "select v1_enable, v2c_enable, write_community, read_community, v3_enable, read_name, read_level, read_auth_type, read_auth_psw, read_pri_type, read_pri_psw, write_name, write_level, write_auth_type, write_auth_psw, write_pri_type, write_pri_psw, port from snmp;");//hrz.milesight
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if (nResult != 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    
    sqlite_query_column(hStmt, 0, &(snmp->v1_enable));
    sqlite_query_column(hStmt, 1, &(snmp->v2c_enable));
    sqlite_query_column_text(hStmt, 2, snmp->write_community, sizeof(snmp->write_community));
    sqlite_query_column_text(hStmt, 3, snmp->read_community, sizeof(snmp->read_community));
    
    sqlite_query_column(hStmt, 4, &(snmp->v3_enable));
    sqlite_query_column_text(hStmt, 5, snmp->read_security_name, sizeof(snmp->read_security_name));
    sqlite_query_column(hStmt, 6, &(snmp->read_level_security));
    sqlite_query_column(hStmt, 7, &(snmp->read_auth_algorithm));
    sqlite_query_column_text(hStmt, 8, snmp->read_auth_password, sizeof(snmp->read_auth_password));
    sqlite_query_column(hStmt, 9, &(snmp->read_pri_algorithm));
    sqlite_query_column_text(hStmt, 10, snmp->read_pri_password, sizeof(snmp->read_pri_password));

    sqlite_query_column_text(hStmt, 11, snmp->write_security_name, sizeof(snmp->write_security_name));
    sqlite_query_column(hStmt, 12, &(snmp->write_level_security));
    sqlite_query_column(hStmt, 13, &(snmp->write_auth_algorithm));
    sqlite_query_column_text(hStmt, 14, snmp->write_auth_password, sizeof(snmp->write_auth_password));
    sqlite_query_column(hStmt, 15, &(snmp->write_pri_algorithm));
    sqlite_query_column_text(hStmt, 16, snmp->write_pri_password, sizeof(snmp->write_pri_password));
   
    sqlite_query_column(hStmt, 17, &(snmp->port));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_snmp(const char *pDbFile, struct snmp *snmp)
{
	if(!pDbFile || !snmp) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	char sExec[2048] = {0};
	snprintf(sExec, sizeof(sExec), "update snmp set v1_enable=%d, v2c_enable=%d, write_community='%s', read_community='%s', v3_enable=%d, read_name='%s', read_level=%d, read_auth_type=%d, read_auth_psw='%s', read_pri_type=%d, read_pri_psw='%s', write_name='%s', write_level=%d, write_auth_type=%d, write_auth_psw='%s', write_pri_type=%d, write_pri_psw='%s', port=%d;", snmp->v1_enable, snmp->v2c_enable, snmp->write_community, snmp->read_community, snmp->v3_enable, snmp->read_security_name, snmp->read_level_security, snmp->read_auth_algorithm, snmp->read_auth_password, snmp->read_pri_algorithm, snmp->read_pri_password, snmp->write_security_name, snmp->write_level_security, snmp->write_auth_algorithm, snmp->write_auth_password, snmp->write_pri_algorithm, snmp->write_pri_password, snmp->port);//hrz.milesight
    
    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


int read_audio_ins(const char *pDbFile, struct audio_in audioins[MAX_AUDIOIN], int *pCnt)
{
	if(!pDbFile || !audioins) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id, enable, samplerate, volume from audio_in;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
		*pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int i = 0;
    for(i=0; i<MAX_AUDIOIN; i++)
    {
        sqlite_query_column(hStmt, 0, &audioins[i].id);
        sqlite_query_column(hStmt, 1, &audioins[i].enable);
        sqlite_query_column(hStmt, 2, &audioins[i].samplerate);
        sqlite_query_column(hStmt, 3, &audioins[i].volume);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
	*pCnt = i;
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_audio_in(const char *pDbFile, struct audio_in *audioin)
{
    if(!pDbFile || !audioin) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update audio_in set enable=%d, samplerate=%d,volume=%d where id=%d;", audioin->enable, audioin->samplerate, audioin->volume, audioin->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_layout(const char *pDbFile, struct layout *layout)
{
	if(!pDbFile || !layout) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select main_output, sub_output, main_layout_mode, sub_layout_mode from layout;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &(layout->main_output));
    sqlite_query_column(hStmt, 1, &(layout->sub_output));
    sqlite_query_column(hStmt, 2, &(layout->main_layout_mode));
    sqlite_query_column(hStmt, 3, &(layout->sub_layout_mode));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_layout(const char *pDbFile, struct layout *layout)
{
	if(!pDbFile || !layout) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};
	snprintf(sExec, sizeof(sExec), "update layout set main_output=%d, sub_output=%d, main_layout_mode=%d, sub_layout_mode=%d;", layout->main_output, layout->sub_output, layout->main_layout_mode, layout->sub_layout_mode);

    sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


int read_ptz_speeds(const char *pDbFile, struct ptz_speed ptzspeeds[MAX_CAMERA], int *pCnt)
{
	if(!pDbFile || !ptzspeeds) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,pan,tilt,zoom,focus,timeout from ptz_speed;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
		*pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int i = 0;
    for(i=0; i<MAX_CAMERA; i++)
    {
        sqlite_query_column(hStmt, 0, &ptzspeeds[i].id);
        sqlite_query_column(hStmt, 1, &ptzspeeds[i].pan);
        sqlite_query_column(hStmt, 2, &ptzspeeds[i].tilt);
        sqlite_query_column(hStmt, 3, &ptzspeeds[i].zoom);
        sqlite_query_column(hStmt, 4, &ptzspeeds[i].focus);
        sqlite_query_column(hStmt, 5, &ptzspeeds[i].timeout);
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
	*pCnt = i;
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_ptz_speed(const char *pDbFile, struct ptz_speed *ptzspeed, int chn_id)
{
	if(!pDbFile || !ptzspeed || chn_id<0) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,pan,tilt,zoom,focus,timeout from ptz_speed where id=%d;", chn_id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    sqlite_query_column(hStmt, 0, &ptzspeed->id);
    sqlite_query_column(hStmt, 1, &ptzspeed->pan);
    sqlite_query_column(hStmt, 2, &ptzspeed->tilt);
    sqlite_query_column(hStmt, 3, &ptzspeed->zoom);
    sqlite_query_column(hStmt, 4, &ptzspeed->focus);
    sqlite_query_column(hStmt, 5, &ptzspeed->timeout);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_ptz_speed(const char *pDbFile, struct ptz_speed *ptzspeed)
{
    if(!pDbFile || !ptzspeed) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update ptz_speed set pan=%d,tilt=%d,zoom=%d,focus=%d,timeout=%d where id=%d;", ptzspeed->pan, ptzspeed->tilt, ptzspeed->zoom, ptzspeed->focus, ptzspeed->timeout, ptzspeed->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_storages(const char *pDbFile, struct storage storages[SATA_MAX], int *pCnt)
{
    if(!pDbFile || !storages) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id, recycle_mode from storage;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
		*pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    for(i=0; i<SATA_MAX; i++)
    {
        sqlite_query_column(hStmt, 0, &(storages[i].id));
        sqlite_query_column(hStmt, 1, &(storages[i].recycle_mode));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
	*pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_storage(const char *pDbFile, struct storage *storage, int port_id)
{
    if(!pDbFile || !storage || port_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id, recycle_mode from storage where id=%d;", port_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &(storage->id));
    sqlite_query_column(hStmt, 1, &(storage->recycle_mode));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_storage(const char *pDbFile, struct storage *storage)
{
    if(!pDbFile || !storage) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update storage set recycle_mode=%d where id=%d;", storage->recycle_mode, storage->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_ptz_tour(const char *pDbFile, struct ptz_tour tour[TOUR_MAX], int ptz_id)
{
    if(!pDbFile || !tour || ptz_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select tour_id, key_id, preset_id, timeout, speed from ptz_tour where ptz_id=%d;", ptz_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int tour_id = 0;
    int key_id = 0;
    int loop_cnt = TOUR_MAX*KEY_MAX;
    for(i=0; i<=loop_cnt; i++)
    {
        sqlite_query_column(hStmt, 0, &tour_id);
        sqlite_query_column(hStmt, 1, &key_id);
        sqlite_query_column(hStmt, 2, &(tour[tour_id].keys[key_id].preset_id));
        sqlite_query_column(hStmt, 3, &(tour[tour_id].keys[key_id].timeout));
        sqlite_query_column(hStmt, 4, &(tour[tour_id].keys[key_id].speed));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_ptz_tour(const char *pDbFile, struct ptz_tour tour[TOUR_MAX], int ptz_id)
{
    if(!pDbFile || !tour || ptz_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from ptz_tour where ptz_id=%d;", ptz_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<TOUR_MAX; i++)
    {
        for(j=0; j<KEY_MAX; j++)
        {
            // preset_id start from 1, if eq 0 means it didn't be set
            if(tour[i].keys[j].preset_id == 0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into ptz_tour values(%d,%d,%d,%d,%d,%d);", ptz_id, i, j, tour[i].keys[j].preset_id, tour[i].keys[j].timeout, tour[i].keys[j].speed);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int get_param_value(const char *pDbFile, const char *key, char *value, int value_len, const char *sdefault)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
		snprintf(value, value_len, "%s", sdefault);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
		snprintf(value, value_len, "%s", sdefault);
        return -1;
    }
    sqlite_query_column_text(hStmt, 0, value, value_len);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int get_param_int(const char *pDbFile, const char *key, int defvalue)
{
	char value[128] = {0};
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return defvalue;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return defvalue;
    }
    sqlite_query_column_text(hStmt, 0, value, sizeof(value));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return atoi(value);
}

int set_param_value(const char *pDbFile, const char *key, char *value)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char buf[128] = {0};
	if (strstr(key, PARAM_POE_PSD))
	{
		translate_pwd(buf,value,strlen(value));	
		snprintf(sExec, sizeof(sExec), "update params set value ='%s' where name='%s';", buf, key);
	}
	else
	{
		snprintf(sExec, sizeof(sExec), "update params set value ='%s' where name='%s';", value, key);
	}
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int set_param_int(const char *pDbFile, const char *key, int value)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update params set value ='%d' where name='%s';", value, key);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;	
}

int add_param_value(const char *pDbFile, const char *key, char *value)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "insert into params(name,value) values('%s','%s');", key, value);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int add_param_int(const char *pDbFile, const char *key, int value)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "insert into params(name,value) values('%s','%d');", key, value);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int check_param_key(const char *pDbFile, const char *key)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);

	return 0;	
}

//params_oem
int get_param_oem_value(const char *pDbFile, const char *key, char *value, int value_len, const char *sdefault)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
		snprintf(value, value_len, "%s", sdefault);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params_oem where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
		snprintf(value, value_len, "%s", sdefault);
        return -1;
    }
    sqlite_query_column_text(hStmt, 0, value, value_len);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int get_param_oem_int(const char *pDbFile, const char *key, int defvalue)
{
	char value[128] = {0};
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return defvalue;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params_oem where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return defvalue;
    }
    sqlite_query_column_text(hStmt, 0, value, sizeof(value));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return atoi(value);
}

int set_param_oem_value(const char *pDbFile, const char *key, char *value)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update params_oem set value ='%s' where name='%s';", value, key);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int set_param_oem_int(const char *pDbFile, const char *key, int value)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update params_oem set value ='%d' where name='%s';", value, key);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;	
}

int add_param_oem_value(const char *pDbFile, const char *key, char *value)
{
    if(!pDbFile || !key || !value) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "insert into params_oem(name,value) values('%s','%s');", key, value);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int add_param_oem_int(const char *pDbFile, const char *key, int value)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "insert into params_oem(name,value) values('%s','%d');", key, value);
    sqlite_execute(hConn, sExec);

    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int check_param_oem_key(const char *pDbFile, const char *key)
{
    if(!pDbFile || !key) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    snprintf(sQuery, sizeof(sQuery), "select value from params_oem where name='%s';", key);
    int nColumnCnt = 0;
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);

	return 0;	
}


/* ipc protocol */
int read_ipc_protocols(const char *pDbFile, struct ipc_protocol **protocols, int *pCnt)
{
	if (!pDbFile || !pCnt || !protocols)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = NULL;
	HSQLSTMT hStmt = NULL;
	HSQLSTMT qStmt = NULL;
	int ret, cols, rows = 0, flag = -1, i = 0;
	struct ipc_protocol *ppro = NULL;
	char buf[256] = {0};

	do {
		if (sqlite_conn(pDbFile, &hConn)) {
 			break;
		}
#if 1
		snprintf(buf, sizeof(buf), "select count(*) from protocol;");
		ret = sqlite_query_record(hConn, buf, &hStmt, &cols);
		if (ret != 0 || cols == 0) {
 			break;
		}
		if (sqlite_query_column(hStmt, 0, &rows) || !rows){
 			break;
		}
 		sqlite_clear_stmt(hStmt);
		hStmt = NULL;
#endif
		snprintf(buf, sizeof(buf), "select pro_id,pro_name,function,enable,display_model from protocol;");
		ret = sqlite_query_record(hConn, buf, &qStmt, &cols);
		if (ret != 0 || cols == 0) {
 			break;
		}
		if ((ppro = ms_calloc(rows, sizeof(struct ipc_protocol))) == NULL) {
			break;
		}
		do {
			sqlite_query_column(qStmt, 0, &ppro[i].pro_id);
			sqlite_query_column_text(qStmt, 1, ppro[i].pro_name, sizeof(ppro[i].pro_name));
			sqlite_query_column(qStmt, 2, &ppro[i].function);
			sqlite_query_column(qStmt, 3, &ppro[i].enable);
			sqlite_query_column(qStmt, 4, &ppro[i].display_model);
			i++;
		} while (sqlite_query_next(qStmt) == 0 && i < rows);
		flag = 0;
		*pCnt = rows;
		*protocols = ppro;
	} while (0);
	if (hStmt)
		sqlite_clear_stmt(hStmt);
	if (qStmt)
		sqlite_clear_stmt(qStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return flag;
}

int read_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol, int pro_id)
{
	if (!pDbFile || !protocol)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[256] = { 0 };
	snprintf(sQuery, sizeof(sQuery), "select pro_id,pro_name,function,enable,display_model from protocol where pro_id=%d;", pro_id);
	int nColumnCnt = 0;
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if (nResult != 0 || nColumnCnt == 0) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	sqlite_query_column(hStmt, 0, &protocol->pro_id);
	sqlite_query_column_text(hStmt, 1, protocol->pro_name, sizeof(protocol->pro_name));
	sqlite_query_column(hStmt, 2, &protocol->function);
	sqlite_query_column(hStmt, 3, &protocol->enable);
	sqlite_query_column(hStmt, 4, &protocol->display_model);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol)
{
    if(!pDbFile || !protocol) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update protocol set pro_name='%s', function=%d, enable=%d, display_model=%d where pro_id=%d;", protocol->pro_name, protocol->function, protocol->enable, protocol->display_model, protocol->pro_id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	return 0;
}

int delete_ipc_protocols(const char *pDbFile)
{
    if(!pDbFile) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from protocol;");
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	return 0;
}

int insert_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol)
{
    if(!pDbFile || !protocol) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "insert into protocol(pro_id,pro_name,function,enable,display_model) values(%d, '%s', %d, %d, %d);", protocol->pro_id, protocol->pro_name, protocol->function, protocol->enable, protocol->display_model);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	return 0;
}

/* ipc model */
int read_ipc_models(const char *pDbFile, struct ipc_model **models, int *pCnt)
{
	if (!pDbFile || !pCnt || !models)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = NULL;
	HSQLSTMT hStmt = NULL;
	HSQLSTMT qStmt = NULL;
	int ret, cols, rows = 0, flag = -1, i = 0;
	struct ipc_model *pmod = NULL;
	char buf[256] = { 0 };

	do {
		if (sqlite_conn(pDbFile, &hConn)) {
			break;
		}
		snprintf(buf, sizeof(buf), "select count(*) from model;");
		ret = sqlite_query_record(hConn, buf, &hStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}
		if (sqlite_query_column(hStmt, 0, &rows) || !rows)  
			break;
		snprintf(buf, sizeof(buf), "select mod_id,pro_id,mod_name,alarm_type,default_model from model;");
		ret = sqlite_query_record(hConn, buf, &qStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}
		if ((pmod = ms_calloc(rows, sizeof(struct ipc_model))) == NULL ) {
			break;
		}
		do {
			sqlite_query_column(qStmt, 0, &pmod[i].mod_id);
			sqlite_query_column(qStmt, 1, &pmod[i].pro_id);
			sqlite_query_column_text(qStmt, 2, pmod[i].mod_name, sizeof(pmod[i].mod_name));
			sqlite_query_column(qStmt, 3, &pmod[i].alarm_type);
			sqlite_query_column(qStmt, 4, &pmod[i].default_model);
			i++;
		} while (sqlite_query_next(qStmt) == 0 && i < rows);
		flag = 0;
		*pCnt = rows;
		*models = pmod;
	} while (0);
	if (hStmt)
		sqlite_clear_stmt(hStmt);
	if (qStmt)
		sqlite_clear_stmt(qStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return flag;
}

int read_ipc_models_by_protocol(const char *pDbFile, struct ipc_model **models, int pro_id, int *pCnt)
{
	if (!pDbFile || !pCnt || !models)
			return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = NULL;
	HSQLSTMT hStmt = NULL;
	HSQLSTMT qStmt = NULL;
	int ret, cols, rows = 0, flag = -1, i = 0;
	struct ipc_model *pmod = NULL;
	char buf[256] = { 0 };

	do {
		if (sqlite_conn(pDbFile, &hConn)) {
			break;
		}
		snprintf(buf, sizeof(buf),
				"select count(*) from model where pro_id=%d;", pro_id);
		ret = sqlite_query_record(hConn, buf, &hStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}
		if (sqlite_query_column(hStmt, 0, &rows) || !rows)  
			break;
		snprintf(buf, sizeof(buf),
				"select mod_id,pro_id,mod_name,alarm_type,default_model from model where pro_id=%d;",
				pro_id);
		ret = sqlite_query_record(hConn, buf, &qStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}
		if ((pmod = ms_calloc(rows, sizeof(struct ipc_model))) == NULL ) {
			break;
		}
		do {
			sqlite_query_column(qStmt, 0, &pmod[i].mod_id);
			sqlite_query_column(qStmt, 1, &pmod[i].pro_id);
			sqlite_query_column_text(qStmt, 2, pmod[i].mod_name, sizeof(pmod[i].mod_name));
			sqlite_query_column(qStmt, 3, &pmod[i].alarm_type);
			sqlite_query_column(qStmt, 4, &pmod[i].default_model);
			i++;
		} while (sqlite_query_next(qStmt) == 0 && i < rows);
		flag = 0;
		*pCnt = rows;
		*models = pmod;
	} while (0);
	if (hStmt)
		sqlite_clear_stmt(hStmt);
	if (qStmt)
		sqlite_clear_stmt(qStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return flag;
}

int read_ipc_model(const char *pDbFile, struct ipc_model *model, int mod_id)
{
	if (!pDbFile || !model)
			return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[256] = { 0 };
	snprintf(sQuery, sizeof(sQuery),
			"select mod_id,pro_id,mod_name,alarm_type,default_model from model where mod_id=%d;",
			mod_id);
	int nColumnCnt = 0;
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if (nResult != 0 || nColumnCnt == 0) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	sqlite_query_column(hStmt, 0, &model->mod_id);
	sqlite_query_column(hStmt, 1, &model->pro_id);
	sqlite_query_column_text(hStmt, 2, model->mod_name, sizeof(model->mod_name));
	sqlite_query_column(hStmt, 3, &model->alarm_type);
	sqlite_query_column(hStmt, 4, &model->default_model);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_ipc_default_model(const char *pDbFile, struct ipc_model *model, int pro_id)
{
	if (!pDbFile || !model)
			return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
 		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[256] = { 0 };
	snprintf(sQuery, sizeof(sQuery),
			"select mod_id,pro_id,mod_name,alarm_type,default_model from model where pro_id=%d and default_model=1;",
			pro_id);
	int nColumnCnt = 0;
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if (nResult != 0 || nColumnCnt == 0) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	sqlite_query_column(hStmt, 0, &model->mod_id);
	sqlite_query_column(hStmt, 1, &model->pro_id);
	sqlite_query_column_text(hStmt, 2, model->mod_name, sizeof(model->mod_name));
	sqlite_query_column(hStmt, 3, &model->alarm_type);
	sqlite_query_column(hStmt, 4, &model->default_model);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_ipc_model(const char *pDbFile, struct ipc_model *model)
{
    if(!pDbFile || !model) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update model set pro_id=%d, mod_name='%s', alarm_type=%d, default_model=%d where mod_id=%d;", model->pro_id, model->mod_name, model->alarm_type, model->default_model, model->mod_id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	return 0;
}

int insert_ipc_model(const char *pDbFile, struct ipc_model *model)
{
    if(!pDbFile || !model) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "insert into model(mod_id,pro_id,mod_name,alarm_type,default_model) values(%d, %d, '%s', %d, %d);", model->mod_id, model->pro_id, model->mod_name, model->alarm_type, model->default_model);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	return 0;	
}

int delete_ipc_models(const char *pDbFile)
{
	if(!pDbFile) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = {0};
	snprintf(sExec, sizeof(sExec), "delete from model;");
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;	
}

void ipc_destroy(void *ptr)
{
	ms_free(ptr);
}

int read_mosaic(const char *pDbFile, struct mosaic *mosaic)
{
	if (!pDbFile || !mosaic)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
//		perror("lock file");
		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char cmd[256] = {0};
	snprintf(cmd, sizeof(cmd), "select layout_mode,channels from mosaic where layout_mode=%d;", mosaic->layoutmode);
	if (sqlite_prepare_blob(hConn, cmd, &hStmt, 1)) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	sqlite_query_column(hStmt, 0, &mosaic->layoutmode);
	sqlite_query_column_blob(hStmt, 1, mosaic->channels, sizeof(mosaic->channels));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_mosaics(const char *pDbFile, struct mosaic mosaic[LAYOUT_MODE_MAX], int *pCnt)
{
	if (!pDbFile || !pCnt)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock)) < 0) {
		perror("lock file");
		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		perror("conn file");
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char cmd[256] = {0};
	snprintf(cmd, sizeof(cmd), "select layout_mode,channels from mosaic;");
	if (sqlite_prepare_blob(hConn, cmd, &hStmt, 1)) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		perror("sqlite_prepare_blob");
		return -1;
	}
	int i = 0;
	for (; i < LAYOUT_MODE_MAX; i++) {
		sqlite_query_column(hStmt, 0, &mosaic[i].layoutmode);
		sqlite_query_column_blob(hStmt, 1, mosaic[i].channels, sizeof(mosaic[i].channels));
		if (sqlite_query_next(hStmt)) {
			i++;
			break;
		}
	}
	*pCnt = i;
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_mosaic(const char *pDbFile, struct mosaic *mosaic)
{
	if (!pDbFile || !mosaic)
		return -1;
	int nFd;
	if ((nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock)) < 0) {
//		perror("lock file");
		return -1;
	}
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char cmd[256] = {0};
	snprintf(cmd, sizeof(cmd), "update mosaic set channels=? where layout_mode=%d;", mosaic->layoutmode);
	if (sqlite_prepare_blob(hConn, cmd, &hStmt, 0)) {
		if (hStmt)
			sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int flag = 0;
	if (sqlite_exec_blob(hStmt, 1, mosaic->channels, sizeof(mosaic->channels))) {
		flag = -1;
	}
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return flag;
}

int read_disk_maintain_info(const char *pDbFile, struct disk_maintain_info *info)
{
    if(!pDbFile) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select log,photo from disk_maintain");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &(info->log));
    sqlite_query_column(hStmt, 1, &(info->photo));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_disk_maintain_info(const char *pDbFile, struct disk_maintain_info *info)
{
    if(!pDbFile || !info) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update disk_maintain set log=%d,photo=%d", info->log, info->photo);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_p2p_info(const char *pDbFile, struct p2p_info *info)
{
    if(!pDbFile) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select enable,company,email,dealer,ipc from p2p");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &(info->enable));
    sqlite_query_column_text(hStmt, 1, info->company, sizeof(info->company));
    sqlite_query_column_text(hStmt, 2, info->email, sizeof(info->email));
    sqlite_query_column_text(hStmt, 3, info->dealer, sizeof(info->dealer));
    sqlite_query_column_text(hStmt, 4, info->ipc, sizeof(info->ipc));

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_p2p_info(const char *pDbFile, struct p2p_info *info)
{
    if(!pDbFile || !info) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "update p2p set enable=%d,company='%s',email='%s',dealer='%s',ipc='%s'", info->enable,info->company,info->email,info->dealer,info->ipc);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_privacy_mask(const char *pDbFile, struct privacy_mask *mask, int chn_id)
{
	if(!pDbFile || !mask) return -1;
	    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	    HSQLITE hConn = 0;
	    if(sqlite_conn(pDbFile, &hConn) != 0)
	    {
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    HSQLSTMT hStmt = 0;
	    char sQuery[272] = {0};
	    int nColumnCnt = 0;
	    int i = 0;
	    snprintf(sQuery, sizeof(sQuery), "select id,area_id,enable,start_x,start_y,area_width,area_height,width,height,fill_color from privacy_mask where id = %d order by(area_id);", chn_id);
	    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	    if(nResult !=0 || nColumnCnt == 0)
	    {
	        if(hStmt) sqlite_clear_stmt(hStmt);
	        sqlite_disconn(hConn);
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }

	    for(i = 0; i < MAX_MASK_AREA_NUM; i++)
	    {
	    	sqlite_query_column(hStmt, 0, &mask->id);
	    	sqlite_query_column(hStmt, 1, &mask->area[i].area_id);
	    	sqlite_query_column(hStmt, 2, &mask->area[i].enable);
	    	sqlite_query_column(hStmt, 3, &mask->area[i].start_x);
	    	sqlite_query_column(hStmt, 4, &mask->area[i].start_y);
	    	sqlite_query_column(hStmt, 5, &mask->area[i].area_width);
	    	sqlite_query_column(hStmt, 6, &mask->area[i].area_height);
	    	sqlite_query_column(hStmt, 7, &mask->area[i].width);
	    	sqlite_query_column(hStmt, 8, &mask->area[i].height);
	    	sqlite_query_column(hStmt, 9, &mask->area[i].fill_color);
			if(sqlite_query_next(hStmt) != 0){ break;}
	    }

	    sqlite_clear_stmt(hStmt);
	    sqlite_disconn(hConn);
	    FileUnlock(nFd, &global_rwlock);
	    return 0;
}

int read_privacy_masks(const char *pDbFile, struct privacy_mask mask[], int *pCnt, DB_TYPE o_flag)
{
	if(!pDbFile || !mask) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0, j = 0;
	int arer_num = 1;
    snprintf(sQuery, sizeof(sQuery), "select id,area_id,enable,start_x,start_y,area_width,area_height,width,height,fill_color from privacy_mask order by id,area_id;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	if(o_flag != DB_OLD)
		arer_num = MAX_MASK_AREA_NUM;
    for(i=0; i<MAX_CAMERA; i++)
    {
    	for(j = 0; j < arer_num; j++)
    	{
    		sqlite_query_column(hStmt, 0, &mask[i].id);
    		sqlite_query_column(hStmt, 1, &mask[i].area[j].area_id);
    		sqlite_query_column(hStmt, 2, &mask[i].area[j].enable);
    		sqlite_query_column(hStmt, 3, &mask[i].area[j].start_x);
    		sqlite_query_column(hStmt, 4, &mask[i].area[j].start_y);
    		sqlite_query_column(hStmt, 5, &mask[i].area[j].area_width);
    		sqlite_query_column(hStmt, 6, &mask[i].area[j].area_height);
    		sqlite_query_column(hStmt, 7, &mask[i].area[j].width);
    		sqlite_query_column(hStmt, 8, &mask[i].area[j].height);
    		sqlite_query_column(hStmt, 9, &mask[i].area[j].fill_color);
			if(sqlite_query_next(hStmt) != 0){j++; break;}
    	}
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_privacy_mask(const char *pDbFile, struct privacy_mask *mask)
{
	if(!pDbFile || !mask) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int i = 0;
	char sExec[512] = {0};
	for(i = 0; i < MAX_MASK_AREA_NUM; i++)
	{
		snprintf(sExec, sizeof(sExec), "update privacy_mask set enable=%d,start_x=%d,start_y=%d,area_width=%d,area_height=%d,width=%d,height=%d,fill_color=%d where id=%d and area_id=%d;", mask->area[i].enable, mask->area[i].start_x, mask->area[i].start_y, mask->area[i].area_width, mask->area[i].area_height, mask->area[i].width, mask->area[i].height, mask->area[i].fill_color, mask->id, i);
		sqlite_execute(hConn, sExec);
	}
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int insert_privacy_mask(const char *pDbFile, struct privacy_mask *mask)
{
	if(!pDbFile || !mask) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int i = 0;
	char sExec[1024] = {0};
	for(i = 0; i < /*MAX_MASK_AREA_NUM*/1; i++)
	{
		snprintf(sExec, sizeof(sExec), "insert into privacy_mask(id,area_id,enable,start_x,start_y,area_width,area_height,width,height,fill_color) values(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);", mask->id, mask->area[i].area_id, mask->area[i].enable, mask->area[i].start_x, mask->area[i].start_y, mask->area[i].area_width, mask->area[i].area_height, mask->area[i].width, mask->area[i].height, mask->area[i].fill_color);
		sqlite_execute(hConn, sExec);
	}
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_alarm_pids(const char *pDbFile, struct alarm_pid **info, int *pCnt)
{
	if(!pDbFile || !info) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = NULL;
	HSQLSTMT qStmt = NULL;
	int ret, cols, rows = 0, i = 0;
	struct alarm_pid *palarm = NULL;
	char buf[256] = { 0 };

	do {
		snprintf(buf, sizeof(buf), "select count(*) from token;");
		ret = sqlite_query_record(hConn, buf, &hStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}
		if (sqlite_query_column(hStmt, 0, &rows) || !rows) //获取记录数
			break;
		snprintf(buf, sizeof(buf), "select id,sn,enable,push_from, push_interval from token;");
		ret = sqlite_query_record(hConn, buf, &qStmt, &cols);
		if (ret != 0 || cols == 0) {
			break;
		}

		if ((palarm = ms_calloc(rows, sizeof(struct alarm_pid))) == NULL ) {
			break;
		}
		do {
			sqlite_query_column_text(qStmt, 0, palarm[i].id, sizeof(palarm[i].id));
			sqlite_query_column_text(qStmt, 1, palarm[i].sn, sizeof(palarm[i].sn));
			sqlite_query_column(qStmt, 2, &palarm[i].enable);
			sqlite_query_column(qStmt, 3, &palarm[i].from);
			sqlite_query_column(qStmt, 4, &palarm[i].push_interval);
			i++;
		} while (sqlite_query_next(qStmt) == 0 && i < rows);
		*pCnt = rows;
		*info = palarm;
	} while (0);
	if (hStmt)
		sqlite_clear_stmt(hStmt);
	if (qStmt)
		sqlite_clear_stmt(qStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_alarm_pid(const char *pDbFile, struct alarm_pid *info)
{
	if(!pDbFile) return -1;
	    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	    HSQLITE hConn = 0;
	    if(sqlite_conn(pDbFile, &hConn) != 0)
	    {
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    HSQLSTMT hStmt = 0;
	    char sQuery[512] = {0};
	    int nColumnCnt = 0;
	    snprintf(sQuery, sizeof(sQuery), "select id,sn,enable,push_from, push_interval from token where id='%s';", info->id);
//	    printf("read_alarm_pid: cmd: %s\n", sQuery);
	    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	    if(nResult !=0 || nColumnCnt == 0)
	    {
	        if(hStmt) sqlite_clear_stmt(hStmt);
	        sqlite_disconn(hConn);
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }

	    sqlite_query_column_text(hStmt, 0, info->id, sizeof(info->id));
	    sqlite_query_column_text(hStmt, 1, info->sn, sizeof(info->sn));
	    sqlite_query_column(hStmt, 2, &info->enable);
	    sqlite_query_column(hStmt, 3, &info->from);
	    sqlite_query_column(hStmt, 4, &info->push_interval);


	    sqlite_clear_stmt(hStmt);
	    sqlite_disconn(hConn);
	    FileUnlock(nFd, &global_rwlock);
	    return 0;
}

int write_alarm_pid(const char *pDbFile, struct alarm_pid *info)
{
	if(!pDbFile || !info) return -1;
	    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	    HSQLITE hConn = 0;
	    if(sqlite_conn(pDbFile, &hConn) != 0)
	    {
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    char sExec[256] = {0};
	    snprintf(sExec, sizeof(sExec), "insert into token values('%s','%s', %d, %d, %d);",
	    		info->id,info->sn,info->enable, info->from, info->push_interval);
	    sqlite_execute(hConn, sExec);
	    sqlite_disconn(hConn);
	    FileUnlock(nFd, &global_rwlock);
	    return 0;
}

int update_alarm_pid(const char *pDbFile, struct alarm_pid *info)
{
	if(!pDbFile || !info) return -1;
	    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	    HSQLITE hConn = 0;
	    if(sqlite_conn(pDbFile, &hConn) != 0)
	    {
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    char sExec[256] = {0};
	    snprintf(sExec, sizeof(sExec), "update token set enable=%d,push_from=%d, push_interval=%d where id='%s';",
	    		info->enable, info->from, info->push_interval,info->id);
//	    printf("cmd is: %s\n", sExec);
	    sqlite_execute(hConn, sExec);
	    sqlite_disconn(hConn);
	    FileUnlock(nFd, &global_rwlock);
	    return 0;
}

int read_trigger_alarms(const char *pDbFile, struct trigger_alarms *pa)
{
	  if(!pDbFile) return -1;
	    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	    HSQLITE hConn = 0;
	    if(sqlite_conn(pDbFile, &hConn) != 0)
	    {
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    HSQLSTMT hStmt = 0;
	    char sQuery[256] = {0};
	    int nColumnCnt = 0;
	    snprintf(sQuery, sizeof(sQuery), "select network_disconn,disk_full,record_fail,disk_fail,disk_unformat,no_disk from trigger_alarms;");
	    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	    if(nResult !=0 || nColumnCnt == 0)
	    {
	        if(hStmt) sqlite_clear_stmt(hStmt);
	        sqlite_disconn(hConn);
	        FileUnlock(nFd, &global_rwlock);
	        return -1;
	    }
	    sqlite_query_column(hStmt, 0, &pa->network_disconn);
	    sqlite_query_column(hStmt, 1, &pa->disk_full);
	    sqlite_query_column(hStmt, 2, &pa->record_fail);
	    sqlite_query_column(hStmt, 3, &pa->disk_fail);
		sqlite_query_column(hStmt, 4, &pa->disk_unformat);
		sqlite_query_column(hStmt, 5, &pa->no_disk);
	    sqlite_clear_stmt(hStmt);
	    sqlite_disconn(hConn);
	    FileUnlock(nFd, &global_rwlock);
	    return 0;
}

int write_trigger_alarms(const char *pDbFile, const struct trigger_alarms *pa)
{
	if (!pDbFile || !pa)
		return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = { 0 };
	snprintf(sExec, sizeof(sExec),"update trigger_alarms set network_disconn=%d, disk_full=%d, record_fail=%d, disk_fail=%d, disk_unformat=%d, no_disk=%d;",
			pa->network_disconn, pa->disk_full, pa->record_fail, pa->disk_fail, pa->disk_unformat, pa->no_disk);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_cameraios(const char *pDbFile, struct camera_io cio[], int *cnt)
{
    if(!pDbFile || !cio) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0){
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select chanid, enable, tri_channels, tri_actions, tri_channels_ex from cameraio;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0){
        *cnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++){
        sqlite_query_column(hStmt, 0, &cio[i].chanid);
        sqlite_query_column(hStmt, 1, &cio[i].enable);
        sqlite_query_column(hStmt, 2, (int*)&cio[i].tri_channels);
        sqlite_query_column(hStmt, 3, (int*)&cio[i].tri_actions);
		sqlite_query_column_text(hStmt, 4, cio[i].tri_channels_ex, sizeof(cio[i].tri_channels_ex));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *cnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
    

int read_cameraio(const char *pDbFile, struct camera_io *pio, int chanid)
{
	if(!pDbFile || !pio) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0){
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[128] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select chanid,enable,tri_channels,tri_actions,tri_channels_ex from cameraio where chanid=%d;", chanid);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0){
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &pio->chanid);
    sqlite_query_column(hStmt, 1, &pio->enable);
    sqlite_query_column(hStmt, 2, (int*)&pio->tri_channels);
    sqlite_query_column(hStmt, 3, (int*)&pio->tri_actions);
	sqlite_query_column_text(hStmt, 4, pio->tri_channels_ex, sizeof(pio->tri_channels_ex));
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_cameraio(const char *pDbFile, const struct camera_io *pio)
{
	if (!pDbFile || !pio) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = { 0 };
	snprintf(sExec, sizeof(sExec),"insert into cameraio(chanid,enable,tri_channels,tri_actions,tri_channels_ex) values(%d,%d,%d,%d,'%s');",
			pio->chanid, pio->enable, pio->tri_channels, pio->tri_actions,pio->tri_channels_ex);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_cameraio(const char *pDbFile, const struct camera_io *pio)
{
	if (!pDbFile || !pio) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = { 0 };
	snprintf(sExec, sizeof(sExec),"update cameraio set enable=%d, tri_channels=%d, tri_actions=%d, tri_channels_ex='%s' where chanid=%d;",
			pio->enable, pio->tri_channels, pio->tri_actions, pio->tri_channels_ex, pio->chanid);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_volumedetects(const char *pDbFile, struct volume_detect vd[], int *cnt)
{
    if(!pDbFile || !vd) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0){
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select chanid, enable, tri_channels, tri_actions, tri_channels_ex from volumedetect;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0){
        *cnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_CAMERA; i++){
        sqlite_query_column(hStmt, 0, &vd[i].chanid);
        sqlite_query_column(hStmt, 1, &vd[i].enable);
        sqlite_query_column(hStmt, 2, (int*)&vd[i].tri_channels);
        sqlite_query_column(hStmt, 3, (int*)&vd[i].tri_actions);
		sqlite_query_column_text(hStmt, 4, vd[i].tri_channels_ex, sizeof(vd[i].tri_channels_ex));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *cnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_volumedetect(const char *pDbFile, struct volume_detect *pvd, int chanid)
{
	if(!pDbFile || !pvd) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0){
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[128] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select chanid,enable,tri_channels,tri_actions,tri_channels_ex from volumedetect where chanid=%d;",chanid);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0){
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, (int*)&pvd->chanid);
    sqlite_query_column(hStmt, 1, (int*)&pvd->enable);
    sqlite_query_column(hStmt, 2, (int*)&pvd->tri_channels);
    sqlite_query_column(hStmt, 3, (int*)&pvd->tri_actions);
	sqlite_query_column_text(hStmt, 4, pvd->tri_channels_ex, sizeof(pvd->tri_channels_ex));
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_volumedetect(const char *pDbFile, const struct volume_detect *pvd)
{
	if (!pDbFile || !pvd) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = { 0 };
	snprintf(sExec, sizeof(sExec),"update volumedetect set enable=%d, tri_channels=%d, tri_actions=%d, tri_channels_ex='%s' where chanid=%d;",
			pvd->enable, pvd->tri_channels, pvd->tri_actions, pvd->tri_channels_ex, pvd->chanid);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int insert_volume_detect(const char *pDbFile, const struct volume_detect *pvd)
{
	if (!pDbFile || !pvd) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) {
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = { 0 };
	snprintf(sExec, sizeof(sExec),"insert into volumedetect(chanid,enable,tri_channels,tri_actions,tri_channels_ex) values(%d,%d,%d,%d,'%s');",
			pvd->chanid, pvd->enable, pvd->tri_channels, pvd->tri_actions, pvd->tri_channels_ex);
	sqlite_execute(hConn, sExec);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}


//bruce.milesight add for param device information
static int ms_set_device(const char *pDbFile, struct device_info *device, const char *table)
{
	char sExec[256] = { 0 };
	if (!pDbFile || !device) 
		return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if (sqlite_conn(pDbFile, &hConn) != 0) 
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->devid, PARAM_DEVICE_ID);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_cameras, PARAM_MAX_CAM);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_pb_num, PARAM_MAX_PB_CAM);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_disk, PARAM_MAX_DISK);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_alarm_in, PARAM_MAX_ALARM_IN);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_alarm_out, PARAM_MAX_ALARM_OUT);
	sqlite_execute(hConn, sExec);	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_audio_in, PARAM_MAX_AUDIO_IN);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_audio_out, PARAM_MAX_AUDIO_OUT);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_screen, PARAM_MAX_SCREEN);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_hdmi, PARAM_MAX_HDMI);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_vga, PARAM_MAX_VGA);
	sqlite_execute(hConn, sExec);	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->camera_layout, PARAM_CAMERA_LAYOUT);
	sqlite_execute(hConn, sExec);	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_lan, PARAM_MAX_NETWORKS);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->max_rs485, PARAM_MAX_RS485);
	sqlite_execute(hConn, sExec);	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->oem_type, PARAM_OEM_TYPE);	
	sqlite_execute(hConn, sExec);
	
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->prefix, PARAM_DEVICE_NO);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->model, PARAM_DEVICE_MODEL);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->sncode, PARAM_DEVICE_SN);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->softver, PARAM_DEVICE_SV);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->hardver, PARAM_DEVICE_HV);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->company, PARAM_DEVICE_COMPANY);
	sqlite_execute(hConn, sExec);
	snprintf(sExec, sizeof(sExec),"update %s set value='%s' where name='%s'", table, device->lang, PARAM_DEVICE_LANG);
	sqlite_execute(hConn, sExec);	
	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->def_lang, PARAM_DEVICE_DEF_LANG);
	sqlite_execute(hConn, sExec);

	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->power_key, PARAM_POWER_KEY);
	sqlite_execute(hConn, sExec);

	snprintf(sExec, sizeof(sExec),"update %s set value='%d' where name='%s'", table, device->msddns, PARAM_MS_DDNS);
	sqlite_execute(hConn, sExec);
	
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int db_set_device(const char *pDbFile, struct device_info *device)
{
	return ms_set_device(pDbFile, device, "params");
}

int db_set_device_oem(const char *pDbFile, struct device_info *device)
{
	return ms_set_device(pDbFile, device, "params_oem");
}

static int ms_get_device(const char *pDbFile, struct device_info *device, const char *table)
{
	char sQuery[256] = {0};
	int nColumnCnt = 0, nFd, nResult;
	HSQLITE hConn = 0;
	HSQLSTMT hStmt = 0;

	if(!pDbFile) 
		return -1;
	nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);   
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
	    FileUnlock(nFd, &global_rwlock);
	    return -2;
	}

	//snprintf(device->devtype, sizeof(device->devtype), "N");//NVR
	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_ID);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->devid);
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_CAM);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_cameras);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_PB_CAM);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_pb_num);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
	
	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_DISK);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_disk);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	


	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_ALARM_IN);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_alarm_in);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_ALARM_OUT);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_alarm_out);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_AUDIO_IN);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_audio_in);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_AUDIO_OUT);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_audio_out);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_SCREEN);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_screen);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_HDMI);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_hdmi);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_VGA);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_vga);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_RS485);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_rs485);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_CAMERA_LAYOUT);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->camera_layout);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	
	
	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MAX_NETWORKS);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->max_lan);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_OEM_TYPE);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->oem_type);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_SN);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->sncode, sizeof(device->sncode));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_NO);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->prefix, sizeof(device->prefix));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_MODEL);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->model, sizeof(device->model));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}		

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_SV);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->softver, sizeof(device->softver));
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_HV);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->hardver, sizeof(device->hardver));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_COMPANY);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->company, sizeof(device->company));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	
	
	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_DEF_LANG);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->def_lang);
	}	
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_DEVICE_LANG);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column_text(hStmt, 0, (char *)device->lang, sizeof(device->lang));
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}		


	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_POWER_KEY);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->power_key);
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	
	

	snprintf(sQuery, sizeof(sQuery), "select value from %s where name='%s';", table, PARAM_MS_DDNS);
	nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(!nResult && nColumnCnt)
	{
		sqlite_query_column(hStmt, 0, (int *)&device->msddns);
	}
	if(hStmt)
	{
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}	
		
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;		
}

int db_get_device(const char *pDbFile, struct device_info *device)
{
	return ms_get_device(pDbFile, device, "params");
}

int db_get_device_oem(const char *pDbFile, struct device_info *device)
{
	return ms_get_device(pDbFile, device, "params_oem");
}

//bruce.milesight add
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// timezone
int db_get_tz_filename(const char *pDbFile, const char *tz, char *tzname, int tzname_size, char *tzfname, int tzfname_size)
{
	int nColumnCnt = 0;
	HSQLSTMT hStmt = 0;
	HSQLITE hConn = 0;
	char sQuery[512] = {0};

	if(!pDbFile) 
		return -1;
    int nFd = FileLock(FILE_TZ_NAME, 'r', &global_rwlock);
    if(sqlite_conn(pDbFile, &hConn) != 0)
	{
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	if(tzname[0]) {
		snprintf(sQuery, sizeof(sQuery), "select name,zonefile from zonemap where timezone='%s' and name='%s'", tz, tzname);
	}else {
		snprintf(sQuery, sizeof(sQuery), "select name,zonefile from zonemap where timezone='%s'", tz);
	}

    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
	{
        if(hStmt) 
			sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	sqlite_query_column_text(hStmt, 0, tzname, tzname_size);
	sqlite_query_column_text(hStmt, 1, tzfname, tzfname_size);	
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
	
    return 0;
}

int write_layout1_chn(const char *pDbFile, int chnid)
{
	if (!pDbFile) return -1;
	if(!check_param_key(pDbFile, PARAM_LAYOUT1_CHNID)){
		set_param_int(pDbFile, PARAM_LAYOUT1_CHNID, chnid);
	}else{
		add_param_int(pDbFile, PARAM_LAYOUT1_CHNID, 0);
		set_param_int(pDbFile, PARAM_LAYOUT1_CHNID, chnid);
	}
	return 0;
}

int read_layout1_chn(const char *pDbFile, int *chnid)
{
	if (!pDbFile || !chnid) return -1;
	*chnid = get_param_int(pDbFile, PARAM_LAYOUT1_CHNID, 0);
	return 0;
}


//###################################################//
int db_read_user_by_name(const char *pDbFile, struct db_user *user, char *username)
{
	if(!pDbFile || !user || !username) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	int nColumnCnt = 0;
	char sQuery[512] = {0};
	snprintf(sQuery, sizeof(sQuery), "select id,enable,username,password,type,permission,remote_permission,local_live_view,local_playback,remote_live_view,remote_playback from user where username='%s';", username);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0)
	{
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

    sqlite_query_column(hStmt, 0, &(user->id));
    sqlite_query_column(hStmt, 1, &(user->enable));
    sqlite_query_column_text(hStmt, 2, user->username, sizeof(user->username));
    sqlite_query_column_text(hStmt, 3, user->password, sizeof(user->password));
    sqlite_query_column(hStmt, 4, &(user->type));
    sqlite_query_column(hStmt, 5, &(user->permission));
    sqlite_query_column(hStmt, 6, &(user->remote_permission));
	sqlite_query_column(hStmt, 7, (int*)&(user->local_live_view));
	sqlite_query_column(hStmt, 8, (int*)&(user->local_playback));
	sqlite_query_column(hStmt, 9, (int*)&(user->remote_live_view));
	sqlite_query_column(hStmt, 10, (int*)&(user->remote_playback));

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_users_oem(const char *pDbFile, struct db_user_oem users[], int *pCnt)
{
    if(!pDbFile || !users) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,username,password,type,permission,remote_permission,local_live_view,local_playback,remote_live_view,remote_playback from user;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_USER; i++)
    {
        sqlite_query_column(hStmt, 0, &(users[i].id));
        sqlite_query_column(hStmt, 1, &(users[i].enable));
        sqlite_query_column_text(hStmt, 2, users[i].username, sizeof(users[i].username));
        sqlite_query_column_text(hStmt, 3, users[i].password, sizeof(users[i].password));
        sqlite_query_column(hStmt, 4, &(users[i].type));
        sqlite_query_column(hStmt, 5, &(users[i].permission));
        sqlite_query_column(hStmt, 6, &(users[i].remote_permission));
		sqlite_query_column(hStmt, 7, (int*)&(users[i].local_live_view));
		sqlite_query_column(hStmt, 8, (int*)&(users[i].local_playback));
		sqlite_query_column(hStmt, 9, (int*)&(users[i].remote_live_view));
		sqlite_query_column(hStmt, 10, (int*)&(users[i].remote_playback));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int write_sub_layout1_chn(const char *pDbFile, int chnid)
{
	if (!pDbFile) return -1;
	if(!check_param_key(pDbFile, PARAM_SUB_LAYOUT1_CHNID)){
		set_param_int(pDbFile, PARAM_SUB_LAYOUT1_CHNID, chnid);
	}else{
		add_param_int(pDbFile, PARAM_SUB_LAYOUT1_CHNID, 0);
		set_param_int(pDbFile, PARAM_SUB_LAYOUT1_CHNID, chnid);
	}
	return 0;
}

int read_sub_layout1_chn(const char *pDbFile, int *chnid)
{
	if (!pDbFile || !chnid) return -1;
	*chnid = get_param_int(pDbFile, PARAM_SUB_LAYOUT1_CHNID, 0);
	return 0;
}

int write_autoreboot_conf(const char *pDbFile, reboot_conf* conf)
{
    if(!pDbFile || !conf) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update auto_reboot set enable=%d,wday=%d,hour=%d,login=%d,username='%s',reboot=%d  where id=0;",
    		conf->enable,conf->wday,conf->hour,conf->login,conf->username,conf->reboot);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_autoreboot_conf(const char *pDbFile, reboot_conf* conf)
{
    if(!pDbFile || !conf) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select enable,wday,hour,login,username,reboot from auto_reboot where id=0;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &conf->enable);
    sqlite_query_column(hStmt, 1, (int *)&conf->wday);
    sqlite_query_column(hStmt, 2, &conf->hour);
    sqlite_query_column(hStmt, 3, &conf->login);
	sqlite_query_column_text(hStmt, 4, conf->username, sizeof(conf->username));
    sqlite_query_column(hStmt, 5, &conf->reboot);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_privacy_mask_by_areaid(const char *pDbFile, struct privacy_mask *mask, int chn_id, int area_id)
{
	if(!pDbFile || !mask) return -1;
	if(area_id < 0 || area_id >= MAX_MASK_AREA_NUM) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0){
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	HSQLSTMT hStmt = 0;
	char sQuery[272] = {0};
	int nColumnCnt = 0;
	snprintf(sQuery, sizeof(sQuery), "select id,area_id,enable,start_x,start_y,area_width,area_height,width,height,fill_color from privacy_mask where id = %d and area_id = %d order by(area_id);", chn_id, area_id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult != 0 || nColumnCnt == 0){
		if(hStmt) sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	sqlite_query_column(hStmt, 0, &mask->id);
	sqlite_query_column(hStmt, 1, &mask->area[area_id].area_id);
	sqlite_query_column(hStmt, 2, &mask->area[area_id].enable);
	sqlite_query_column(hStmt, 3, &mask->area[area_id].start_x);
	sqlite_query_column(hStmt, 4, &mask->area[area_id].start_y);
	sqlite_query_column(hStmt, 5, &mask->area[area_id].area_width);
	sqlite_query_column(hStmt, 6, &mask->area[area_id].area_height);
	sqlite_query_column(hStmt, 7, &mask->area[area_id].width);
	sqlite_query_column(hStmt, 8, &mask->area[area_id].height);
	sqlite_query_column(hStmt, 9, &mask->area[area_id].fill_color);

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_privacy_mask_by_areaid(const char *pDbFile, struct privacy_mask *mask, int area_id)
{
	if(!pDbFile || !mask) return -1;
	if(area_id < 0 || area_id >= MAX_MASK_AREA_NUM) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[512] = {0};

	snprintf(sExec, sizeof(sExec), "update privacy_mask set enable=%d,start_x=%d,start_y=%d,area_width=%d,area_height=%d,width=%d,height=%d,fill_color=%d where id=%d and area_id=%d;", 
			mask->area[area_id].enable, mask->area[area_id].start_x, mask->area[area_id].start_y, mask->area[area_id].area_width, mask->area[area_id].area_height, mask->area[area_id].width, mask->area[area_id].height, mask->area[area_id].fill_color, mask->id, mask->area[area_id].area_id);
	sqlite_execute(hConn, sExec);

	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_motion_email_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id)
{
    if(!pDbFile || !motionSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from motion_email_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion_email_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_email_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into motion_email_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_motion_popup_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id)
{
    if(!pDbFile || !motionSchedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from motion_popup_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(motionSchedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion_popup_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from motion_popup_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into motion_popup_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_videoloss_email_schedule(const char *pDbFile, struct video_loss_schedule  *schedule, int chn_id)
{
    if(!pDbFile || !schedule) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from videoloss_email_schedule where chn_id=%d;", chn_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
    for(i=0; i<=nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_videoloss_email_schedule(const char *pDbFile, struct video_loss_schedule  *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
    snprintf(sExec, sizeof(sExec), "delete from videoloss_email_schedule where chn_id=%d;", chn_id);
    sqlite_execute(hConn, sExec);
    int i=0, j=0;
    for(i=0; i<MAX_DAY_NUM; i++)
    {
        for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
        {
            if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
            {
                continue;
            }
            snprintf(sExec, sizeof(sExec), "insert into videoloss_email_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
            sqlite_execute(hConn, sExec);
        }
    }
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarmin_email_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int chn_id)
{
	if(!pDbFile || !schedule) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	HSQLSTMT hStmt = 0;
	char sQuery[256] = {0};
	int nColumnCnt = 0;
	int i = 0;
	snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from alarmin_email_schedule where chn_id=%d;", chn_id);
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult !=0 || nColumnCnt == 0)
	{
		if(hStmt) sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	int nWeekId = 1;
	int nPlanId = 0;
	int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	for(i=0; i<=nLoopCnt; i++)
	{
		sqlite_query_column(hStmt, 0, &nWeekId);
		sqlite_query_column(hStmt, 1, &nPlanId);
		sqlite_query_column_text(hStmt, 2, schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
		sqlite_query_column_text(hStmt, 3, schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
		sqlite_query_column(hStmt, 4, &(schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0){i++; break;}
	}
	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int write_alarmin_email_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int chn_id)
{
	if(!pDbFile || !schedule || chn_id<0) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	char sExec[256] = {0};
	snprintf(sExec, sizeof(sExec), "delete from alarmin_email_schedule where chn_id=%d;", chn_id);
	sqlite_execute(hConn, sExec);
	int i=0, j=0;
	for(i=0; i<MAX_DAY_NUM; i++)
	{
		for(j=0; j<MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time)==0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into alarmin_email_schedule values(%d,%d,%d, '%s', '%s', %d);", chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

//modify for action PZT preset & patrol x.7.0.9
int read_alarm_in_ex(const char *pDbFile, struct alarm_in *alarm, int alarm_id)
{
    if(!pDbFile || !alarm || alarm_id<0) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id, enable,name,type,tri_channels,tri_alarms,buzzer_interval,tri_channels_ex,acto_ptz_channel,acto_ptz_preset_enable,acto_ptz_preset,acto_ptz_patrol_enable,acto_ptz_patrol,email_enable,email_buzzer_interval from alarm_in where id=%d;", alarm_id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &alarm->id);
    sqlite_query_column(hStmt, 1, &alarm->enable);
    sqlite_query_column_text(hStmt, 2, alarm->name, sizeof(alarm->name));
    sqlite_query_column(hStmt, 3, &alarm->type);
    sqlite_query_column(hStmt, 4, (int*)&alarm->tri_channels);
    sqlite_query_column(hStmt, 5, (int*)&alarm->tri_alarms);
    sqlite_query_column(hStmt, 6, &alarm->buzzer_interval);
	sqlite_query_column_text(hStmt, 7, alarm->tri_channels_ex, sizeof(alarm->tri_channels_ex));
	sqlite_query_column(hStmt, 8, &alarm->acto_ptz_channel);
	sqlite_query_column(hStmt, 9, &alarm->acto_ptz_preset_enable);
	sqlite_query_column(hStmt, 10, &alarm->acto_ptz_preset);
	sqlite_query_column(hStmt, 11, &alarm->acto_ptz_patrol_enable);
	sqlite_query_column(hStmt, 12, &alarm->acto_ptz_patrol);
	sqlite_query_column(hStmt, 13, &alarm->email_enable);
	sqlite_query_column(hStmt, 14, &alarm->email_buzzer_interval);

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_alarm_ins_ex(const char *pDbFile, struct alarm_in alarms[], int *pCnt)
{
    if(!pDbFile || !alarms) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,name,type,tri_channels,tri_alarms,buzzer_interval,tri_channels_ex,acto_ptz_channel,acto_ptz_preset_enable,acto_ptz_preset,acto_ptz_patrol_enable,acto_ptz_patrol,email_enable,email_buzzer_interval from alarm_in;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_ALARM_IN; i++)
    {
        sqlite_query_column(hStmt, 0, &alarms[i].id);
        sqlite_query_column(hStmt, 1, &alarms[i].enable);
        sqlite_query_column_text(hStmt, 2, alarms[i].name, sizeof(alarms[i].name));
        sqlite_query_column(hStmt, 3, &alarms[i].type);
        sqlite_query_column(hStmt, 4, (int*)&alarms[i].tri_channels);
        sqlite_query_column(hStmt, 5, (int*)&alarms[i].tri_alarms);
        sqlite_query_column(hStmt, 6, &alarms[i].buzzer_interval);
		sqlite_query_column_text(hStmt, 7, alarms[i].tri_channels_ex, sizeof(alarms[i].tri_channels_ex));
		sqlite_query_column(hStmt, 8, &alarms[i].acto_ptz_channel);
		sqlite_query_column(hStmt, 9, &alarms[i].acto_ptz_preset_enable);
		sqlite_query_column(hStmt, 10, &alarms[i].acto_ptz_preset);
		sqlite_query_column(hStmt, 11, &alarms[i].acto_ptz_patrol_enable);
		sqlite_query_column(hStmt, 12, &alarms[i].acto_ptz_patrol);
		sqlite_query_column(hStmt, 11, &alarms[i].email_enable);
		sqlite_query_column(hStmt, 12, &alarms[i].email_buzzer_interval);

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_alarm_in_ex(const char *pDbFile, struct alarm_in *alarm)
{
    if(!pDbFile || !alarm) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update alarm_in set enable=%d,name='%s',type=%d, tri_channels=%u,tri_alarms=%u, buzzer_interval =%d,tri_channels_ex='%s',acto_ptz_channel=%d,acto_ptz_preset_enable=%d,acto_ptz_preset=%d,acto_ptz_patrol_enable=%d,acto_ptz_patrol=%d,email_enable=%d,email_buzzer_interval=%d where id=%d;",
    		alarm->enable, alarm->name, alarm->type, alarm->tri_channels, alarm->tri_alarms, alarm->buzzer_interval, alarm->tri_channels_ex, alarm->acto_ptz_channel, alarm->acto_ptz_preset_enable, alarm->acto_ptz_preset, alarm->acto_ptz_patrol_enable, alarm->acto_ptz_patrol, alarm->email_enable, alarm->email_buzzer_interval, alarm->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
int write_ssl_params(const char *pDbFile, struct cert_info *cert)
{
    if(!pDbFile || !cert) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[1024] = {0};
	if(cert->type == PRIVATE_CERT)
	{
	    snprintf(sExec, sizeof(sExec), "update https set country='%s',common_name='%s',email='%s',dates='%s';",
	    		cert->country, cert->common_name, cert->email, cert->dates);
	}
	else if(cert->type == CERT_REQ)
	{
	    snprintf(sExec, sizeof(sExec), "update https set country_req='%s',common_name_req='%s',email_req='%s';",
	    		cert->country, cert->common_name, cert->email);
	}
	
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
int read_ssl_params(const char *pDbFile, struct cert_info *cert,int type)
{
    if(!pDbFile || !cert) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
	if(type == PRIVATE_CERT)
	{
		snprintf(sQuery, sizeof(sQuery), "select country,common_name,email,dates from https;");
		int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
		if(nResult !=0 || nColumnCnt == 0)
		{
			if(hStmt) sqlite_clear_stmt(hStmt);
			sqlite_disconn(hConn);
			FileUnlock(nFd, &global_rwlock);
			return -1;
		}
		sqlite_query_column_text(hStmt, 0, cert->country, sizeof(cert->country));
		sqlite_query_column_text(hStmt, 1, cert->common_name, sizeof(cert->common_name));
		sqlite_query_column_text(hStmt, 2, cert->email, sizeof(cert->email));
		sqlite_query_column_text(hStmt, 3, cert->dates, sizeof(cert->dates));
	}
	else if(type == CERT_REQ)
	{
		snprintf(sQuery, sizeof(sQuery), "select country_req,common_name_req,email_req from https;");
		int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
		if(nResult !=0 || nColumnCnt == 0)
		{
			if(hStmt) sqlite_clear_stmt(hStmt);
			sqlite_disconn(hConn);
			FileUnlock(nFd, &global_rwlock);
			return -1;
		}
		sqlite_query_column_text(hStmt, 0, cert->country, sizeof(cert->country));
		sqlite_query_column_text(hStmt, 1, cert->common_name, sizeof(cert->common_name));
		sqlite_query_column_text(hStmt, 2, cert->email, sizeof(cert->email));
	}

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
int write_encrypted_list(const char *pDbFile, struct squestion *sqa, int sorder)
{
    if(!pDbFile || !sqa) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
	char buf[MAX_LEN_64*MAX_SQA_CNT*2] = {0};
	char answer[MAX_LEN_64*MAX_SQA_CNT*2] = {0};
	char question[MAX_LEN_64*MAX_SQA_CNT*2] = {0};
	
	translate_pwd(buf,sqa->answer,MAX_LEN_64*MAX_SQA_CNT*2);
	memcpy(answer, buf, MAX_LEN_64*MAX_SQA_CNT*2);
	memset(buf, 0, MAX_LEN_64*MAX_SQA_CNT*2);
	
	if(sqa->sqtype == 12)
	{
		translate_pwd(buf,sqa->squestion,MAX_LEN_64*MAX_SQA_CNT*2);
		memcpy(question, buf, MAX_LEN_64*MAX_SQA_CNT*2);
		snprintf(sExec, sizeof(sExec), "update encrypted_list set sqType='%d',squestion='%s',answer='%s',enable='1' where id='%d';",sqa->sqtype, question, answer,sorder);
	}
	else
	{
		snprintf(sExec, sizeof(sExec), "update encrypted_list set sqType='%d',answer='%s',enable='1' where id='%d';",sqa->sqtype,answer,sorder);
	}
	
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}
int read_encrypted_list(const char *pDbFile, struct squestion sqas[])
{
    if(!pDbFile || !sqas) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[512] = {0};
    int nColumnCnt = 0;
	int i = 0;
	
	snprintf(sQuery, sizeof(sQuery), "select id,sqtype,squestion,answer,enable from encrypted_list;");
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult !=0 || nColumnCnt == 0)
	{
		if(hStmt) sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    for(i=0; i<MAX_SQA_CNT; i++)
    {
		sqlite_query_column(hStmt, 1, &sqas[i].sqtype);
		sqlite_query_column_text(hStmt, 2, sqas[i].squestion, sizeof(sqas[i].squestion));
		sqlite_query_column_text(hStmt, 3, sqas[i].answer, sizeof(sqas[i].answer));
		sqlite_query_column(hStmt, 4, &sqas[i].enable);

		if(sqlite_query_next(hStmt) != 0){i++; break;}
	}

    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int check_table_key(const char *pDbFile, char *table, const char *key)
{
	if(!pDbFile || !key) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
	HSQLITE hConn = 0;
	if(sqlite_conn(pDbFile, &hConn) != 0)
	{
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	HSQLSTMT hStmt = 0;
	char sQuery[256] = {0};
	snprintf(sQuery, sizeof(sQuery), "select %s from %s where id=0;", key, table);
	int nColumnCnt = 0;
	int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
	if(nResult !=0 || nColumnCnt == 0)
	{
		if(hStmt) sqlite_clear_stmt(hStmt);
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	sqlite_clear_stmt(hStmt);
	sqlite_disconn(hConn);
	FileUnlock(nFd, &global_rwlock);

	return 0;	
}

int read_disks(const char *pDbFile, struct diskInfo disks[], int *pCnt)
{
    if(!pDbFile || !disks) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select disk_port,enable,disk_vendor,disk_address,disk_directory,disk_type,disk_group,disk_property,raid_level from disk;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i = 0; i<MAX_MSDK_NUM; i++)
    {
        sqlite_query_column(hStmt, 0, &disks[i].disk_port);
		sqlite_query_column(hStmt, 1, &disks[i].enable);
		sqlite_query_column_text(hStmt, 2, disks[i].disk_vendor, sizeof(disks[i].disk_vendor));
		sqlite_query_column_text(hStmt, 3, disks[i].disk_address, sizeof(disks[i].disk_address));
		sqlite_query_column_text(hStmt, 4, disks[i].disk_directory, sizeof(disks[i].disk_directory));
		sqlite_query_column(hStmt, 5, &disks[i].disk_type);
		sqlite_query_column(hStmt, 6, &disks[i].disk_group);
		sqlite_query_column(hStmt, 7, &disks[i].disk_property);
		sqlite_query_column(hStmt, 8, &disks[i].raid_level);

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_disk(const char *pDbFile, struct diskInfo *disk, int portId)
{
    if(!pDbFile || !disk) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select disk_port,enable,disk_vendor,disk_address,disk_directory,disk_type,disk_group,disk_property,raid_level from disk where disk_port=%d;", portId);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    sqlite_query_column(hStmt, 0, &disk->disk_port);
	sqlite_query_column(hStmt, 1, &disk->enable);
	sqlite_query_column_text(hStmt, 2, disk->disk_vendor, sizeof(disk->disk_vendor));
	sqlite_query_column_text(hStmt, 3, disk->disk_address, sizeof(disk->disk_address));
	sqlite_query_column_text(hStmt, 4, disk->disk_directory, sizeof(disk->disk_directory));
	sqlite_query_column(hStmt, 5, &disk->disk_type);
	sqlite_query_column(hStmt, 6, &disk->disk_group);
	sqlite_query_column(hStmt, 7, &disk->disk_property);
	sqlite_query_column(hStmt, 8, &disk->raid_level);
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_disk(const char *pDbFile, struct diskInfo *disk)
{
    if(!pDbFile || !disk) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update disk set disk_port=%d,enable=%d,disk_vendor='%s',disk_address='%s',disk_directory='%s',disk_type=%d,disk_group=%d,disk_property=%d,raid_level=%d where disk_port=%d;",
    		disk->disk_port, disk->enable, disk->disk_vendor, disk->disk_address, disk->disk_directory, disk->disk_type, disk->disk_group, disk->disk_property, disk->raid_level, disk->disk_port);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int insert_disk(const char* pDbFile, struct diskInfo*diskInfo)
{
	if(!pDbFile || !diskInfo) return -1;
	int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
	HSQLITE hConn = 0;
	char sExec[256] = {0};
	if(sqlite_conn(pDbFile, &hConn) != 0){
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}

	snprintf(sExec, sizeof(sExec), "insert into disk(disk_port,enable,disk_vendor,disk_address,disk_directory,disk_type,disk_group,disk_property,raid_level) values(%d,%d,%s,%s,%s,%d,%d,%d,%d);", diskInfo->disk_port, 
		diskInfo->enable, diskInfo->disk_vendor, diskInfo->disk_address, diskInfo->disk_directory, diskInfo->disk_type, diskInfo->disk_group, diskInfo->disk_property, diskInfo->raid_level);

	sqlite_execute(hConn, sExec);
	FileUnlock(nFd, &global_rwlock);
	return 0;
}

int read_groups(const char *pDbFile, struct groupInfo groups[], int *pCnt)
{
    if(!pDbFile || !groups) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,chnMaskl,chnMaskh from diskGroup;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *pCnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i = 0; i<MAX_MSDK_GROUP_NUM; i++)
    {
        sqlite_query_column(hStmt, 0, &groups[i].groupid);
        sqlite_query_column_text(hStmt, 1, groups[i].chnMaskl, sizeof(groups[i].chnMaskl));
		sqlite_query_column_text(hStmt, 2, groups[i].chnMaskh, sizeof(groups[i].chnMaskh));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }
    *pCnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_group_st(const char *pDbFile, struct groupInfo *group)
{
    if(!pDbFile || !group) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	
    char sExec[512] = {0};
    snprintf(sExec, sizeof(sExec), "update diskGroup set id=%d,chnMaskl='%s',chnMaskh='%s' where id=%d;",
    		group->groupid, group->chnMaskl, group->chnMaskh, group->groupid);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_effective_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[256] = {0};
	
    HSQLSTMT hStmt = 0;
  
	if(type == REGIONIN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == TAMPER)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LOITERING)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == HUMAN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_human_effective_schedule where chn_id=%d;", chn_id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_effective_schedule where chn_id=%d;", chn_id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
        {
			sqlite_clear_stmt(hStmt);
		}
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
   
    for(i = 0; i <= nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0)
		{	
			i++; 
			break;
		}
    }
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event_effective_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !schedule || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[64] = {0};
	if(type == REGIONIN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionen_effective_schedule");
	}
	else if(type == REGIONOUT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionex_effective_schedule");
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_motionadv_effective_schedule");
	}
	else if(type == TAMPER)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_tamper_effective_schedule");
	}
	else if(type == LINECROSS)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_linecross_effective_schedule");
	}
	else if(type == LOITERING)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_loiter_effective_schedule");
	}
	else if(type == HUMAN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_human_effective_schedule");
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_peolpe_effective_schedule");
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table, chn_id);
	sqlite_execute(hConn, sExec);
	int i = 0, j = 0;
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		for(j = 0; j < MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table, chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
		
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_audible_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[256] = {0};
	
    HSQLSTMT hStmt = 0;
  
	if(type == REGIONIN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == TAMPER)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LOITERING)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == HUMAN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_human_audible_schedule where chn_id=%d;", chn_id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_audible_schedule where chn_id=%d;", chn_id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
        {
			sqlite_clear_stmt(hStmt);
		}
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
   
    for(i = 0; i <= nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0)
		{	
			i++; 
			break;
		}
    }
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event_audible_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !schedule || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[64] = {0};
	if(type == REGIONIN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionen_audible_schedule");
	}
	else if(type == REGIONOUT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionex_audible_schedule");
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_motionadv_audible_schedule");
	}
	else if(type == TAMPER)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_tamper_audible_schedule");
	}
	else if(type == LINECROSS)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_linecross_audible_schedule");
	}
	else if(type == LOITERING)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_loiter_audible_schedule");
	}
	else if(type == HUMAN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_human_audible_schedule");
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_peolpe_audible_schedule");
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table, chn_id);
	sqlite_execute(hConn, sExec);
	int i = 0, j = 0;
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		for(j = 0; j < MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table, chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
		
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_mail_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[256] = {0};
	
    HSQLSTMT hStmt = 0;
  
	if(type == REGIONIN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == TAMPER)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LOITERING)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == HUMAN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_human_mail_schedule where chn_id=%d;", chn_id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_mail_schedule where chn_id=%d;", chn_id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
        {
			sqlite_clear_stmt(hStmt);
		}
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
   
    for(i = 0; i <= nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0)
		{	
			i++; 
			break;
		}
    }
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event_mail_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !schedule || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[64] = {0};
	if(type == REGIONIN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionen_mail_schedule");
	}
	else if(type == REGIONOUT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionex_mail_schedule");
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_motionadv_mail_schedule");
	}
	else if(type == TAMPER)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_tamper_mail_schedule");
	}
	else if(type == LINECROSS)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_linecross_mail_schedule");
	}
	else if(type == LOITERING)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_loiter_mail_schedule");
	}
	else if(type == HUMAN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_human_mail_schedule");
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_peolpe_mail_schedule");
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table, chn_id);
	sqlite_execute(hConn, sExec);
	int i = 0, j = 0;
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		for(j = 0; j < MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table, chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
		
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_popup_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[256] = {0};
	
    HSQLSTMT hStmt = 0;
  
	if(type == REGIONIN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionin_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_regionexit_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_motion_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == TAMPER)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == LOITERING)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == HUMAN)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_human_popup_schedule where chn_id=%d;", chn_id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from vca_people_popup_schedule where chn_id=%d;", chn_id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
        {
			sqlite_clear_stmt(hStmt);
		}
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
   
    for(i = 0; i <= nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0)
		{	
			i++; 
			break;
		}
    }
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event_popup_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !schedule || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[64] = {0};
	if(type == REGIONIN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionin_popup_schedule");
	}
	else if(type == REGIONOUT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_regionexit_popup_schedule");
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_motion_popup_schedule");
	}
	else if(type == TAMPER)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_tamper_popup_schedule");
	}
	else if(type == LINECROSS)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_linecross_popup_schedule");
	}
	else if(type == LOITERING)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_loiter_popup_schedule");
	}
	else if(type == HUMAN)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_human_popup_schedule");
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(db_table, sizeof(db_table), "%s", "vca_people_popup_schedule");
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table, chn_id);
	sqlite_execute(hConn, sExec);
	int i = 0, j = 0;
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		for(j = 0; j < MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table, chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
		
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event(const char *pDbFile, struct smart_event *smartevent, int id, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !smartevent) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[256] = {0};
    int nColumnCnt = 0;
	
	if(type == REGIONIN)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionin_event where id=%d;", id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionexit_event where id=%d;", id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_motion_event where id=%d;", id);
	}
	else if(type == TAMPER)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_tamper_event where id=%d;", id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_linecross_event where id=%d;", id);
	}
	else if(type == LOITERING)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_loiter_event where id=%d;", id);
	}
	else if(type == HUMAN)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_human_event where id=%d;", id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sQuery, sizeof(sQuery), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_people_event where id=%d;", id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
			sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	
    sqlite_query_column(hStmt, 0, &smartevent->id);
    sqlite_query_column(hStmt, 1, &smartevent->enable);
    sqlite_query_column(hStmt, 2, (int*)&smartevent->tri_alarms);
	sqlite_query_column_text(hStmt, 3, smartevent->tri_channels_ex, sizeof(smartevent->tri_channels_ex));
	sqlite_query_column(hStmt, 4, &smartevent->buzzer_interval);
	sqlite_query_column(hStmt, 5, &smartevent->email_interval);
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event(const char *pDbFile, struct smart_event *smartevent, SMART_EVENT_TYPE type)
{
    if(!pDbFile || !smartevent) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};
	if(type == REGIONIN)
	{
		snprintf(sExec, sizeof(sExec), "update vca_regionin_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == REGIONOUT)
	{
		snprintf(sExec, sizeof(sExec), "update vca_regionexit_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == ADVANCED_MOTION)
	{
		snprintf(sExec, sizeof(sExec), "update vca_motion_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == TAMPER)
	{
		snprintf(sExec, sizeof(sExec), "update vca_tamper_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == LINECROSS)
	{
		snprintf(sExec, sizeof(sExec), "update vca_linecross_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == LOITERING)
	{
		snprintf(sExec, sizeof(sExec), "update vca_loiter_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == HUMAN)
	{
		snprintf(sExec, sizeof(sExec), "update vca_human_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else if(type == PEOPLE_CNT)
	{
		snprintf(sExec, sizeof(sExec), "update vca_people_event set enable=%d, tri_alarms=%d, tri_channels_ex='%s', buzzer_interval='%d', email_interval='%d' where id=%d;",
				smartevent->enable, smartevent->tri_alarms, smartevent->tri_channels_ex, smartevent->buzzer_interval, smartevent->email_interval, smartevent->id);
	}
	else
	{
		sqlite_disconn(hConn);
		FileUnlock(nFd, &global_rwlock);
		return -1;
	}
	
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_motion_effective_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id)
{
    if(!pDbFile || !schedule || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[64] = {0};
	snprintf(db_table, sizeof(db_table), "%s", "motion_effective_schedule");
	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table, chn_id);
	sqlite_execute(hConn, sExec);
	int i = 0, j = 0;
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		for(j = 0; j < MAX_PLAN_NUM_PER_DAY; j++)
		{
			if(strcasecmp(schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time) == 0)
			{
				continue;
			}
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table, chn_id, i, j, schedule->schedule_day[i].schedule_item[j].start_time, schedule->schedule_day[i].schedule_item[j].end_time, schedule->schedule_day[i].schedule_item[j].action_type);
			sqlite_execute(hConn, sExec);
		}
	}
		
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_motion_effective_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[256] = {0};
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery, sizeof(sQuery), "select week_id,plan_id,start_time,end_time,action_type from motion_effective_schedule where chn_id=%d;", chn_id);
    nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult != 0 || nColumnCnt == 0)
    {
        if(hStmt) 
        {
			sqlite_clear_stmt(hStmt);
		}
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
   
    for(i = 0; i <= nLoopCnt; i++)
    {
        sqlite_query_column(hStmt, 0, &nWeekId);
        sqlite_query_column(hStmt, 1, &nPlanId);
        sqlite_query_column_text(hStmt, 2, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].start_time));
        sqlite_query_column_text(hStmt, 3, Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].end_time));
        sqlite_query_column(hStmt, 4, &(Schedule->schedule_day[nWeekId].schedule_item[nPlanId].action_type));
		if(sqlite_query_next(hStmt) != 0)
		{	
			i++; 
			break;
		}
    }
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_events(const char *pDbFile, struct smart_event *smartevent, int chnid)
{
    if(!pDbFile || !smartevent) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	int i = 0; 
    HSQLSTMT hStmt = 0;
    char sQuery[MAX_SMART_EVENT][256];
    int nColumnCnt = 0;
		
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionin_event where id=%d;", chnid);
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionexit_event where id=%d;", chnid);
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_motion_event where id=%d;", chnid);
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_tamper_event where id=%d;", chnid);
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_linecross_event where id=%d;", chnid);
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_loiter_event where id=%d;", chnid);
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_human_event where id=%d;", chnid);
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_people_event where id=%d;", chnid);

	for(i = REGIONIN; i < MAX_SMART_EVENT; i++)
	{
		int nResult = sqlite_query_record(hConn, sQuery[i], &hStmt, &nColumnCnt);
	    if(nResult != 0 || nColumnCnt == 0)
	    {
	        if(hStmt)
			{ 
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
	        }
	        continue;
	    }
		
		sqlite_query_column(hStmt, 0, &smartevent[i].id);
	    sqlite_query_column(hStmt, 1, &smartevent[i].enable);
	    sqlite_query_column(hStmt, 2, (int*)&smartevent[i].tri_alarms);
		sqlite_query_column_text(hStmt, 3, smartevent[i].tri_channels_ex, sizeof(smartevent[i].tri_channels_ex));
		sqlite_query_column(hStmt, 4, &smartevent[i].buzzer_interval);
		sqlite_query_column(hStmt, 5, &smartevent[i].email_interval);
		if(hStmt)
		{ 
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
        }
	}

  	if(hStmt)
    	sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_effective_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_effective_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_effective_schedule where chn_id=%d;", chn_id);

	for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
	{
		nResult = sqlite_query_record(hConn, sQuery[iSmt], &hStmt, &nColumnCnt);
	    if(nResult != 0 || nColumnCnt == 0)
	    {
	        if(hStmt) 
	        {
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
	        continue;
	    }
	   
	    for(i = 0; i <= nLoopCnt; i++)
	    {
	        sqlite_query_column(hStmt, 0, &nWeekId);
	        sqlite_query_column(hStmt, 1, &nPlanId);
	        sqlite_query_column_text(hStmt, 2, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
	        sqlite_query_column_text(hStmt, 3, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
	        sqlite_query_column(hStmt, 4, &Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
			if(sqlite_query_next(hStmt) != 0)
			{	
				i++; 
				break;
			}
	    }
		if(hStmt)
		{
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_audible_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_audible_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_audible_schedule where chn_id=%d;", chn_id);

	for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
	{
		nResult = sqlite_query_record(hConn, sQuery[iSmt], &hStmt, &nColumnCnt);
	    if(nResult != 0 || nColumnCnt == 0)
	    {
	        if(hStmt) 
	        {
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
	        continue;
	    }
	   
	    for(i = 0; i <= nLoopCnt; i++)
	    {
	        sqlite_query_column(hStmt, 0, &nWeekId);
	        sqlite_query_column(hStmt, 1, &nPlanId);
	        sqlite_query_column_text(hStmt, 2, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
	        sqlite_query_column_text(hStmt, 3, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
	        sqlite_query_column(hStmt, 4, &Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
			if(sqlite_query_next(hStmt) != 0)
			{	
				i++; 
				break;
			}
	    }
		if(hStmt)
		{
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_mail_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_mail_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_mail_schedule where chn_id=%d;", chn_id);

	for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
	{
		nResult = sqlite_query_record(hConn, sQuery[iSmt], &hStmt, &nColumnCnt);
	    if(nResult != 0 || nColumnCnt == 0)
	    {
	        if(hStmt) 
	        {
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
	        continue;
	    }
	   
	    for(i = 0; i <= nLoopCnt; i++)
	    {
	        sqlite_query_column(hStmt, 0, &nWeekId);
	        sqlite_query_column(hStmt, 1, &nPlanId);
	        sqlite_query_column_text(hStmt, 2, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
	        sqlite_query_column_text(hStmt, 3, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
	        sqlite_query_column(hStmt, 4, &Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
			if(sqlite_query_next(hStmt) != 0)
			{	
				i++; 
				break;
			}
	    }
		if(hStmt)
		{
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_popup_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionin_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionexit_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motion_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_popup_schedule where chn_id=%d;", chn_id);
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_people_popup_schedule where chn_id=%d;", chn_id);

	for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
	{
		nResult = sqlite_query_record(hConn, sQuery[iSmt], &hStmt, &nColumnCnt);
	    if(nResult != 0 || nColumnCnt == 0)
	    {
	        if(hStmt) 
	        {
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
	        continue;
	    }
	   
	    for(i = 0; i <= nLoopCnt; i++)
	    {
	        sqlite_query_column(hStmt, 0, &nWeekId);
	        sqlite_query_column(hStmt, 1, &nPlanId);
	        sqlite_query_column_text(hStmt, 2, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
	        sqlite_query_column_text(hStmt, 3, Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
	        sqlite_query_column(hStmt, 4, &Schedule[iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
			if(sqlite_query_next(hStmt) != 0)
			{	
				i++; 
				break;
			}
	    }
		if(hStmt)
		{
			sqlite_clear_stmt(hStmt);
			hStmt = 0;
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_all_chns(const char *pDbFile, struct smart_event *smartevent[], int allChn)
{
    if(!pDbFile || !smartevent) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
	int i = 0, ichn = 0; 
    HSQLSTMT hStmt = 0;
    char sQuery[MAX_SMART_EVENT][256];
	char cmd[512] = {0};
    int nColumnCnt = 0;
	
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionin_event");
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_regionexit_event");
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_motion_event");
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_tamper_event");
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_linecross_event");
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_loiter_event");
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_human_event");
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select id,enable,tri_alarms,tri_channels_ex,buzzer_interval,email_interval from vca_people_event");

	for(ichn = 0; ichn < allChn; ichn++)
	{
		for(i = REGIONIN; i < MAX_SMART_EVENT; i++)
		{
			snprintf(cmd, sizeof(cmd), "%s where id=%d;", sQuery[i], ichn);
			int nResult = sqlite_query_record(hConn, cmd, &hStmt, &nColumnCnt);
		    if(nResult != 0 || nColumnCnt == 0)
		    {
		        if(hStmt)
				{ 
					sqlite_clear_stmt(hStmt);
					hStmt = 0;
		        }
		        continue;
		    }
			
			sqlite_query_column(hStmt, 0, &smartevent[ichn][i].id);
		    sqlite_query_column(hStmt, 1, &smartevent[ichn][i].enable);
		    sqlite_query_column(hStmt, 2, (int*)&smartevent[ichn][i].tri_alarms);
			sqlite_query_column_text(hStmt, 3, smartevent[ichn][i].tri_channels_ex, sizeof(smartevent[ichn][i].tri_channels_ex));
			sqlite_query_column(hStmt, 4, &smartevent[ichn][i].buzzer_interval);
			sqlite_query_column(hStmt, 5, &smartevent[ichn][i].email_interval);
			if(hStmt)
			{ 
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
	        }
		}
	}
	
  	if(hStmt)
    	sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_effective_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0, ichn = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	char cmd[512] = {0};
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_effective_schedule");
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_effective_schedule");
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_effective_schedule");
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_effective_schedule");
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_effective_schedule");
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_effective_schedule");
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_effective_schedule");
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_effective_schedule");

	for(ichn = 0; ichn < allChn; ichn++)
	{
		for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
		{
			snprintf(cmd, sizeof(cmd), "%s where chn_id=%d;", sQuery[iSmt], ichn);
			nResult = sqlite_query_record(hConn, cmd, &hStmt, &nColumnCnt);
		    if(nResult != 0 || nColumnCnt == 0)
		    {
		        if(hStmt) 
		        {
					sqlite_clear_stmt(hStmt);
					hStmt = 0;
				}
		        continue;
		    }
		   
		    for(i = 0; i <= nLoopCnt; i++)
		    {
		        sqlite_query_column(hStmt, 0, &nWeekId);
		        sqlite_query_column(hStmt, 1, &nPlanId);
		        sqlite_query_column_text(hStmt, 2, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
		        sqlite_query_column_text(hStmt, 3, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
		        sqlite_query_column(hStmt, 4, &Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
				if(sqlite_query_next(hStmt) != 0)
				{	
					i++; 
					break;
				}
		    }
			if(hStmt)
			{
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_audible_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0, ichn = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	char cmd[512] = {0};
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_audible_schedule");
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_audible_schedule");
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_audible_schedule");
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_audible_schedule");
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_audible_schedule");
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_audible_schedule");
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_audible_schedule");
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_audible_schedule");

	for(ichn = 0; ichn < allChn; ichn++)
	{
		for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
		{
			snprintf(cmd, sizeof(cmd), "%s where chn_id=%d;", sQuery[iSmt], ichn);
			nResult = sqlite_query_record(hConn, cmd, &hStmt, &nColumnCnt);
		    if(nResult != 0 || nColumnCnt == 0)
		    {
		        if(hStmt) 
		        {
					sqlite_clear_stmt(hStmt);
					hStmt = 0;
				}
		        continue;
		    }
		   
		    for(i = 0; i <= nLoopCnt; i++)
		    {
		        sqlite_query_column(hStmt, 0, &nWeekId);
		        sqlite_query_column(hStmt, 1, &nPlanId);
		        sqlite_query_column_text(hStmt, 2, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
		        sqlite_query_column_text(hStmt, 3, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
		        sqlite_query_column(hStmt, 4, &Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
				if(sqlite_query_next(hStmt) != 0)
				{	
					i++; 
					break;
				}
		    }
			if(hStmt)
			{
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_mail_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0, ichn = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	char cmd[512] = {0};
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionen_mail_schedule");
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionex_mail_schedule");
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motionadv_mail_schedule");
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_mail_schedule");
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_mail_schedule");
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_mail_schedule");
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_mail_schedule");
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_peolpe_mail_schedule");

	for(ichn = 0; ichn < allChn; ichn++)
	{
		for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
		{
			snprintf(cmd, sizeof(cmd), "%s where chn_id=%d;", sQuery[iSmt], ichn);
			nResult = sqlite_query_record(hConn, cmd, &hStmt, &nColumnCnt);
		    if(nResult != 0 || nColumnCnt == 0)
		    {
		        if(hStmt) 
		        {
					sqlite_clear_stmt(hStmt);
					hStmt = 0;
				}
		        continue;
		    }
		   
		    for(i = 0; i <= nLoopCnt; i++)
		    {
		        sqlite_query_column(hStmt, 0, &nWeekId);
		        sqlite_query_column(hStmt, 1, &nPlanId);
		        sqlite_query_column_text(hStmt, 2, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
		        sqlite_query_column_text(hStmt, 3, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
		        sqlite_query_column(hStmt, 4, &Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
				if(sqlite_query_next(hStmt) != 0)
				{	
					i++; 
					break;
				}
		    }
			if(hStmt)
			{
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_smart_event_popup_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn)
{
    if(!pDbFile || !Schedule) 
		return -1;
	
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

	int i = 0, iSmt =0, ichn = 0;
	int nResult = 0;
	int nColumnCnt = 0;
	int nWeekId = 1;
    int nPlanId = 0;
    int nLoopCnt = MAX_DAY_NUM*MAX_PLAN_NUM_PER_DAY;
	char sQuery[MAX_SMART_EVENT][256];
	char cmd[512] = {0};
	
    HSQLSTMT hStmt = 0;
	snprintf(sQuery[REGIONIN], sizeof(sQuery[REGIONIN]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionin_popup_schedule");
	snprintf(sQuery[REGIONOUT], sizeof(sQuery[REGIONOUT]), "select week_id,plan_id,start_time,end_time,action_type from vca_regionexit_popup_schedule");
	snprintf(sQuery[ADVANCED_MOTION], sizeof(sQuery[ADVANCED_MOTION]), "select week_id,plan_id,start_time,end_time,action_type from vca_motion_popup_schedule");
	snprintf(sQuery[TAMPER], sizeof(sQuery[TAMPER]), "select week_id,plan_id,start_time,end_time,action_type from vca_tamper_popup_schedule");
	snprintf(sQuery[LINECROSS], sizeof(sQuery[LINECROSS]), "select week_id,plan_id,start_time,end_time,action_type from vca_linecross_popup_schedule");
	snprintf(sQuery[LOITERING], sizeof(sQuery[LOITERING]), "select week_id,plan_id,start_time,end_time,action_type from vca_loiter_popup_schedule");
	snprintf(sQuery[HUMAN], sizeof(sQuery[HUMAN]), "select week_id,plan_id,start_time,end_time,action_type from vca_human_popup_schedule");
	snprintf(sQuery[PEOPLE_CNT], sizeof(sQuery[PEOPLE_CNT]), "select week_id,plan_id,start_time,end_time,action_type from vca_people_popup_schedule");

	for(ichn = 0; ichn < allChn; ichn++)
	{
		for(iSmt = REGIONIN; iSmt < MAX_SMART_EVENT; iSmt++)
		{
			snprintf(cmd, sizeof(cmd), "%s where chn_id=%d;", sQuery[iSmt], ichn);
			nResult = sqlite_query_record(hConn, cmd, &hStmt, &nColumnCnt);
		    if(nResult != 0 || nColumnCnt == 0)
		    {
		        if(hStmt) 
		        {
					sqlite_clear_stmt(hStmt);
					hStmt = 0;
				}
		        continue;
		    }
		   
		    for(i = 0; i <= nLoopCnt; i++)
		    {
		        sqlite_query_column(hStmt, 0, &nWeekId);
		        sqlite_query_column(hStmt, 1, &nPlanId);
		        sqlite_query_column_text(hStmt, 2, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].start_time));
		        sqlite_query_column_text(hStmt, 3, Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time, sizeof(Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].end_time));
		        sqlite_query_column(hStmt, 4, &Schedule[ichn][iSmt].schedule_day[nWeekId].schedule_item[nPlanId].action_type);
				if(sqlite_query_next(hStmt) != 0)
				{	
					i++; 
					break;
				}
		    }
			if(hStmt)
			{
				sqlite_clear_stmt(hStmt);
				hStmt = 0;
			}
		}
	}
	
    if(hStmt)
    {
		sqlite_clear_stmt(hStmt);
		hStmt = 0;
	}
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_smart_event_effective_schedule_default(const char *pDbFile, int chn_id)
{
    if(!pDbFile || chn_id <0) 
		return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[256] = {0};
	char db_table[MAX_SMART_EVENT][64];
	char db_mot_table[64] = "motion_effective_schedule";
	
	snprintf(db_table[REGIONIN], sizeof(db_table[REGIONIN]), "%s", "vca_regionen_effective_schedule");
	snprintf(db_table[REGIONOUT], sizeof(db_table[REGIONOUT]), "%s", "vca_regionex_effective_schedule");
	snprintf(db_table[ADVANCED_MOTION], sizeof(db_table[ADVANCED_MOTION]), "%s", "vca_motionadv_effective_schedule");
	snprintf(db_table[TAMPER], sizeof(db_table[TAMPER]), "%s", "vca_tamper_effective_schedule");
	snprintf(db_table[LINECROSS], sizeof(db_table[LINECROSS]), "%s", "vca_linecross_effective_schedule");
	snprintf(db_table[LOITERING], sizeof(db_table[LOITERING]), "%s", "vca_loiter_effective_schedule");
	snprintf(db_table[HUMAN], sizeof(db_table[HUMAN]), "%s", "vca_human_effective_schedule");
	snprintf(db_table[PEOPLE_CNT], sizeof(db_table[PEOPLE_CNT]), "%s", "vca_peolpe_effective_schedule");

	int i = 0, iSmt = 0;
	for(iSmt = 0; iSmt < MAX_SMART_EVENT; iSmt++)
	{
		snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_table[iSmt], chn_id);
		sqlite_execute(hConn, sExec);
		for(i = 0; i < MAX_DAY_NUM; i++)
		{
			snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_table[iSmt], chn_id, i, 0, "00:00", "24:00", SMART_EVT_RECORD);
			sqlite_execute(hConn, sExec);
		}
	}

	snprintf(sExec, sizeof(sExec), "delete from %s where chn_id=%d;", db_mot_table, chn_id);
	sqlite_execute(hConn, sExec);
	for(i = 0; i < MAX_DAY_NUM; i++)
	{
		snprintf(sExec, sizeof(sExec), "insert into %s values(%d,%d,%d, '%s', '%s', %d);", db_mot_table, chn_id, i, 0, "00:00", "24:00", MOTION_ACTION);
		sqlite_execute(hConn, sExec);
	}
	
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}


int read_failover(const char *pDbFile, struct failover_list *failover, int id)
{
    if(!pDbFile || !failover) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[2048] = {0};
    int nColumnCnt = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,ipaddr,port,mac,model,username,password,start_time,end_time from failover_list where id = %d;", id);
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    sqlite_query_column(hStmt, 0, &failover->id);
    sqlite_query_column(hStmt, 1, &failover->enable);
    sqlite_query_column_text(hStmt, 2, failover->ipaddr, sizeof(failover->ipaddr));
    sqlite_query_column(hStmt, 3, &failover->port);
    sqlite_query_column_text(hStmt, 4, failover->mac, sizeof(failover->mac));
    sqlite_query_column_text(hStmt, 5, failover->model, sizeof(failover->model));
    sqlite_query_column_text(hStmt, 6, failover->username, sizeof(failover->username));
    sqlite_query_column_text(hStmt, 7, failover->password, sizeof(failover->password));
    sqlite_query_column_text(hStmt, 8, failover->start_time, sizeof(failover->start_time));
    sqlite_query_column_text(hStmt, 9, failover->end_time, sizeof(failover->end_time));
	
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int read_failovers(const char *pDbFile, struct failover_list failovers[], int *cnt)
{
    if(!pDbFile || !failovers) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'r', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    HSQLSTMT hStmt = 0;
    char sQuery[2048] = {0};
    int nColumnCnt = 0;
    int i = 0;
    snprintf(sQuery, sizeof(sQuery), "select id,enable,ipaddr,port,mac,model,username,password,start_time,end_time from failover_list;");
    int nResult = sqlite_query_record(hConn, sQuery, &hStmt, &nColumnCnt);
    if(nResult !=0 || nColumnCnt == 0)
    {
        *cnt = 0;
        if(hStmt) sqlite_clear_stmt(hStmt);
        sqlite_disconn(hConn);
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }

    for(i=0; i<MAX_FAILOVER; i++)
    {
        sqlite_query_column(hStmt, 0, &failovers[i].id);
        sqlite_query_column(hStmt, 1, &failovers[i].enable);
        sqlite_query_column_text(hStmt, 2, failovers[i].ipaddr, sizeof(failovers[i].ipaddr));
        sqlite_query_column(hStmt, 3, &failovers[i].port);
        sqlite_query_column_text(hStmt, 4, failovers[i].mac, sizeof(failovers[i].mac));
        sqlite_query_column_text(hStmt, 5, failovers[i].model, sizeof(failovers[i].model));
        sqlite_query_column_text(hStmt, 6, failovers[i].username, sizeof(failovers[i].username));
        sqlite_query_column_text(hStmt, 7, failovers[i].password, sizeof(failovers[i].password));
        sqlite_query_column_text(hStmt, 8, failovers[i].start_time, sizeof(failovers[i].start_time));
        sqlite_query_column_text(hStmt, 9, failovers[i].end_time, sizeof(failovers[i].end_time));

		if(sqlite_query_next(hStmt) != 0){i++; break;}
    }

    *cnt = i;
    sqlite_clear_stmt(hStmt);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

int write_failover(const char *pDbFile, struct failover_list *failover)
{
    if(!pDbFile || !failover) return -1;
    int nFd = FileLock(FILE_LOCK_NAME, 'w', &global_rwlock);
    HSQLITE hConn = 0;
    if(sqlite_conn(pDbFile, &hConn) != 0)
    {
        FileUnlock(nFd, &global_rwlock);
        return -1;
    }
    char sExec[2048] = {0};

    char buf1[MAX_PWD_LEN*2+2] = {0};
	if (failover->id == 0)
    {   
	    translate_pwd(buf1,failover->password,strlen(failover->password));
    }
	
    snprintf(sExec, sizeof(sExec), "update failover_list set enable=%d,ipaddr='%s',port=%d,mac='%s',model='%s',username='%s',password='%s',start_time='%s',end_time='%s' where id=%d;", failover->enable, failover->ipaddr, failover->port, failover->mac, failover->model, failover->username, buf1, failover->start_time, failover->end_time, failover->id);
    sqlite_execute(hConn, sExec);
    sqlite_disconn(hConn);
    FileUnlock(nFd, &global_rwlock);
    return 0;
}

