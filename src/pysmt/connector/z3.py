import z3

from maudeSE.maude import *
from maudeSE.util import id_gen
from maudeSE.maude import *

class Z3Connector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter
        self._g = id_gen()

        _logic = "QF_LRA" if logic is None else logic

        # set solver
        self._s = z3.SolverFor(_logic)
    
    def check_sat(self, consts):
        for const in consts:
            self._s.add(get_data(const))

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
            body = get_data(cur)
        else:
            body = z3.And(get_data(acc), get_data(cur))

        return SmtTerm(z3.simplify(body))

    def simplify(self, term):
        return SmtTerm(z3.simplify(get_data(term)))

    def subsume(self, subst, prev, acc, cur):
        t_l = list()
        sub = subst.keys()
        for p in sub:
            src = get_data(self._c.dag2term(p))
            trg = get_data(self._c.dag2term(subst.get(p)))

            t_l.append((src, trg))

        prev_c = get_data(prev)

        acc_c = get_data(acc)
        cur_c = get_data(cur)
    
        self._s.add(z3.Not(z3.Implies(z3.And(acc_c, cur_c), z3.substitute(prev_c, *t_l))))

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
        
        m = SmtModel()
        for d in raw_m.decls():
            m.set(d, raw_m[d])
        return m

    def print_model(self):
        print(self._m)

    def set_logic(self, logic):
        # recreate solver
        self._s = z3.SolverFor(logic)
    
    def get_converter(self):
        return self._c