import cvc5
import time

from cvc5 import Kind as cvcKind
from maudeSE.maude import *
from maudeSE.util import id_gen
from maudeSE.maude import *

class Cvc5Connector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter
        self._g = id_gen()

        _logic = "ALL" if logic is None else logic

        # time
        self._tt = 0.0
        self._dt = 0.0
        self._st = 0.0
        self._mt = 0.0

        # set solver
        self._s = cvc5.Solver()
        self._s.setOption("produce-models", "true")
        # self._s.setOption("produce-unsat-cores", "true")
        self._s.setLogic(_logic)

        self._m = None
    
    def check_sat(self, consts):
        for const in consts:
            c, _, _ = const.data()
            self._s.assertFormula(c)
        
        r = self._s.checkSat()

        if r.isSat():
            return True
        elif r.isUnsat():
            return False
        else:
            raise Exception("failed to handle check sat (solver give-up)")
        
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
            k, v = [v, None, None], [self._s.getValue(v), None, None]
            m.set(k, v)
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
            (cur_t, _, cur_v) = cur.data()
            body = cur_t

        else:
            (acc_f, _, acc_v), (cur_t, _, cur_v) = acc.data(), cur.data()
            body = self._s.mkTerm(cvcKind.AND, acc_f, cur_t)

        return SmtTerm([body, None, None])

    def subsume(self, subst, prev, acc, cur):
        s = time.time()

        d_s = time.time()
        t_v, t_l = list(), list()
        sub = subst.keys()
        for p in sub:
            src, _, _ = self._c.dag2term(p).data()
            trg, _, _ = self._c.dag2term(subst.get(p)).data()

            t_v.append(src)
            t_l.append(trg)

        d_e = time.time()

        self._dt += d_e - d_s

        prev_c, _, _ = prev.data()

        acc_c, _, _ = acc.data()
        cur_c, _, _ = cur.data()
    
        so_s = time.time()
        self._s.push()

        # implication and its children
        l = self._s.mkTerm(cvcKind.AND, acc_c, cur_c)
        r = prev_c.substitute(t_v, t_l)
        imply = self._s.mkTerm(cvcKind.IMPLIES, l, r)

        self._s.assertFormula(self._s.mkTerm(cvcKind.NOT, imply))

        r = self._s.checkSat()

        self._s.pop()
        so_e = time.time()
        self._st += so_e - so_s

        if r.isUnsat():
            e = time.time()
            self._tt += e - s
            return True
        elif r.isSat():
            e = time.time()
            self._tt += e - s
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
        # set solver
        self._s = cvc5.Solver()
        self._s.setOption("produce-models", "true")
        self._s.setLogic(logic)

    def get_converter(self):
        return self._c
