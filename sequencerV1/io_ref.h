/*
 *  Pointers to the inputs and outputs
 */

extern struct input * i_joystick;
extern struct input * i_ig_pressure;
extern struct input * i_push_1;
extern struct input * i_push_2;
extern struct input * i_safe_ig;
extern struct input * i_safe_main;
extern struct input * i_cmd_1;
extern struct input * i_cmd_2;

extern struct output * o_ipaIgValve;
extern struct output * o_n2oIgValve;
extern struct output * o_greenStatus;
extern struct output * o_amberStatus;
extern struct output * o_redStatus;
extern struct output * o_powerStatus;
extern struct output * o_spark;

#define	IPAServoPin	5
#define	N2OServoPin	6
