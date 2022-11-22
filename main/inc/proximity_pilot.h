#ifndef __PROXIMITY_PILOT_H__
#define __PROXIMITY_PILOT_H__

// PP Status
#define PP_DISCONNECTED (0)
#define PP_PRESSED (1)
#define PP_CONNECTED (2)

void init_proximity_pilot();

int32_t get_pp_status();


#endif