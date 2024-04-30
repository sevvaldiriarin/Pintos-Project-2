### PINTOS PROJECT 2 - User Programs

---

#### User Programs

This assignment focuses on enabling user programs to interact with the Pintos operating system via system calls. The base code already supports loading and running user programs, but lacks I/O or interactivity. Our task is to implement system calls and handle user program exceptions to enable interaction with the OS.

---

#### Background

Up to this point, all code executed in Pintos has been part of the kernel. This changes with user programs running on top of the OS, requiring careful management of processes, memory, and scheduling. The project entails modifying various parts of Pintos, primarily within the `userprog` directory.

---

#### Relevant Source Files

- `process.c/process.h`: Load ELF binaries and start processes.
- `pagedir.c/pagedir.h`: Manage 80x86 hardware page tables.
- `syscall.c/syscall.h`: Implement system call handling.
- `exception.c/exception.h`: Handle exceptions during user process execution.
- `gdt.c/gdt.h`, `tss.c/tss.h`: Set up Global Descriptor Table (GDT) and Task-State Segment (TSS).

---

#### Using the File System

Although the focus is not on the file system, interaction with it is necessary. A simple file system is provided in the `filesys` directory. Limitations include no internal synchronization and fixed file size at creation.

---

#### How User Programs Work

User programs are loaded as ELF executables using the provided loader in `userprog/process.c`. Only system calls we will implement can be used; memory allocation and floating-point operations are not supported.

---

#### Virtual Memory Layout

Virtual memory consists of user and kernel regions. User memory ranges from 0 to `PHYS_BASE`, while kernel memory occupies the rest. Accessing kernel memory results in a page fault.

---

#### Suggested Order of Implementation

1. Implement argument passing.
2. Handle user memory access.
3. Set up system call infrastructure.
4. Implement the `exit` system call.
5. Implement the `write` system call for console output.
6. Refine `process_wait()`.

---

We'll follow this suggested order to progressively enable user programs to interact with the OS. 
