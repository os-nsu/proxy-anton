//header for master.c

#ifndef MASTER_H
#define MASTER_H

typedef void(*Hook)(void);

extern Hook start_hook;

extern Hook end_hook;

#endif  // MASTER_H