import select


_cls_select_poll = None
if hasattr(select, "poll"):
    _cls_select_poll = type(select.poll())


class poll_wrapper:
    def __init__(self, wrapped):
        self.wrapped = wrapped

    def register(self, *args, **kwargs):
        self.wrapped.register(*args, **kwargs)

    def modify(self, *args, **kwargs):
        self.wrapped.modify(*args, **kwargs)

    def unregister(self, *args, **kwargs):
        self.wrapped.unregister(*args, **kwargs)

    def poll(self, timeout=None):
        if timeout is None:
            return self.wrapped.poll()
        else:
            return self.wrapped.poll(round(timeout * 1000))


# As per libusb1 documentation, work around the mismatch of select.poll
# timeout argument (in milliseconds) and the one expected by USBPoller
# (in seconds).
def wrap_poller_for_libusb(poller):
    if _cls_select_poll is not None and isinstance(poller, _cls_select_poll):
        return poll_wrapper(poller)
    else:
        return poller
