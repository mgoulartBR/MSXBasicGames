#ifndef KEEN_CART_H
#define KEEN_CART_H
#include "world.h"
#define CART_FRAME 84
typedef struct {u16 x,y; i8 dir,delta;} Cart;
extern Cart carts[3];
extern u8 cart_riding;
void CartReset(void);
void CartStep(void);
void CartStepFast(void);
void CartContact(u16 oldbottom);
void CartDraw(u8 page,DrawObject* ob) __banked;
#endif
