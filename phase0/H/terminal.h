/* Terminal functions */
int atoi(const char* s);
int term_putchar(char c,int i);
int term_puts(char *str, unsigned int i);

int term_getchar(int i);

/* Disk functions */
int store(char* data0, unsigned int head, unsigned int sect);
char* read(unsigned int head, unsigned int sect);
int seek (unsigned int cyl);
