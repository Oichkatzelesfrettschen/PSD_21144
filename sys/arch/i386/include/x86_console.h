#ifndef _X86_CONSOLE_H_
#define _X86_CONSOLE_H_

void x86_cons_init(void);
void x86_cons_early_putc(int c); // No locking, for very early use
void x86_cons_putc(int c);      // Potentially locking
int  x86_cons_getc(void);       // Potentially locking

#endif /* _X86_CONSOLE_H_ */
