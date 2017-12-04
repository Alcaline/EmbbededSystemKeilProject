//

#ifndef __TEMP_H__
#define __TEMP_H__

extern void temp_init(void);
extern int16_t temp_read(void);
extern int16_t temp_read_vobj(void);
extern int16_t temp_read_temp(void);
extern long double TMP006_getTemp(void);

#endif //__TEMP_H__