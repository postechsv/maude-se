import z3
import time

from maudeSE.maude import *
from maudeSE.util import id_gen
from maudeSE.maude import *

class Z3Connector(PyConnector):
    def __init__(self, converter: PyConverter, logic=None):
        super().__init__()
        self._c = converter
        self._g = id_gen()

        _logic = "QF_LRA" if logic is None else logic

        # time
        self._tt = 0.0
        self._dt = 0.0
        self._st = 0.0
        self._mt = 0.0

        # set solver
        self._s = z3.SolverFor(_logic)
        self._ss = z3.SolverFor(_logic)
    
    def check_sat(self, consts):
        for const in consts:
            c, _, _ = const.data()
            self._s.add(c)

        r = self._s.check()

        if r == z3.sat:
            return True
        elif r == z3.unsat:
            return False
        else:
            raise Exception("failed to handle check sat (solver give-up)")

    def push(self):
        self._s.push()

    def pop(self):
        self._s.pop()

    def reset(self):
        self._s.reset()

    def add_const(self, acc, cur):
        # initial case
        if acc is None:
            cur_t, _, cur_v = cur.data()
            body = cur_t

        else:
            acc_f, _, acc_v = acc.data()
            cur_t, _, cur_v = cur.data()
            body = z3.And(acc_f, cur_t)

        return PySmtTerm([z3.simplify(body), None, None])

    def subsume(self, subst, prev, acc, cur):
        s = time.time()
        
        d_s = time.time()
        t_l = list()
        sub = subst.keys()
        for p in sub:
            src, _, _ = self._c.dag2term(p).data()
            trg, _, _ = self._c.dag2term(subst.get(p)).data()

            t_l.append((src, trg))
        d_e = time.time()

        self._dt += d_e - d_s

        prev_c, _, _ = prev.data()

        acc_c, _, _ = acc.data()
        cur_c, _, _ = cur.data()
    
        so_s = time.time()
        self._ss.push()
        self._ss.add(z3.Not(z3.Implies(z3.And(acc_c, cur_c), z3.substitute(prev_c, *t_l))))

        r = self._ss.check()
        self._ss.pop()
        so_e = time.time()
        self._st += so_e - so_s

        if r == z3.unsat:
            e = time.time()
            self._tt += e - s
            return True
        elif r == z3.sat:
            e = time.time()
            self._tt += e - s
            return False
        else:
            raise Exception("failed to apply subsumption (give-up)")

    def merge(self, subst, prev_t, prev, cur_t, acc, cur):
        mo = prev_t.symbol().getModule()
        
        rename_dict = dict()
        t_l, t_r = list(), list()
        eq_l, eq_r = list(), list()
        for p in subst:
            if p == subst[p]:
                continue

            idx = next(self._g)

            src, _, _ = self._c.dag2term(p)
            trg, _, _ = self._c.dag2term(subst[p])

            n_trg = z3.Const(f"#mergeVar{idx}", trg.sort())
            newVar = mo.parseTerm(f"#mergeVar{idx}:{p.getSort()}")

            eq_l.append(trg == n_trg)
            eq_r.append(src == n_trg)

            t_l.append((trg, n_trg))
            t_r.append((src, n_trg))
            
            rename_dict[subst[p]] = newVar
            
        prev_c, _, _ = prev

        (acc_f, _, _), (cur_c, _, _) = acc, cur

        conj = z3.And(acc_f, cur_c)

        c = z3.Or(z3.And(z3.substitute(prev_c, *t_l), *eq_l), z3.And(z3.substitute(conj, *t_r), *eq_r))

        n_subst = Substitution(rename_dict)
        new_prev = n_subst.instantiate(prev_t)

        return tuple([new_prev, tuple([z3.simplify(c), None, None])])
    
    # def mk_subst(self, vars, vals):
    #     subst = dict()
    #     for v, val in zip(vars, vals):
    #         subst[v] = val
    #     return PyTermSubst(subst)

    def get_model(self):
        raw_m = self._s.model()
        
        m = PySmtModel()
        for d in raw_m.decls():
            k, v = [d, None, None], [raw_m[d], None, None]
            m.set(k, v)
        return m

    def print_model(self):
        print(self._m)

    def set_logic(self, logic):
        # will recreate solver
        self._s = z3.SolverFor(logic)
        self._ss = z3.SolverFor(logic)
    
    def get_converter(self):
        return self._c