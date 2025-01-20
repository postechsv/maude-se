from .connector import *
from .converter import *
from maudeSE.maude import PySmtManagerFactory

class Factory(PySmtManagerFactory):
    def __init__(self):
        PySmtManagerFactory.__init__(self)
        self._map = {
            "z3"       : (Z3Converter, Z3Connector),
            "yices"    : (YicesConverter, YicesConnector),
            "cvc5"     : (Cvc5Converter, Cvc5Connector),
        }

        self.solver = None

    def set_solver(self, solver: str):
        # deprecate ...
        if solver not in self._map:
            raise Exception("unsupported solver {}".format(solver))
        self.solver = solver

    def createConverter(self):
        if self.solver is None:
            raise Exception("fail to create connector")

        cv, _ = self._map[self.solver]
        conv = cv()
    
        if conv is None:
            raise Exception("fail to create connector")
    
        # must be disown in order to take over the ownership
        return conv.__disown__()
    
    def createConnector(self, conv):
        if self.solver is None:
            raise Exception("fail to create connector")

        _, cn = self._map[self.solver]
        conn = cn(conv)
    
        if conn is None:
            raise Exception("fail to create connector")
    
        # must be disown in order to take over the ownership
        return conn.__disown__()