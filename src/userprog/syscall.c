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
        break;

      case SYS_WAIT:       
        break;

      case SYS_CREATE:
        addr_check(ptr+5);
        addr_check(*(ptr+4));
        // need to acquire the lock first, to be able to
        // do a operation on a file
        fsys_lock_acquire();
	// store the outcome in the accumulator
	f->eax = filesys_create(*ptr, *(ptr+1));
	fsys_lock_release();
        break;

      case SYS_REMOVE:
        addr_check(ptr+1);
        addr_check(*(ptr+1));
        fsys_lock_acquire();
        f->eax = filesys_remove((const char *)*(ptr+1));
        fsys_lock_release();   
        break;

      case SYS_OPEN:       
        break;

      case SYS_FILESIZE:        
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
        break;

      case SYS_SEEK:        
        break;

      case SYS_TELL:        
        break;

      case SYS_CLOSE:        
        break;
    
      default:
        printf ("ERROR: syscall not found\n");
        thread_exit ();
        break;
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
