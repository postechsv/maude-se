from .connector import *
from .converter import *
from maudeSE.maude import PyWrapperFactory

class Factory(PyWrapperFactory):
    def __init__(self):
        PyWrapperFactory.__init__(self)
        self._map = {
            "z3"       : (Z3Converter, Z3Connector),
            "yices"    : (YicesConverter, YicesConnector),
            "cvc5"     : (Cvc5Converter, Cvc5Connector),
        }

        # update this...
        self.conv = None
        self.conn = None

    def create(self, solver: str):
        # deprecate ...
        if solver not in self._map:
            raise Exception("unsupported solver {}".format(solver))

        cv_f, cn_f = self._map[solver]
        self.conv = cv_f()
        self.conn = cn_f(self.conv)

        # return self.cur_conn, self.conv
    
    def createConnector(self):
        if self.conn is None:
            raise Exception("fail to create converter")
    
        return self.conn

    def createConverter(self):
        if self.conv is None:
            raise Exception("fail to create connector")

        return self.conv