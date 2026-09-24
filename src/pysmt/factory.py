import maudeSE.maude

from .connector import *
from .converter import *
from maudeSE.maude import SmtManagerFactory

class Factory(SmtManagerFactory):
    def __init__(self):
        SmtManagerFactory.__init__(self)
        self._map = dict()
        self._converter = None

    def register(self, name, conv_cls, conn_cls):
        self._map[name] = (conv_cls, conn_cls)

    def check_solver(self, solver: str):
        # deprecate ...
        if solver not in self._map:
            raise Exception("unregistered solver {}".format(solver))

    def createConverter(self):
        solver = maudeSE.maude.cvar.smtSolver

        self.check_solver(solver)
 
        cv, _ = self._map[solver]
        conv = cv()
    
        if conv is None:
            raise Exception("fail to create converter")

        # Keep the Python director object.  SWIG passes createConnector a
        # base-class proxy, which hides solver-specific Python attributes
        # needed by backends such as cvc5.
        self._converter = conv
    
        # must be disown in order to take over the ownership
        return conv.__disown__()
    
    def createConnector(self, conv):
        solver = maudeSE.maude.cvar.smtSolver

        self.check_solver(solver)

        _, cn = self._map[solver]
        conn = cn(self._converter)
    
        if conn is None:
            raise Exception("fail to create connector")
    
        # must be disown in order to take over the ownership
        return conn.__disown__()
