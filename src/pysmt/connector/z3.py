import z3

from maudeSE.maude import *
from maudeSE.decorators import connector

@connector
class Z3Connector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter

        _logic = "QF_LRA" if logic is None else logic

        # set solver
        self._s = z3.SolverFor(_logic)
    
    def check_sat(self, consts):
        for const in consts:
            self._s.add(const)

        r = self._s.check()

        if r == z3.sat:
            return sat
        elif r == z3.unsat:
            return unsat
        else:
            return unknown

    def push(self):
        self._s.push()

    def pop(self):
        self._s.pop()

    def reset(self):
        self._s.reset()

    def add_const(self, acc, cur):
        # initial case
        if acc is None:
            body = cur
        else:
            body = z3.And(acc, cur)

        return z3.simplify(body)

    def simplify(self, term):
        return z3.simplify(term)

    def subsume(self, subst, prev, acc, cur):
        self._s.add(z3.Not(z3.Implies(z3.And(acc, cur), z3.substitute(prev, *subst))))

        r = self._s.check()

        if r == z3.unsat:
            return True
        elif r == z3.sat:
            return False
        else:
            raise Exception("failed to apply subsumption (give-up)")

    def merge(self, subst, prev_t, prev, cur_t, acc, cur):
        pass

    def get_model(self):
        raw_m = self._s.model()
        
        return [(d, raw_m[d]) for d in raw_m.decls()]

    def print_model(self):
        print(self._m)

    def set_logic(self, logic):
        # recreate solver
        self._s = z3.SolverFor(logic)
    
    def get_converter(self):
        return self._c
