import time

from yices import *
from ..maude import *
from ..util import id_gen
from maudeSE.maude import *

class YicesConnector(PyConnector):
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
        self._cfg: Config = Config()
        self._cfg.default_config_for_logic(_logic)

        self._ctx: Context = Context(self._cfg)
        self._m = None
    
    def check_sat(self, consts):
        self._ctx.push()

        try:
            fs = list()
            for const in consts:
                (c, _), _, _ = const.data()
                fs.append(c)

            self._ctx.assert_formulas(fs)
            r = self._ctx.check_context()

            if r == Status.SAT:
                # store model
                self._m = Model.from_context(self._ctx, 1)
                self._ctx.pop()
                return True
            elif r == Status.UNSAT:
                self._ctx.pop()
                return False
            else:
                self._ctx.pop()
                raise Exception("failed to handle check sat (solver give-up)")
        except:
            import traceback
            print(traceback.print_exc())

    def add_const(self, acc, cur):
        # initial case
        if acc is None:
            ((cur_t, _), _, cur_v) = cur.data()
            body = cur_t

        else:
            ((acc_f, _), _, acc_v), ((cur_t, _), _, cur_v) = acc.data(), cur.data()
            body = Terms.yand([acc_f, cur_t])

        return PySmtTerm([(body, Terms.type_of_term(body)), None, None])

    def subsume(self, subst, prev, acc, cur):
        s = time.time()

        d_s = time.time()
        t_v, t_l = list(), list()
        sub = subst.getSubst()
        for p in sub:
            (src, _), _, _ = self._c.dag2term(p).data()
            (trg, _), _, _ = self._c.dag2term(sub[p]).data()

            t_v.append(src)
            t_l.append(trg)

        d_e = time.time()

        self._dt += d_e - d_s

        (prev_c, _), _, _ = prev.data()

        (acc_c, _), _, _ = acc.data()
        (cur_c, _), _, _ = cur.data()
    
        so_s = time.time()
        self._ctx.push()
        self._ctx.assert_formula(Terms.ynot(Terms.implies(Terms.yand([acc_c, cur_c]), Terms.subst(t_v, t_l, prev_c))))

        r = self._ctx.check_context()

        self._ctx.pop()
        so_e = time.time()
        self._st += so_e - so_s

        if r == Status.UNSAT:
            e = time.time()
            self._tt += e - s
            return True
        elif r == Status.SAT:
            e = time.time()
            self._tt += e - s
            return False
        else:
            raise Exception("failed to apply subsumption (give-up)")

    def merge(self, subst, prev_t, prev, cur_t, acc, cur):
        mo = prev_t.symbol().getModule()
        
        rename_dict = dict()
        t_l, t_r, t_s = list(), list(), list()
        eq_l, eq_r = list(), list()
        for p in subst:
            if p == subst[p]:
                continue

            idx = next(self._g)

            src, _, _ = self._c.dag2term(p)
            trg, _, _ = self._c.dag2term(subst[p])

            n_trg = Terms.new_uninterpreted_term(Terms.type_of_term(trg), f"#mergeVar{idx}")
            newVar = mo.parseTerm(f"#mergeVar{idx}:{p.getSort()}")

            eq_l.append(Terms.eq(trg, n_trg))
            eq_r.append(Terms.eq(src, n_trg))

            t_l.append(trg)
            t_r.append(src)
            t_s.append(n_trg)
            
            rename_dict[subst[p]] = newVar
            
        prev_c, _, _ = prev

        (acc_f, _, _), (cur_c, _, _) = acc, cur

        conj = Terms.yand([acc_f, cur_c])

        c = Terms.yor([Terms.yand([Terms.subst(t_l, t_s, prev_c), *eq_l]), Terms.yand([Terms.subst(t_r, t_s, conj), *eq_r])])

        n_subst = Substitution(rename_dict)
        new_prev = n_subst.instantiate(prev_t)

        return tuple([new_prev, tuple([c, None, None])])
    
    # def mkSubst(self, vars, vals):
    #     subst = dict()
    #     for v, val in zip(vars, vals):
    #         subst[v] = val
    #     return TermSubst(subst)

    def get_model(self):
        # hack
        # model_dict = dict()
        # for t in self._m.collect_defined_terms():
        #     try:
        #         ty = Terms.type_of_term(t)
        #         model_dict[PySmtTerm([(t, ty), None, None])] = PySmtTerm([(Terms.parse_term(str(self._m.get_value(t)).lower()), ty), None, None])
        #     except:
        #         continue
        # return model_dict
        m = PySmtModel()
        for t in self._m.collect_defined_terms():
            try:
                ty = Terms.type_of_term(t)
                k, v = [(t, ty), None, None], [(Terms.parse_term(str(self._m.get_value(t)).lower()), ty), None, None]
                m.set(k, v)
            except:
                continue
        return m
    
    def print_model(self):
        print(self._m.to_string(80, 100, 0))

    def set_logic(self, logic):
        self._ctx.dispose()

        self._cfg: Config = Config()
        self._cfg.default_config_for_logic(logic)

        self._ctx: Context = Context(self._cfg)
        self._m = None

    def get_converter(self):
        return self._c




















# class YicesConverter(Converter):
#     """A term converter from Maude to Yices"""

#     def __init__(self):
#         self._cfg: Config = Config()
#         self._cfg.default_config_for_logic("QF_LRA")

#         self._ctx: Context = Context(self._cfg)
#         self._model = None

#     def __del__(self):
#         self._ctx.dispose()
#         self._cfg.dispose()

#     def make_assignment(self):
#         if self._model is None:
#             raise Exception("Yices solver error occurred during making assignment (no model exists)")
#         return YicesAssignment(self._model)

#     def push(self):
#         self._ctx.push()

#     def pop(self):
#         self._ctx.pop()

#     def reset(self):
#         self._ctx.reset_context()

#     def add(self, formula: Formula):
#         self._ctx.assert_formula(Terms.parse_term(translate(formula)))

#     def assert_and_track(self, formula: Formula, track_id: str):
#         pass

#     def unsat_core(self):
#         return self._ctx.get_unsat_core()

#     def _clear_model(self):
#         if self._model is not None:
#             self._model.dispose()
#             self._model = None



# class YicesAssignment(Assignment):
#     def __init__(self, _yices_model):
#         self._yices_model = _yices_model
#         Assignment.__init__(self)

#     # solver_model_to_generalized_model
#     def _get_assignments(self):
#         new_dict = dict()
#         for e in self._yices_model.collect_defined_terms():
#             t, v = Terms.to_string(e), self._yices_model.get_float_value(e)
#             if Terms.is_real(e):
#                 new_dict[Real(t)] = RealVal(str(v))
#             elif Terms.is_int(e):
#                 new_dict[Int(t)] = IntVal(str(v))
#             elif Terms.is_bool(e):
#                 new_dict[Bool(t)] = BoolVal(str(v))
#             else:
#                 Exception("cannot generate assignments")
#         return new_dict
