import cvc5

from cvc5 import Kind as cvcKind
from maudeSE.maude import *
from maudeSE.util import id_gen
from maudeSE.maude import *

class Cvc5Connector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter
        self._g = id_gen()

        # set solver
        # Terms are owned by a cvc5 TermManager.  Reuse the converter's
        # solver so converted terms and asserted formulas have the same
        # owner (required by cvc5 1.4 and later).
        self._s = converter.solver
        self._s.setOption("produce-models", "true")
        self._logic = None
        if logic is not None:
            self._s.setLogic(logic)
            self._logic = logic

        self._m = None
    
    def check_sat(self, consts):
        for const in consts:
            self._s.assertFormula(get_data(const))
        
        r = self._s.checkSat()

        if r.isSat():
            return sat
        elif r.isUnsat():
            return unsat
        else:
            return unknown
        
    def simplify(self, term):
        return SmtTerm(self._s.simplify(get_data(term)))
        
    def push(self):
        self._s.push()

    def pop(self):
        self._s.pop()

    def reset(self):
        self._s.resetAssertions()

    def _make_model(self):
        _vars = self._get_vars()

        m = SmtModel()
        for v in _vars:
            m.set(v, self._s.getValue(v))
        return m
    
    def _get_vars(self):
        assertions = self._s.getAssertions()
        q, _vars, visit = list(assertions), set(), set(assertions)

        while len(q) > 0:
            a = q.pop()
            if a.getKind() == cvcKind.CONSTANT:
                _vars.add(a)
            else:
                for c in a:
                    if c not in visit:
                        q.append(c)
                        visit.add(c)
        return _vars

    def add_const(self, acc, cur):
        # initial case
        if acc is None:
            body = get_data(cur)
        else:
            acc_f, cur_t = get_data(acc), get_data(cur)
            body = self._s.mkTerm(cvcKind.AND, acc_f, cur_t)

        return SmtTerm(body)

    def subsume(self, subst, prev, acc, cur):
        arr = self._s.getAssertions()

        assert len(arr) == 0

        t_v, t_l = list(), list()
        sub = subst.keys()
        for p in sub:
            src = get_data(self._c.dag2term(p))
            trg = get_data(self._c.dag2term(subst.get(p)))

            t_v.append(src)
            t_l.append(trg)

        prev_c = get_data(prev)

        acc_c = get_data(acc)
        cur_c = get_data(cur)

        # implication and its children
        l = self._s.mkTerm(cvcKind.AND, acc_c, cur_c)
        r = prev_c.substitute(t_v, t_l)
        imply = self._s.mkTerm(cvcKind.IMPLIES, l, r)

        self._s.assertFormula(self._s.mkTerm(cvcKind.NOT, imply))

        r = self._s.checkSat()

        if r.isUnsat():
            return True
        elif r.isSat():
            return False
        else:
            raise Exception("failed to apply subsumption (give-up)")

    def merge(self, subst, prev_t, prev, cur_t, acc, cur):
        pass

    def get_model(self):
        return self._make_model()
    
    def print_model(self):
        for v in self._m:
            print(f"  {v} ---> {self._m[v]}")

    def set_logic(self, logic):
        self._s.resetAssertions()
        if self._logic is None:
            self._s.setLogic(logic)
            self._logic = logic
        elif self._logic != logic:
            raise ValueError(
                f"cvc5 connector already uses logic {self._logic}; "
                f"cannot switch to {logic}"
            )

    def get_converter(self):
        return self._c
