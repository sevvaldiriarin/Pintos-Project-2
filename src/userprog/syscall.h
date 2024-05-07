#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void exit_thread(void);
void * addr_check(const void *vaddr);  
bool is_ptruser_valid (void* esp, uint8_t argc);

#endif /* userprog/syscall.h */
