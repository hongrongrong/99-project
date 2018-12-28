#include "ccsqlite.h"
//#include "sqlite3.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //memcpy
#include "msstd.h"
#define SQLITE_OPT_CNT 200
static void sqlite_debug(const char* pDebug, ...)
{
	#define STARTUP_DEBUG_MODE//bruce.milesight
	#ifndef STARTUP_DEBUG_MODE
	return;
	#endif
	char sDebugAll[1024] = {0};
    va_list  va;
    va_start (va, pDebug);
    vsprintf(sDebugAll, pDebug, va);
    va_end(va);
    mslogerr(DEBUG_ERR, "[sqlite_debug]%s", sDebugAll);
}


int sqlite_conn(const char* pSqlFile, HSQLITE* phConn) 
{
	if (!pSqlFile || !phConn)
	{
		sqlite_debug("param is invalid, SqlFile:%x  Handle:%x\n", pSqlFile, phConn);
		return -1;
	}
	int nOpenRet = sqlite3_open(pSqlFile, (struct sqlite3 **)phConn);
	if (nOpenRet != SQLITE_OK)
	{
		sqlite_debug("open sqlite:%s fail with:%d", pSqlFile, nOpenRet);
		return -1;
	}
	sqlite3_busy_handler(*phConn, NULL, NULL);//mjx.milesight add version:2.0.7 2014.10.17
	return 0;
}


int  sqlite_disconn(HSQLITE hConn)
{
	if (!hConn)
		return -1;
	//sqlite_debug("#############sqlite_disconn####sqlite_disconn####################\n");
	if (sqlite3_close((struct sqlite3 *)hConn) != SQLITE_OK)
	{
		sqlite_debug("sqlite close fail, error:%s\n", sqlite3_errmsg(hConn));
		return -1;
	}
	return 0;
}

int  sqlite_execute(HSQLITE hSqlite, const char* pExec)
{
	int count = 0;
	if (!hSqlite || !pExec)
	{
		sqlite_debug("sql execute param is invalid\n");
		return -1;
	}
	char* pErrMsg = 0;
	while(sqlite3_exec(hSqlite, pExec, 0, 0, &pErrMsg) != SQLITE_OK)
	{
		count++;		
		if(count >= SQLITE_OPT_CNT)
		{
			if(pErrMsg)
			{
				if(strstr(pErrMsg, "database is locked") != NULL)
				{
					system("fuser -m /mnt/nand/app/msdb.db >> /mnt/nand/err_log.txt");
					sqlite3_free(pErrMsg);
					usleep(10000);
					continue;
				}
			}
			sqlite_debug("sql execute fail, %s=count:%d=pErrMsg:%s===\n", pExec, count, pErrMsg);
			if(pErrMsg)
				sqlite3_free(pErrMsg);
			return -1;
		}
		//sqlite_debug("error:%s\n",pErrMsg);
		if(pErrMsg)
			sqlite3_free(pErrMsg);
		usleep(10000);
	}
	return 0;
}

const char* sqlite_query_column_name(HSQLITE hSqlite, HSQLSTMT hStmt, int index)
{
	if (!hSqlite || !hStmt)
		return 0;
	return sqlite3_column_name(hStmt, index);
}

int  sqlite_query_record(HSQLITE hSqlite, const char* pQuery, HSQLSTMT* pStmt, int* pClmCnt)
{
	int count = 0;
	int ret = 0;
	if (!hSqlite || !pQuery || !pStmt || !pClmCnt)
	{
		sqlite_debug("sql query record, param is invalid\n");
		return -1;
	}
	//sqlite_debug("sql:%s\n", pQuery);
	while((ret = sqlite3_prepare_v2( (struct sqlite3 *)hSqlite, pQuery, -1, (struct sqlite3_stmt **)pStmt, 0)) != SQLITE_OK )
	{
		count++;		
		if(count >= SQLITE_OPT_CNT)
		{
			sqlite_debug("sql query record fail,ret=%d, %s==count:%d=\n", ret, pQuery, count);
			return -1;		
		}
		usleep(10000);
	}
	//sqlite_debug("sql:%s\n", pQuery);
	count = 0;
	while ((ret=sqlite3_step(*pStmt)) != SQLITE_ROW)
	{
		if(ret == SQLITE_DONE)
			return 1;
		count++;
		if(count >= SQLITE_OPT_CNT)
		{
			sqlite_debug("sql query record sqlite3_step fail,ret=%d, %s==count:%d=\n", ret, pQuery, count);
			return ret;		
		}		
		usleep(10000);
	}
	*pClmCnt = sqlite3_column_count(*pStmt);
	return 0;
}

int  sqlite_query_next(HSQLSTMT hStmt)
{
	if (!hStmt)
	{
		sqlite_debug("sql query record fail, param is invalid\n");
		return -1;		
	}
	if (sqlite3_step(hStmt) != SQLITE_ROW)
	{
		return 1;		
	}
	return 0;
}


int  sqlite_query_column(HSQLSTMT hStmt, int pClmIndex, int* sDataInfo)
{
	if (!hStmt || pClmIndex<0 || !sDataInfo)
	{
		sqlite_debug("sql query data fail, param is invalid\n");
		return -1;		
	}
	*sDataInfo = sqlite3_column_int(hStmt, pClmIndex);
	return 0;
}

int  sqlite_query_column_double(HSQLSTMT hStmt, int pClmIndex, double* fDataInfo)
{
	if (!hStmt || pClmIndex<0 || !fDataInfo)
	{
		sqlite_debug("sql query data fail, param is invalid\n");
		return -1;		
	}
	*fDataInfo = sqlite3_column_double(hStmt, pClmIndex);
	return 0;
}

int  sqlite_clear_stmt(HSQLSTMT hStmt)
{
	if (!hStmt)
	{
		sqlite_debug("sql clear fail\n");
		return -1;
	}
	sqlite3_finalize(hStmt);
    return 0;
}

int sqlite_query_column_byte(HSQLSTMT hStmt, int pClmIndex, unsigned char* sDataInfo, int size)
{
	if (!hStmt || pClmIndex<0 || !sDataInfo)
	{
		sqlite_debug("sql query data fail, param is invalid\n");
		return -1;		
	}
	int value = sqlite3_column_int(hStmt, pClmIndex);
	*sDataInfo = (unsigned char)value;
	
	return 0;	
}

int  sqlite_query_column_text(HSQLSTMT hStmt, int pClmIndex, char* sDataInfo, int nBufLen)
{
	if (!hStmt || pClmIndex<0 || !sDataInfo || nBufLen<=0)
	{
		sqlite_debug("sql query data fail, param is invalid\n");
		return -1;		
	}
	const unsigned char* pData = sqlite3_column_text(hStmt, pClmIndex);
	if (pData)
	{
		snprintf(sDataInfo, nBufLen, "%s", pData);
	}
	return 0;
}

int sqlite_query_column_blob(HSQLSTMT hStmt, int pClmIndex, void *pData, int size)
{
	if (!hStmt || pClmIndex < 0 || !pData || size <= 0)
	{
		sqlite_debug("sql query data fail, param is invalid\n");
		return -1;
	}
	const void *data = sqlite3_column_blob(hStmt, pClmIndex);
	int bytes = sqlite3_column_bytes(hStmt, pClmIndex);
	if (data && bytes > 0)
	{
		memcpy(pData, data, bytes);
	}
	else
	{
		sqlite_debug("sqlite_query_column_blob failed.(bytes:%d==size:%d)\n", bytes, size);
	}	
	return 0;
}

int  sqlite_prepare_blob(HSQLITE hSqlite, const char* pQuery, HSQLSTMT* pStmt, int query)
{
	int count = 0, ret = 0;
	if (!hSqlite || !pQuery || !pStmt)
	{
		sqlite_debug("sql prepare blob, param is invalid\n");
		return -1;
	}
	while ((ret=sqlite3_prepare_v2( (struct sqlite3 *)hSqlite, pQuery, -1, (struct sqlite3_stmt **)pStmt, 0)) != SQLITE_OK )
	{
		count++;		
		if(count >= SQLITE_OPT_CNT)
		{
			sqlite_debug("sql prepare blob fail,ret=%d, %s==count:%d=\n", ret, pQuery, count);
			return -1;		
		}
		usleep(10000);
	}
	if (query) 
	{
		count = 0;
		while ((ret=sqlite3_step(*pStmt)) != SQLITE_ROW) 
		{
			if(ret == SQLITE_DONE)
				return 1;
			count++;
			if(count >= SQLITE_OPT_CNT)
			{
				sqlite_debug("sql blob sqlite3_step fail,ret=%d, %s==count:%d=\n", ret, pQuery, count);
				return 1;
			}
			usleep(10000);
		}
	}
	return 0;
}

int sqlite_exec_blob(HSQLSTMT pStmt, int pCol, void *pBuf, int size)
{
	if (!pStmt || !pBuf) {
		sqlite_debug("sql exec blob error, invalid argument\n");
		return -1;
	}
	int ret = sqlite3_bind_blob(pStmt, pCol, pBuf, size, NULL);
	if (ret != SQLITE_OK) {
		sqlite_debug("sql exec blob error: bind_blob\n");
		return -1;
	}
	if (sqlite3_step(pStmt) != SQLITE_DONE) {
		sqlite_debug("sql exec blob error: step\n");
		return -1;
	}
	return 0;
}
