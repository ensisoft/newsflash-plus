
import traceback

def format_error(etype, value, tb=None):
    return ''.join(traceback.format_exception(etype, value, tb, None))