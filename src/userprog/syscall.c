#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <devices/shutdown.h>
#include <threads/vaddr.h>


void exit_thread(void);
void * addr_check(const void *vaddr);  



static void syscall_handler (struct intr_frame *);

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
		if(get_user(vaddr) == -1) exit_thread;
	}
	
	return tmp;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * ptr = f->esp; 
  addr_check(ptr);
  
  // TO DO: address check
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
        break;

      case SYS_REMOVE:       
        break;

      case SYS_OPEN:       
        break;

      case SYS_FILESIZE:        
        break;

      case SYS_READ:        
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
