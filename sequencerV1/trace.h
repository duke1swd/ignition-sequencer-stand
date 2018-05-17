/*
 * Define what pin we are tracing.  If nothing, then not tracing.
 */

/*
 * Some interesting pins:
 * 	A1 = main pressure
 * 	A2 = ig pressure
 * 	A5 = power supply
 */
#define	TRACE_PIN	A5

/*
 * Tracing support routines.
 */
void trace_init();
void trace_trigger();
bool trace_done();
void trace_point(int data);
unsigned int trace_commit();
bool trace_to_serial(int i);
