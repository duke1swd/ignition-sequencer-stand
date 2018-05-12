/*
 * Define what pin we are tracing.  If nothing, then not tracing.
 */
#define	TRACE_PIN	5

/*
 * Tracing support routines.
 */
void trace_init();
void trace_trigger();
bool trace_done();
void trace_point(int data);
unsigned int trace_commit();
bool trace_to_serial(int i);
