/* -------------------------------------------------------------------------- */
/* Copyright 2002-2013 GridWay Project Leads (GridWay.org)                    */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#include "gw_em_mad_cream.h"

#include <iostream>
#include <string>
#include <pthread.h>

const char * usage =
"USAGE\n gw_em_mad_cream [-h] [-d] [-t]\n\n"
"SYNOPSIS\n"
"  Execution driver to interface with CREAM services. It is not intended to be used from CLI.\n\n"
"OPTIONS\n"
"  -h    print this help\n"
"  -d    delegation id\n"
"  -t    credentials refresh time";

const char * susage =
"usage: gw_em_mad_cream [-h] [-d] [-t]";

void *creamAction(void *thread_data);
void *timer(void *);
void *polling(void *);

using namespace std;
	
extern char *optarg;
extern int   optopt;

typedef struct thread_operation_s{
    string action;
    int jidCREAM;
    string contact;
    string jdlFile;
} thread_operation_t;

pthread_mutex_t mutex;
pthread_cond_t cond;
int num_active = 0;
CreamEmMad *creamEmMad = NULL;
Namespace namespaces[] = {};

int main( int argc, char **argv) 
{
    char str[4096];
    char str1[20];
    char str2[20];
    char str3[500];
    char str4[1024];
    bool end = false;
    string action;
    string contact;
    string jdlFile;
    string delegation;
    int refreshTime = 0;
    char opt;
    int paramNum;
    int jidCREAM;
    thread_operation_t *operation;

    pthread_t creamTimer;
    pthread_t creamPolling;
    pthread_t thread_id;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&cond, 0);

    while((opt = getopt(argc, argv, "d:t:h")) != -1)
        switch(opt)
        {
            case 'd': 
                delegation = optarg;
                break;
            case 't': 
           	refreshTime = atoi(optarg);
		break;
            case 'h':
                cout << usage << endl;
                exit(0);
                break;
            case '?':
                cout << "error: invalid option " << (char)optopt << endl;
		cout << susage << endl;
                exit(1);
                break;                
	}

    while (!end)
    {
        cin.getline(str,4096,'\n');

        paramNum = sscanf(str, "%s %s %s %[^\n]", str1, str2, str3, str4);

        if (paramNum != 4)
        {
            cout << "FAILURE Not all four arguments defined" << endl;
            continue;
        }

        action = str1;
        jidCREAM = atoi(str2);
        contact = str3;
        jdlFile = str4;

        if (creamEmMad == NULL)
            if (action.compare("INIT") == 0)
            {
		creamEmMad = new CreamEmMad(delegation, refreshTime);
                creamEmMad->init();

                pthread_create(&creamTimer, &attr, timer, NULL);
                pthread_create(&creamPolling, &attr, polling, NULL);
            }
            else
               cout << action << " " << jidCREAM << " FAILURE Not initialized" << endl;
        else if (action.compare("INIT") == 0)
               cout << action << " " << jidCREAM << " FAILURE Already initialized" << endl;
        else if (action.compare("FINALIZE") == 0)
        {
            end = true;
	    pthread_cancel(creamTimer);
            pthread_cancel(creamPolling);
            pthread_attr_destroy(&attr); 
            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
            creamEmMad->finalize();
        }
        else if ( (action.compare("SUBMIT") == 0) || (action.compare("POLL") == 0) || (action.compare("CANCEL") == 0) || (action.compare("RECOVER") == 0) ) {
            operation = new thread_operation_t;
            operation->action = action;
            operation->jidCREAM = jidCREAM;
            operation->contact = contact;
            operation->jdlFile = jdlFile;
            pthread_mutex_lock(&mutex);
                while (num_active >= MAX_THREADS)
                    pthread_cond_wait(&cond, &mutex); 
                num_active++;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread_id, &attr, creamAction, operation);
        }
	else
	    cout << "FAILURE " << action << " is not a valid action" << endl;
  } 
}

void *timer(void *)
{
    pthread_detach(pthread_self());
 
    creamEmMad->timer();
    pthread_exit(NULL);
}

void *polling(void *)
{
    pthread_detach(pthread_self());

    creamEmMad->polling();
    pthread_exit(NULL);
}

void *creamAction(void *thread_data)
{
    thread_operation_t *data;
    data = (thread_operation_t*) thread_data;

    string action =  data->action;
    string contact = data->contact;
    string jdlFile = data->jdlFile;
    int jidCREAM = data->jidCREAM;
    delete data;
    string host;

    pthread_detach(pthread_self());

    if (action.compare("SUBMIT") == 0)
    {
        host = contact.substr(0, contact.find("/"));
        creamEmMad->submit(jidCREAM, host, jdlFile);
    }
    else if (action.compare("RECOVER") == 0)
    {
        creamEmMad->recover(jidCREAM, contact);
    }
    else if (action.compare("CANCEL") == 0)
        creamEmMad->cancel(jidCREAM);
    else if (action.compare("POLL") == 0)
        creamEmMad->poll(jidCREAM);

    pthread_mutex_lock(&mutex);
        num_active--;
        pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}
