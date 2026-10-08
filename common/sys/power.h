
typedef enum { scale1 = 3, scale2 = 2, scale3 = 1 } VOS;

void set_voltage_scale(VOS scale);

VOS get_voltage_scale(void);

void start_Overdrive(void);

void stop_Overdrive(void);
