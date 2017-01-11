
/*
 * machinarium.
 *
 * Cooperative multitasking engine.
*/

#include <machinarium_private.h>
#include <machinarium.h>

static void
mm_connect_timeout_cb(uv_timer_t *handle)
{
	mmio *io = handle->data;
	io->connect_timeout = 1;
	/* cancel connection request,
	 * connect callback will be called anyway */
	mm_io_close_handle(io, (uv_handle_t*)&io->handle);
}

static void
mm_connect_cb(uv_connect_t *handle, int status)
{
	mmio *io = handle->data;
	if (mm_fiber_is_cancel(io->connect_fiber))
		goto wakeup;
	if (io->connect_timeout)
		goto wakeup;
	mm_io_timer_stop(io, &io->connect_timer);
wakeup:
	io->connect_status = status;
	mm_wakeup(io->f, io->connect_fiber);
}

static void
mm_connect_cancel_cb(mmfiber *fiber, void *arg)
{
	(void)fiber;
	mmio *io = arg;
	io->write_timeout = 0;
	mm_io_timer_stop(io, &io->connect_timer);
	mm_io_close_handle(io, (uv_handle_t*)&io->handle);
}

MM_API int
mm_is_connected(mm_io_t iop)
{
	mmio *io = iop;
	return io->connected;
}

MM_API int
mm_connect(mm_io_t iop, struct sockaddr *sa, uint64_t time_ms)
{
	mmio *io = iop;
	mmfiber *current = mm_current(io->f);
	if (mm_fiber_is_cancel(current))
		return -ECANCELED;
	if (io->connect_fiber)
		return -1;
	io->connect_status  = 0;
	io->connect_timeout = 0;
	io->connect_fiber   = NULL;

	/* assign fiber */
	io->connect_fiber = current;

	/* start timer and connection */
	mm_io_timer_start(io, &io->connect_timer, mm_connect_timeout_cb,
	                  time_ms);
	int rc;
	rc = uv_tcp_connect(&io->connect, &io->handle,
	                    sa, mm_connect_cb);
	if (rc < 0) {
		mm_io_timer_stop(io, &io->connect_timer);
		io->connect_fiber = NULL;
		return rc;
	}

	/* register cancellation procedure */
	mm_fiber_op_begin(io->connect_fiber, mm_connect_cancel_cb, io);
	/* yield fiber */
	mm_scheduler_yield(&io->f->scheduler);
	mm_fiber_op_end(io->connect_fiber);

	/* result from timer or connect callback */
	rc = io->connect_status;
	if (rc == 0) {
		assert(! io->connect_timeout);
		io->connected = 1;
	}
	io->connect_fiber = NULL;
	return rc;
}

MM_API int
mm_connect_is_timeout(mm_io_t iop)
{
	mmio *io = iop;
	return io->connect_timeout;
}
