/*
Copyright 2009 Yoichi Kawasaki

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MYSQL_SERVER
#include <mysql_priv.h>
#include <mysql/plugin.h>
#ifdef OLD
#include <my_global.h>
#include <mysql_version.h>
#include <my_dir.h>
#endif

#define L_STORE_ROW_LONG(a,b,c)  \
		if (a) {                          \
			a->field[b]->store(c);        \
		}

#define L_STORE_ROW_STRING(a,b,c,d)     \
		if (a) {                               \
			a->field[b]->store(c,strlen(c),d); \
		}

bool schema_table_store_record(THD *thd, TABLE *table);

ST_FIELD_INFO is_locks_field_info[]=
{
    {"Thread_id", MY_INT64_NUM_DECIMAL_DIGITS, MYSQL_TYPE_LONG, 0,0, NULL},
    {"Database", 255, MYSQL_TYPE_STRING, 0,0, NULL},
    {"Table", 255, MYSQL_TYPE_STRING, 0,0, NULL},
    {"Locked", MY_INT64_NUM_DECIMAL_DIGITS, MYSQL_TYPE_LONG, 0,0, NULL},
    {"Waiting", MY_INT64_NUM_DECIMAL_DIGITS, MYSQL_TYPE_LONG, 0,0, NULL},
    {"Type", 255, MYSQL_TYPE_STRING, 0,0, NULL},
    {"Desription", 255, MYSQL_TYPE_STRING, 0,0, NULL},
    {"Query", 1055, MYSQL_TYPE_STRING, 0,0, NULL},
    {0, 0, MYSQL_TYPE_STRING, 0, 0, 0}
};

static const char *lock_descriptions[] =
{
    "No lock",
    "Low priority read lock",
    "Shared Read lock",
    "High priority read lock",
    "Read lock  without concurrent inserts",
    "Write lock that allows other writers",
    "Write lock, but allow reading",
    "Concurrent insert lock",
    "Lock Used by delayed insert",
    "Low priority write lock",
    "High priority write lock",
    "Highest priority write lock"
};

static int fill_rows_into_table ( THD *thd, TABLE * table, CHARSET_INFO *scs,
                    THR_LOCK_DATA *data, int locked, int waiting, int type )
{
    if ( !thd || !table || !scs || !data ) {
        DBUG_RETURN(0);
    }
    TABLE *entry = (TABLE *)data->debug_print_param;
    if (entry && entry->s->tmp_table == NO_TMP_TABLE) {
        char table_name[FN_REFLEN];
        /* treahd id */
        L_STORE_ROW_LONG(table, 0, (longlong) entry->in_use ? entry->in_use->thread_id : 0L );
        /* database name */
        L_STORE_ROW_STRING(table, 1, entry->s->db.str, scs);
        /* table name */
        L_STORE_ROW_STRING(table, 2, entry->s->table_name.str, scs);
        /* Locked */
        L_STORE_ROW_LONG(table, 3, (longlong) locked );
        /* Waiting */
        L_STORE_ROW_LONG(table, 4, (longlong) waiting );
        /* Type 0-read 1-write */
        L_STORE_ROW_STRING(table, 5, ( type==1 ) ? "write" : "read" , scs);
        /* Description */
        L_STORE_ROW_STRING(table, 6,
            entry->in_use ? lock_descriptions[(int)entry->reginfo.lock_type] : "NULL", scs);
        /* Query */
        L_STORE_ROW_STRING(table, 7,
            (entry->in_use && entry->in_use->query) ? entry->in_use->query : "NULL", scs);

        if ( schema_table_store_record(thd, table)) {
            DBUG_RETURN(1);
        }
    }
    DBUG_RETURN(0);
}

/********************************************************************
*  CODE partly from void thr_print_locks(void) of mysys/thr_lock.c
*********************************************************************/
#define L_MAX_THREADS 100
static int fill_is_locks_schema(THD *thd, TABLE_LIST *tables, COND *cond)
{
    DBUG_ENTER("fill_is_locks_schema");
    CHARSET_INFO *scs= system_charset_info;
    TABLE *table= (TABLE *) tables->table;
    LIST *list;
    uint thread_count=0;

    pthread_mutex_lock(&THR_LOCK_lock);
    for (list= thr_lock_thread_list;
            list && thread_count++ < L_MAX_THREADS;
            list= list_rest(list) )
    {
        THR_LOCK *lock=(THR_LOCK*) list->data;
        VOID(pthread_mutex_lock(&lock->mutex));
        if (lock->write.data) {
            if( fill_rows_into_table( thd, table, scs, lock->write.data, 1, 0, 1 ) ) {
                VOID(pthread_mutex_unlock(&lock->mutex));
                pthread_mutex_unlock(&THR_LOCK_lock);
                DBUG_RETURN(1);
            }
        }
        if (lock->write_wait.data) {
            if( fill_rows_into_table( thd, table, scs, lock->write_wait.data, 0, 1, 1 ) ) {
                VOID(pthread_mutex_unlock(&lock->mutex));
                pthread_mutex_unlock(&THR_LOCK_lock);
                DBUG_RETURN(1);
            }
        }
        if (lock->read.data) {
            if( fill_rows_into_table( thd, table, scs, lock->read.data, 1, 0, 0 ) ) {
                VOID(pthread_mutex_unlock(&lock->mutex));
                pthread_mutex_unlock(&THR_LOCK_lock);
                DBUG_RETURN(1);
            }
        }
        if (lock->read_wait.data) {
            if( fill_rows_into_table( thd, table, scs, lock->read_wait.data, 0, 1, 0 ) ) {
                VOID(pthread_mutex_unlock(&lock->mutex));
                pthread_mutex_unlock(&THR_LOCK_lock);
                DBUG_RETURN(1);
            }
        }
        VOID(pthread_mutex_unlock(&lock->mutex));
    }
    pthread_mutex_unlock(&THR_LOCK_lock);
    DBUG_RETURN(0);
}

static int is_locks_plugin_init(void *p)
{
    DBUG_ENTER("is_locks_plugin_init");
    ST_SCHEMA_TABLE *schema= (ST_SCHEMA_TABLE *)p;
    schema->fields_info= is_locks_field_info;
    schema->fill_table= fill_is_locks_schema;
    DBUG_RETURN(0);
}

static int is_locks_plugin_deinit(void *p)
{
    DBUG_ENTER("is_locks_plugin_deinit");
    DBUG_RETURN(0);
}

struct st_mysql_information_schema is_locks_plugin=
{ MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION };

/*
  Plugin library descriptor
*/
mysql_declare_plugin(is_locks)
{
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &is_locks_plugin,
    "TABLE_LOCKS",
    "Yoichi Kawasaki",
    "table locks information schema interface.",
    PLUGIN_LICENSE_GPL,
    is_locks_plugin_init,   /* Plugin Init */
    is_locks_plugin_deinit, /* Plugin Deinit */
    0x0010                          /* 1.0 */,
    NULL,                           /* status variables                */
    NULL,                           /* system variables                */
    NULL                            /* config options                  */
}
mysql_declare_plugin_end;


