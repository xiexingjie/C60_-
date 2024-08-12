#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "hashtable.h"
#include "str_util.h"

#define IP "ip"
#define PORT "port"
#define THREAD_NUM "thread_num"
#define MYSQL_IP "mysql_ip"
#define MYSQL_PORT "mysql_port"
#define MYSQL_USERNAME "mysql_username"
#define MYSQL_PASSWORD "mysql_password"
#define MYSQL_DATABASE "mysql_database"


void readConfig(const char* filename, HashTable * ht);

#endif
