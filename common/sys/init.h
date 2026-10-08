#include <stdint.h>

void set_FLASH_latency(int32_t latency);
void fpu_init(void);
void enable_io_compensation(void);
void clock_init(void);
void software_init_hook(void);
