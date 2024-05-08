#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <string.h>
#include <devices/shutdown.h>
#include <devices/input.h>
#include <threads/vaddr.h>
#include <threads/malloc.h>
#include <threads/palloc.h>
#include <filesys/file.h>
#include <filesys/filesys.h>
#include "process.h"
#include "pagedir.h"
 
 
void exit_thread(void);
void * addr_check(const void *vaddr);  

void sys_close (struct intr_frame* f);
void sys_open (struct intr_frame* f);
void sys_seek (struct intr_frame* f);
void sys_tell (struct intr_frame* f);
void sys_filesize (struct intr_frame* f);
void sys_exec (struct intr_frame* f);
void sys_write (struct intr_frame* f);
struct thread_file *find_file_id (int file_id);
bool is_ptruser_valid (void* esp, uint8_t argc);
 
static void syscall_handler (struct intr_frame *f);
 

 
/*Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful,
   -1 if a segfault occurred. */
static int get_user (const uint8_t *uaddr) {
    int result;
    asm ("movl $1f, %0; movzbl %1, %0; 1:"
    : "=&a" (result) : "m" (*uaddr));
    return result;
}
 
/*
Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful,
   false if a segfault occurred. 
static bool put_user (uint8_t *udst, uint8_t byte) {
    int error_code;
    asm ("movl $1f, %0; movb %b2, %1; 1:"
    : "=&a" (error_code), "=m" (*udst) : "q" (byte));
    return error_code != -1;
}*/
 
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

}
 
// to handle the special occasion for thread
void exit_thread(void){
	thread_current()->exit_status = -1;
	thread_exit();
}
 
// to avoid user programs from reaching invalid addresses
void * addr_check(const void *vaddr){
	if(is_kernel_vaddr(vaddr)){
		exit_thread();
	}
	// looks up the physical addres of vaddr in the current threads
	// page directory and returns the Kernel Virtual Address
	// corresponding to that physical address
	// or a NULL pointer, if the vaddr is unmapped
	void *tmp = pagedir_get_page(thread_current()->pagedir, vaddr);
	if(tmp == NULL){
		exit_thread();
	}
	uint8_t *check = (uint8_t *) vaddr;
	for(uint8_t i=0; i<4; i++){
		if(get_user(check +i) == -1) exit_thread();
	}
	return tmp;
}
 
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * ptr = f->esp; 
  addr_check(ptr);
  int syscall_num = * ptr;
  switch (syscall_num)
    {
      case SYS_HALT:
        shutdown_power_off();
        break;
      case SYS_EXIT:  
      	addr_check(ptr+1);
      	thread_current()->exit_status = (*ptr) +1;
      	thread_exit();     
        break;
 
      case SYS_EXEC:      
        sys_exec(f);  
        break;
 
      case SYS_WAIT:
	addr_check(ptr+1);
	*ptr++;
	f->eax = process_wait(*ptr);      
        break;
 
      case SYS_CREATE:
        addr_check(ptr+5);
        //addr_check(*(ptr+4));
        // need to acquire the lock first, to be able to
        // do a operation on a file
        fsys_lock_acquire();
	// store the outcome in the accumulator
	f->eax = filesys_create(*ptr, *(ptr+1));
	fsys_lock_release();
        break;
 
      case SYS_REMOVE:
        addr_check(ptr+1);
        //addr_check(*(ptr+1));
        fsys_lock_acquire();
        f->eax = filesys_remove((const char *)*(ptr+1));
        fsys_lock_release();  
        break;
 
      case SYS_OPEN:    
        sys_open(f);   
        break;
 
      case SYS_FILESIZE:   
        sys_filesize(f);     
        break;
 
      case SYS_READ:
        *ptr++;
        int fd = *ptr;
        uint8_t * buffer = (uint8_t*)*(ptr+1);
        off_t size = *(ptr+2);
        if((!is_ptruser_valid(buffer, 1)) || (!is_ptruser_valid(buffer+size, 1))) exit_thread();
        // if the FD is = 0, we read from the keyboard using input_getc()
        if(fd == 0){
        	for(int i=0; i<size; i++){
        		buffer[i] = input_getc();
        	}
        	f->eax = size;
        }
        else{
        }
        // if the FD is not 0, we read the file OR can't read the file
        // due to a condition other than EOF. If that is the case eax -> -1
        break;
 
      case SYS_WRITE:        
        sys_write(f);
        break;
 
      case SYS_SEEK:      
        sys_seek(f);  
        break;
 
      case SYS_TELL:    
        sys_tell(f);    
        break;
 
      case SYS_CLOSE:   
        sys_close(f);     
        break;
      default:
        printf ("ERROR: syscall not found\n");
        thread_exit ();
        break;
    }
}

void sys_seek(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 5);
  *user_ptr++;
  
  struct thread_file *file_temp = find_file_id (*user_ptr);
  
  if (file_temp)
  {
    fsys_lock_acquire();
    file_seek (file_temp->file, *(user_ptr+1));
    fsys_lock_release();
  }
  
}

void sys_tell (struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 1);
  *user_ptr++;
  struct thread_file *thread_file_temp = find_file_id (*user_ptr);
  if (thread_file_temp)
  {
    fsys_lock_acquire();
    f->eax = file_tell (thread_file_temp->file);
    fsys_lock_release();
  }else{
    f->eax = -1;
  }
}

void sys_filesize (struct intr_frame* f){
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 1);
  (*user_ptr)++;
  struct thread_file * thread_file_temp = find_file_id (*user_ptr);
  if (thread_file_temp)
  {
    fsys_lock_acquire();
    f->eax = file_length (thread_file_temp->file);
    fsys_lock_release();
  }
  else
  {
    f->eax = -1;
  }
}

void sys_open (struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 1);
  addr_check (*(user_ptr + 1));
  *user_ptr++;
  fsys_lock_acquire();
  struct file * file_opened = file_open((const char *)*user_ptr);
  fsys_lock_release();
  struct thread * t = thread_current();
  if (file_opened)
  {
    struct thread_file *thread_file_temp = malloc(sizeof(struct thread_file));
    thread_file_temp->fd = t->file_fd++;
    thread_file_temp->file = file_opened;
    list_push_back (&t->files, &thread_file_temp->file_elem);
    f->eax = thread_file_temp->fd;
  }
  else
  {
    f->eax = -1;
  }
}

void sys_close (struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check(user_ptr + 1);
  *user_ptr++;
  struct thread_file * opened_file = find_file_id (*user_ptr);
  if (opened_file)
  {
    fsys_lock_acquire();
    file_close (opened_file->file);
    fsys_lock_release();
    list_remove (&opened_file->file_elem);
    free (opened_file);
  }
}

struct thread_file *
find_file_id (int file_id)
{
  struct list_elem *e;
  struct thread_file * thread_file_temp = NULL;
  struct list *files = &thread_current ()->files;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
    thread_file_temp = list_entry (e, struct thread_file, file_elem);
    if (file_id == thread_file_temp->fd)
      return thread_file_temp;
  }
  return false;
}

/* Do sytem exec */
void 
sys_exec (struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 1);
  addr_check (*(user_ptr + 1));
  *user_ptr++;
  f->eax = process_execute((char*)* user_ptr);
}

void 
sys_write (struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  addr_check (user_ptr + 7);
  addr_check(*(user_ptr + 6));
  *user_ptr++;
  int temp2 = *user_ptr;
  const char * buffer = (const char *)*(user_ptr+1);
  off_t size = *(user_ptr+2);
  if (temp2 == 1) {
    /* Use putbuf to do testing */
    putbuf(buffer,size);
    f->eax = size;
  }
  else
  {
    /* Write to Files */
    struct thread_file * thread_file_temp = find_file_id (*user_ptr);
    if (thread_file_temp)
    {
      fsys_lock_acquire();
      f->eax = file_write (thread_file_temp->file, buffer, size);
      fsys_lock_release();
    } 
    else
    {
      f->eax = 0;
    }
  }
}
// to check if the user pointer is valid 
bool 
is_ptruser_valid (void* esp, uint8_t argc){
  for (uint8_t i = 0; i < argc; ++i)
  {
    if((is_kernel_vaddr (esp)) || 
      (pagedir_get_page (thread_current()->pagedir, esp)==NULL)){
      return false;
    }
  }
  return true;
}

