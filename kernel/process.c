#include "libc.h"
#include "kernel.h"
#include "kheap.h"
#include "shell.h"
#include "isr.h"
#include "process.h"
#include "virtualmem.h"
#include "display.h"
#include "gui_window.h"
#include "text_window.h"
#include "gui_mouse.h"
#include "descriptor_tables.h"

uint next_pid = 0;
extern PageDirectory *current_page_directory;

// For now we are statically allocating the process structures
char pad[7];
Process processes[3];
//process *process_focus;		           // The process which has user focus
volatile Process *current_process;  // The process which gets CPU cycles

extern uint initial_esp;

int new_process = 0;				// Indicates if it's the first "context switch"

extern uint read_eip();
extern uint get_eip();
extern void shell();
extern void edit();

uint nb_processes = 0;

void switch_to_user_mode()
{
   // Set up a stack structure for switching to user mode.
   asm volatile("  \
     cli; \
     mov $0x23, %ax; \
     mov %ax, %ds; \
     mov %ax, %es; \
     mov %ax, %fs; \
     mov %ax, %gs; \
                   \
     mov %esp, %eax; \
     pushl $0x23; \
     pushl %eax; \
     pushf; \
     pushl $0x1B; \
     push $1f; \
     iret; \
   1: \
     ");
}

// For now we're hard-coding 2 processes
void init_processes() {
	processes[0].pid = 0;
	processes[1].pid = 1;
  processes[0].function = shell;
  processes[1].function = shell;
  
  if (display_mode() == VGA_MODE) {
    init_window(&gui_win1, &processes[0]);
    init_window(&gui_win2, &processes[1]);
  } else {
    init_window(&text_win1, &processes[0]);
    init_window(&text_win2, &processes[1]);
  }
	processes[0].next = &processes[0];
	processes[1].next = &processes[0];

	current_process = &processes[0];
}

Process *get_new_process(PageDirectory *dir) {
    Process *ps = &processes[nb_processes];
    if (current_process) current_process->next = ps;
    ps->next = &processes[0];
    ps->stack = (unsigned char *)kmalloc_pages(PROCESS_STACK_SIZE / 0x1000, "Process stack");
//    memset(ps->stack, 0, PROCESS_STACK_SIZE);
    ps->pid = nb_processes++;
    ps->esp = 0;
    ps->ebp = 0;
    ps->eip = 0;
    ps->page_dir = dir;
    ps->flags = 0;
    ps->buffer = 0;

    return ps;
}

void init_tasking()
{
    // Disable interrupts
    asm volatile("cli");

    // Initialise the first process
    current_process = get_new_process(current_page_directory);

    // Relocate the stack so we know where it is.
//    move_stack((char*)&current_process->eax, 0x2000);
    move_stack((unsigned char*)current_process->stack + PROCESS_STACK_SIZE, 0x2000);
//    set_kernel_stack((char*)current_process->esp);
    set_kernel_stack((void*)&current_process->eip);

    // Reenable interrupts.
    asm volatile("sti");
}

void move_stack(void *new_stack_start, uint size)
{
  uint i;
  // Allocate some space for the new stack.
  for( i = (uint)new_stack_start;
       i >= ((uint)new_stack_start-size);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
//    map_page(i, i, 0, 1);
//    alloc_frame( get_page(i, 1, current_page_directory), 0 /* User mode */, 1 /* Is writable */ );
  }
  
  // Flush the TLB by reading and writing the page directory address again.
  uint pd_addr;
  asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
  asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  uint old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  uint old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  uint offset = (uint)new_stack_start - initial_esp;

  // New ESP and EBP.
  uint new_stack_pointer = old_stack_pointer + offset;
  uint new_base_pointer  = old_base_pointer  + offset;

/*  debug_i("Move - Old stack start: ", initial_esp);
  debug_i("Old ESP: ", old_stack_pointer);
  debug_i("Old EBP: ", old_base_pointer);

  debug_i("New stack start: ", (uint)new_stack_start);
  debug_i("New ESP: ", new_stack_pointer);
  debug_i("New EBP: ", new_base_pointer);
*/
//  printf("Stack: %x ->%x\n", (uint)new_stack_start, ((uint)new_stack_start-PROCESS_STACK_SIZE));
  // Copy the stack.
  memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, initial_esp-old_stack_pointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.  
  for(i = (uint)new_stack_start; i > (uint)new_stack_start-size; i -= 4)
  {
    uint tmp = * (uint*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      uint *tmp2 = (uint*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  current_process->ebp = new_base_pointer;
  current_process->esp = new_stack_pointer;

  asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

void copy_stack(void *new_stack_start, void *old_stack_start)
{
  uint i;
  // Allocate some space for the new stack.
  for( i = (uint)new_stack_start;
       i >= ((uint)new_stack_start-PROCESS_STACK_SIZE);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
//    map_page(i, i, 0, 1);
//    alloc_frame( get_page(i, 1, current_page_directory), 0 /* User mode */, 1 /* Is writable */ );
  }
  
  // Flush the TLB by reading and writing the page directory address again.
  uint pd_addr;
  asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
  asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  uint old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  uint old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  uint offset = (uint)new_stack_start - (uint)old_stack_start;

  // New ESP and EBP.
  uint new_stack_pointer = old_stack_pointer + offset;
  uint new_base_pointer  = old_base_pointer  + offset;

/*  debug_i("Copy - Old stack start: ", (uint) old_stack_start);
  debug_i("Old ESP: ", old_stack_pointer);
  debug_i("Old EBP: ", old_base_pointer);

  debug_i("New stack start: ", (uint)new_stack_start);
  debug_i("New ESP: ", new_stack_pointer);
  debug_i("New EBP: ", new_base_pointer);
*/
//  printf("Stack: %x ->%x\n", (uint)new_stack_start, ((uint)new_stack_start-PROCESS_STACK_SIZE));

  // Copy the stack.
  memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, PROCESS_STACK_SIZE);

  // Backtrace through the original stack, copying new values into
  // the new stack.  
  for(i = (uint)new_stack_start; i > (uint)new_stack_start-PROCESS_STACK_SIZE; i -= 4)
  {
    uint tmp = * (uint*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < (uint)old_stack_start))
    {
      tmp = tmp + offset;
      uint *tmp2 = (uint*)i;
      *tmp2 = tmp;
//      debug_i("Relocated stack pointer to ", tmp);
    }
  }

}

// We're using static variable to avoid messing up with the
// stack at a time where we are relocating the stack
uint esp_global;
uint ebp_global;
uint eip_global;

void switch_process()
{
    // Disable interrupts
    asm volatile("cli");

    // If there is no current process, do nothing
    if (!current_process)
        return;

    // Read the ESP, EBP and EIP registers
    // Those will be saved in the current process structure
    uint esp, ebp, eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    eip = get_eip();

    // Because we have just saved EIP, future context switches may start right here
    // So we may be in two cases:
    // - We are the old current process, so we need to continue
    // - We are the new current process, so we need to return
    // We check the process flag PROCESS_EXIT_NOW to know whether we should exit or not
    if (current_process->flags & PROCESS_EXIT_NOW) {
        current_process->flags &= ~PROCESS_EXIT_NOW;
        asm volatile("sti");
        return;
    }

    // We haven't switched context yet
    // So we save the registers in the process structure
    current_process->eip = eip;
    current_process->esp = esp;
    current_process->ebp = ebp;
/*
    debug_i("Before switch to ESP: ", esp);
    debug_i("EBP: ", ebp);
    debug_i("EIP: ", eip);
  */

    // Get the next process. The processes are linked in a circular linked list
    Process *new_process = current_process->next;
    // We skip processes that are polling (e.g. waiting for the keyboard)
    // as there is no need to spend cycles on them
    while (new_process->flags & PROCESS_POLLING &&
           new_process != current_process) new_process = new_process->next;
    // if all processes are polling, stay on the current process
    if (new_process == current_process) current_process = current_process->next;
    else current_process = new_process;

    // Retrieves the values for the new current process
    eip_global = current_process->eip;
    esp_global = current_process->esp;
    ebp_global = current_process->ebp;
    current_page_directory = current_process->page_dir;
    set_kernel_stack((void*)&current_process->eip);

/*
    debug_i("Switch to ESP: ", esp_global);
    debug_i("EBP: ", ebp_global);
    debug_i("EIP: ", eip_global);
    */
//    debug_i("Page dir: ", (uint)current_page_directory);

    // We set the PROCESS_EXIT_NOW for the new current process to exit the fork() or
    // switch_process() functions as soon as it gets there
    current_process->flags |= PROCESS_EXIT_NOW;

    // - Sets the ESP and EBP pointers to the saved values for the new current process
    // - Sets the CR3 pointer to point to the new page directory
    // - Stores the new EIP pointer in the EAX register
    // - Reenable interrupts
    // - Jump to EAX (=EIP)
    asm volatile("         \
      mov %0, %%esp;       \
      mov %1, %%ebp;       \
      mov %2, %%cr3;       \
      mov %3, %%eax;       \
      sti;                 \
      jmp *%%eax           "
                 : : "r"(esp_global), "r"(ebp_global), "r"(current_page_directory), "r"(eip_global));
}

// Spawn a new process
int fork()
{
    // Disable interrupts
    asm volatile("cli");

    // Clone the address space.
    PageDirectory *directory = clone_page_directory(current_page_directory);

    // Create a new child process.
    Process *new_process = get_new_process(directory);

    // Copy the stack of the parent process to the child process
//    copy_stack((void*)&new_process->eax, (void*)&current_process->eax);
    copy_stack((void*)(new_process->stack + PROCESS_STACK_SIZE), (void*)(current_process->stack + PROCESS_STACK_SIZE));

    // This will be the entry point for the new process.
    uint eip = get_eip();

    // At this point we have two possibilities:
    // - We are still the parent process, in which case we need to continue
    // - We are the child process, returning after a context switch, in which case we need to exit
    // We use the process PROCESS_EXIT_NOW flag to know what scenario we are in
    if (current_process->flags & PROCESS_EXIT_NOW) {
        current_process->flags &= ~PROCESS_EXIT_NOW;
        asm volatile("sti");
        return -1;
    }

    // It is the parent process, continue with the forking process

    // Get the ESP and EBP registers from the parent process
    uint esp; asm volatile("mov %%esp, %0" : "=r"(esp));
    uint ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));

    // Because we have a different stack, we need to use the relative values for ESP and EBP
    uint stack_offset = (uint)new_process->stack - (uint)current_process->stack;
    new_process->esp = esp + stack_offset;
    new_process->ebp = ebp + stack_offset;
    new_process->eip = eip;
    asm volatile("sti");

/*    debug_i("Old process stack trace start: ", (uint)&current_process->eax);
    debug_i("Old process ESP: ", current_process->esp);
    debug_i("Old process EBP: ", current_process->ebp);
    debug_i("Old process stack trace end: ", (uint)&current_process->stack);

    debug_i("Current ESP: ", esp);
    debug_i("Current EBP: ", ebp);

    debug_i("New process stack trace start: ", (uint)&new_process->eax);
    debug_i("New process ESP: ", new_process->esp);
    debug_i("New process EBP: ", new_process->ebp);
    debug_i("New process EIP: ", new_process->eip);
    debug_i("New process stack trace end: ", (uint)&new_process->stack);
*/
    return new_process->pid;
}

int getpid()
{
    return current_process->pid;
}

void error(const char *msg) {
    char *error = (char*)&current_process->error;
    strcpy(error, msg);
}

void error_reset() {
    current_process->error[0] = 0;
}

const char *error_get() {
    return (const char*)&current_process->error;
}
