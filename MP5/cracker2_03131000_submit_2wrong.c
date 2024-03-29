/**
* Password Cracker Lab
* CS 241 - Spring 2018
*/

#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "queue.h"
#include <crypt.h>
#include "math.h"
#include "unistd.h"
struct task{
	char* name;
	int threadId;
	pthread_t id;
	size_t thread_count;
	char* hash;
	char* password;
};
typedef struct task task;
static queue *queue_task;
static pthread_barrier_t barrier;
static int find = 0;
static int totalHashCount = 0;
static int pull = 0;
static task* current;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
char **strsplit(const char *,const char *, size_t *);
void free_args(char **);
void* password_cracker(void* task_input){
		pthread_mutex_lock(&mtx);
		if (!pull){
			current = (task*)queue_pull(queue_task);
			v2_print_start_user(current->name);			
			pull = 1;
		}
		task *t = malloc(sizeof(task));
		memmove(t,current,sizeof(task));
		t->password = strdup(current->password);			
		pthread_mutex_unlock(&mtx);
		
		t->threadId = *(int*)task_input;
		double start_time = getThreadCPUTime();
		double start_cpu_time = getCPUTime();
		//double startTime = getThreadCPUTime();
		//task *t = (task*)task_input ;
		//printf("password1:%s\n",t->password);
		int unknown_letter_count = strlen(t->password) - getPrefixLength(t->password);
		long start_index, count;
		//printf("unknown:%d,thread_count:%zd,t_id:%d\n",unknown_letter_count,t->thread_count, t->threadId);
		getSubrange(unknown_letter_count, t->thread_count, t->threadId, &start_index, &count);
		char* startPassword = t->password + getPrefixLength(t->password);
		//printf("password:%s\n",startPassword);
		//printf("count:%ld,start_index:%ld\n",count,start_index);
		setStringPosition(startPassword , start_index);
		v2_print_thread_start(t->threadId, t->name, start_index, t->password);
		struct crypt_data cdata;
		cdata.initialized = 0;
		int result = 1;
		char *hashed;
		int hashCount = 1;
		char* answer = t->password ;
		int thisfind = 0;
		while(hashCount <= count){
			pthread_mutex_lock(&mtx);
			if (find == 1) {
				pthread_mutex_unlock(&mtx);
				break;
			}
			pthread_mutex_unlock(&mtx);
			hashed = crypt_r(answer, "xx", &cdata);
			if (strcmp(hashed, t->hash)==0) {
				pthread_mutex_lock(&mtx);
				find = 1;
				pthread_mutex_unlock(&mtx);
				result = 0;
				thisfind = 1;
				break;
			}			
			if (incrementString(startPassword) == 0 ) result = 2; //if reach end, result = 2
			if (hashCount == count) {
				result = 2;
				break;
			}
			if (result == 1) hashCount ++;
		} 
		pthread_mutex_lock(&mtx);
		totalHashCount += hashCount;	
		//double timeElapsed = getThreadCPUTime() - startTime;
		v2_print_thread_result(t->threadId, hashCount, result);
		pthread_mutex_unlock(&mtx);
		pthread_barrier_wait(&barrier);
		double timeElapsed = getThreadCPUTime() - start_time;
		double totalCPUTime = getCPUTime() - start_cpu_time;
		pthread_mutex_lock(&mtx);
		if (thisfind == 1 || find == 0){
			v2_print_summary(t->name, t->password, totalHashCount, timeElapsed, totalCPUTime, result);
			find = 1;
		}
		pthread_mutex_unlock(&mtx);
		free (t->password);
		free (t);
    return NULL;
}
int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
	char *buffer = NULL;
	size_t size = 0;
	ssize_t chars ;
	size_t password_count = 0;
	pthread_t *t = malloc(sizeof(pthread_t)*thread_count);
	task *password_t = malloc(sizeof(task));
	queue_task = queue_create(-1);
	pthread_barrier_init(&barrier, NULL,thread_count);
	while((chars = getline(&buffer, &size, stdin))!= -1){
				// Discard newline character if it is present,			
		if (chars > 0 && buffer[chars-1] == '\n') 
			buffer[chars-1] = '\0';
	//	printf("%s\n",buffer);
		char **tokens;
		size_t numtokens;
		password_count++ ;
		password_t = realloc(password_t,sizeof(task)*password_count);
		tokens = strsplit(buffer, " ", &numtokens);			
		password_t[password_count-1].name = strdup(tokens[0]);
		password_t[password_count-1].threadId = 0;
		//printf("p:%p\n",(void*)&password_t[password_count-1]);
		password_t[password_count-1].hash = strdup(tokens[1]);
		password_t[password_count-1].password = strdup(tokens[2]);
		password_t[password_count-1].thread_count = thread_count;
		free_args(tokens);		
	}
	int password_remain = password_count;
	for (size_t i=0; i<password_count ; i++)
		queue_push(queue_task,(void*)&password_t[i]);
	int data[thread_count];
	//task *new = malloc(sizeof(task)*thread_count);
	for (size_t i = 1; i <= thread_count; i++){
		data[i-1] = i;
	}
	//printf("%zd\n",password_count);
	while(password_remain){
		find = 0;
		pull = 0;
		//task *temp = (task*)queue_pull(queue_task);	
		//printf("p:%p\n",temp);
		//printf("%s\n",temp->password);
		//v2_print_start_user(temp->name);
		/*for(int i =0; i < (int)thread_count; i++) {
			memmove(new+i,temp,sizeof(task));
			//printf("new_p:%p\n",new+i);
			new[i].password = strdup(temp->password);
			new[i].threadId = data[i];
			pthread_create(&t[i], NULL, password_cracker, &new[i]);
		}*/
		for(int i =0; i < (int)thread_count; i++) {
			pthread_create(&t[i], NULL, password_cracker, &data[i]);
		}
		void* result; 
		for(int i =0; i < (int)thread_count; i++) {
			pthread_join(t[i], &result);
		}
		/*for(int i =0; i < (int)thread_count; i++) {
			free(new[i].password);
		}*/
		totalHashCount = 0;
		password_remain--;
	}
	//free(new);
	free(buffer);
	for (size_t i=0; i<password_count ; i++){
		free(password_t[i].name);
		free(password_t[i].hash);
		free(password_t[i].password);
	}
	free(password_t);
	queue_destroy(queue_task);
	free(t);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}


char **strsplit(const char *str, const char *delim, size_t *numtokens) {
    // copy the original string so that we don't overwrite parts of it
    // (don't do this if you don't need to keep the old line,
    // as this is less efficient)
    char *s = strdup(str);
    // these three variables are part of a very common idiom to
    // implement a dynamically-growing array
    size_t tokens_alloc = 1;
    size_t tokens_used = 0;
    char **tokens = calloc(tokens_alloc, sizeof(char *));
    char *token, *strtok_ctx;
    for (token = strtok_r(s, delim, &strtok_ctx); token != NULL;
         token = strtok_r(NULL, delim, &strtok_ctx)) {
        // check if we need to allocate more space for tokens
        if (tokens_used == tokens_alloc) {
            tokens_alloc *= 2;
            tokens = realloc(tokens, tokens_alloc * sizeof(char *));
        }
        tokens[tokens_used++] = strdup(token);
    }
    // cleanup
    if (tokens_used == 0) {
        free(tokens);
        tokens = NULL;
    } else {
        tokens = realloc(tokens, tokens_used * sizeof(char *));
    }
    *numtokens = tokens_used;
    free(s);
    // Adding a null terminator
    tokens = realloc(tokens, sizeof(char *) * (tokens_used + 1));
    tokens[tokens_used] = NULL;
    return tokens;
}

void free_args(char **args) {
    char **ptr = args;
    while (*ptr) {
        free(*ptr);
        ptr++;
    }
    free(args);
}
