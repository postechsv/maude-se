import cvc5

from cvc5 import Kind as cvcKind
from maudeSE.maude import *
from maudeSE.decorators import connector

@connector
class Cvc5Connector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter

        # set solver
        # Terms are owned by a cvc5 TermManager.  Reuse the converter's
        # solver so converted terms and asserted formulas have the same
        # owner (required by cvc5 1.4 and later).
        self._s = converter.solver
        self._s.setOption("produce-models", "true")
        self._logic = None
        if logic is not None:
            self.set_logic(logic)

        self._m = None
    
    def check_sat(self, consts):
        for const in consts:
            self._s.assertFormula(const)
        
        r = self._s.checkSat()

        if r.isSat():
            return sat
        elif r.isUnsat():
            return unsat
        else:
            return unknown
        
    def simplify(self, term):
        return self._s.simplify(term)
        
    def push(self):
        self._s.push()

    def pop(self):
        self._s.pop()

    def reset(self):
        self._s.resetAssertions()

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
            body = cur
        else:
            body = self._s.mkTerm(cvcKind.AND, acc, cur)

        return body

    def subsume(self, subst, prev, acc, cur):
        assert len(self._s.getAssertions()) == 0

        t_v = [source for source, _ in subst]
        t_l = [target for _, target in subst]

        # implication and its children
        l = self._s.mkTerm(cvcKind.AND, acc, cur)
        r = prev.substitute(t_v, t_l)
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
        return [(variable, self._s.getValue(variable)) for variable in self._get_vars()]
    
    def print_model(self):
        for v in self._m:
            print(f"  {v} ---> {self._m[v]}")

    def set_logic(self, logic):
        self._s.resetAssertions()
        if self._s.isLogicSet():
            current_logic = self._s.getLogic()
            if current_logic == logic:
                self._logic = logic
                return
            raise ValueError(
                f"cvc5 connector already uses logic {current_logic}; "
                f"cannot switch to {logic}"
            )
        self._s.setLogic(logic)
        self._logic = logic

    def get_converter(self):
        return self._c
