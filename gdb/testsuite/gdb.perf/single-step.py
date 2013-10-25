
from perftest import perftest
from perftest import measure

class SingleStep (perftest.TestCaseWithBasicMeasurements):
    def __init__(self, step):
        super (SingleStep, self).__init__ ("single-step")
        self.step = step

    def warm_up(self):
        for i in range(0, self.step):
            gdb.execute("stepi", False, True)

    def _run(self, r):
        for j in range(0, r):
            gdb.execute("stepi", False, True)

    def execute_test(self):
        for i in range(1, 5):
            func = lambda: self._run(i * self.step)
            self.measure.measure(func, i * self.step)
            # gdb.execute ("maintenance info statistics")

