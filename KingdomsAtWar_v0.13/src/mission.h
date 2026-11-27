/* Mission parameters in RAM (a copy of one 16 byte record of tools/campaign.py's table). */
#ifndef KAW_MISSION_H
#define KAW_MISSION_H
typedef struct {u8 wood,gold;u16 first_wave;u8 wave_gap,extras,ai_on,obj;u16 obj_param;u8 limit,text,map,pad[3];} Mission;
#define OBJ_DESTROY 0
#define OBJ_SURVIVE 1
#define OBJ_KILLS 2
#define OBJ_GOLD 3
#define OBJ_BUILD 4
#endif
