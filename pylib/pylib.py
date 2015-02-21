
import traceback

# DO NOT MODIFY THIS FUNCTION
def format_error(etype, value, tb=None):
    return ''.join(traceback.format_exception(etype, value, tb, None))