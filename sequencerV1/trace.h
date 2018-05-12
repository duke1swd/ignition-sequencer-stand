/*
 * Define what pin we are tracing.  If nothing, then not tracing.
 */

/*
 * Some interesting pins:
 * 	1 = main pressure
 * 	2 = ig pressure
 */
#define	TRACE_PIN	1

/*
 * Tracing support routines.
 */
void trace_init();
void trace_trigger();
bool trace_done();
void trace_point(int data);
unsigned int trace_commit();
bool trace_to_serial(int i);
