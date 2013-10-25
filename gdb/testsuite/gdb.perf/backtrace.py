from perftest import perftest
from perftest import measure

class BackTrace (perftest.TestCaseWithBasicMeasurements):
    def __init__(self, depth):
        super (BackTrace, self).__init__ ("backtrace")
        self.depth = depth

    def warm_up(self):
        # Warm up.
        gdb.execute ("bt", False, True)
        gdb.execute ("bt", False, True)

    def _do_test(self):
        """Do backtrace multiple times."""
        for j in range(1, 15):
            # Clear dcache.
            gdb.execute ("set stack-cache off")
            gdb.execute ("set stack-cache on")
            do_test_command = "bt %d" % self.depth
            gdb.execute (do_test_command, False, True)

    def execute_test(self):

        line_size = 2
        for i in range(1, 12):
            
            line_size_command = "set dcache line-size %d" % (line_size)
            size_command = "set dcache size %d" % (4096 * 64 / line_size)
            gdb.execute (line_size_command)
            gdb.execute (size_command)

            func = lambda: self._do_test()

            self.measure.measure(func, line_size)
            # gdb.execute ("maintenance info statistics")

            line_size *= 2
