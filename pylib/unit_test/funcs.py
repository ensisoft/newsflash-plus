

class UnitTestError(Exception):
    def __init__(self, msg):
        self.msg = msg

def test():
    print 'test'

def test_arg_int(arg):
    print 'test_arg_int'
    if arg != 1234:
        raise UnitTestError("int argument check failed")

def test_arg_str(arg):
    print 'test_arg_str'
    if arg != 'ARDVARK':
        raise UnitTestError('str argument check failed')


def test_arg_str_int(one, two):
    print 'test_arg_str_int'
    if one != 1234:
        raise UnitTestError("int argument check failed")
    if two != 'ARDVARK':
        raise UnitTestError("str argument check failed")

def test_thread_identity(identity):
    print 'thread: ' + str(identity)


def raise_exception():
    raise Exception("this function always returns with an exception")

def broken_function():
    asdgpasdg()

nonfunction=123


