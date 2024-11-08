#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;
  u32 time_left;
  u32 added;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);
  for(u32  k=0; k<size; k++){
    data[k].added=0;
  }
  struct process_list list;
  TAILQ_INIT(&list);
 
  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  
  int finishTime=0;
  struct process* running=NULL;
  int num_added=0;
  for(int i=0; (num_added<size  ||  !TAILQ_EMPTY(&list)) ; i++){
  
    for(u32 k=0; k<size; k++){
      if(data[k].added==0){
	
      if(data[k].arrival_time==i){
	data[k].time_left=data[k].burst_time;
	data[k].added=1;
	TAILQ_INSERT_TAIL(&list,&(data[k]), pointers);
	num_added++;
      }
      }

    }
    if(running==NULL){
      running=TAILQ_FIRST(&list);
      if(running!=NULL){
	if(running->time_left==running->burst_time){
	  int response_time=i-(running->arrival_time);
	  total_response_time+=response_time;
	}
	int timeRemaining=running->time_left-quantum_length;
	if(timeRemaining<=0){
	  finishTime=i+running->time_left;
	  running->time_left=0;
	  
	  
	}else{
	  running->time_left=timeRemaining;
	  finishTime=i+quantum_length;
	}
	printf("Time %d: Process %u runs for %d units\n", i, running->pid, running->time_left);

	
      }
    }

    if(running!=NULL){
      

      if(i==finishTime){
	
	if(running->time_left<=0){
	  u32  wait_time=i-(running->arrival_time)-(running->burst_time);
	  total_waiting_time+=wait_time;
	  TAILQ_REMOVE(&list, running, pointers);
	  
	  running=NULL;
	  
	}else{
	  TAILQ_REMOVE(&list, running, pointers);
	  TAILQ_INSERT_TAIL(&list, running, pointers);
	  running=NULL;
	  
	}
      }


    }
    
    
  }
  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

