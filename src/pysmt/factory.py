import maudeSE.maude

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

    def check_solver(self, solver: str):
        # deprecate ...
        if solver not in self._map:
            raise Exception("unsupported solver {}".format(solver))

    def createConverter(self):
        solver = maudeSE.maude.cvar.smtSolver

        self.check_solver(solver)
 
        cv, _ = self._map[solver]
        conv = cv()
    
        if conv is None:
            raise Exception("fail to create converter")
    
        # must be disown in order to take over the ownership
        return conv.__disown__()
    
    def createConnector(self, conv):
        solver = maudeSE.maude.cvar.smtSolver

        self.check_solver(solver)

        _, cn = self._map[solver]
        conn = cn(conv)
    
        if conn is None:
            raise Exception("fail to create connector")
    
        # must be disown in order to take over the ownership
        return conn.__disown__()