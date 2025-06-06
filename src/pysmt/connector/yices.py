from yices import *
from maudeSE.maude import *
from maudeSE.util import id_gen
from maudeSE.maude import *

class YicesConnector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter
        self._g = id_gen()

        _logic = "QF_LRA" if logic is None else logic

        # set solver
        self._cfg: Config = Config()
        self._cfg.default_config_for_logic(_logic)

        self._ctx: Context = Context(self._cfg)
        self._m = None
    
    def check_sat(self, consts):
        fs = list()
        for const in consts:
            c, _ = get_data(const)
            fs.append(c)

        self._ctx.assert_formulas(fs)
        r = self._ctx.check_context()

        if r == Status.SAT:
            return sat
        elif r == Status.UNSAT:
            return unsat
        else:
            return unknown
        
    def push(self):
        self._ctx.push()

    def pop(self):
        self._ctx.pop()
    
    def reset(self):
        self._ctx.reset_context()

    def add_const(self, acc, cur):
        # initial case
        if acc is None:
            body, _ = get_data(cur)
        else:
            (acc_f, _), (cur_t, _) = get_data(acc), get_data(cur)
            body = Terms.yand([acc_f, cur_t])

        return SmtTerm((body, Terms.type_of_term(body)))
    
    def simplify(self, term):
        return term

    def subsume(self, subst, prev, acc, cur):
        t_v, t_l = list(), list()
        sub = subst.keys()
        for p in sub:
            src, _ = get_data(self._c.dag2term(p))
            trg, _ = get_data(self._c.dag2term(subst.get(p)))

            t_v.append(src)
            t_l.append(trg)

        prev_c, _ = get_data(prev)

        acc_c, _ = get_data(acc)
        cur_c, _ = get_data(cur)
    
        self._ctx.assert_formula(Terms.ynot(Terms.implies(Terms.yand([acc_c, cur_c]), Terms.subst(t_v, t_l, prev_c))))

        r = self._ctx.check_context()

        if r == Status.UNSAT:
            return True
        elif r == Status.SAT:
            return False
        else:
            raise Exception("failed to apply subsumption (give-up)")

    def merge(self, subst, prev_t, prev, cur_t, acc, cur):
        pass

    def get_model(self):
        raw_m = Model.from_context(self._ctx, 1)
        m = SmtModel()
        for t in raw_m.collect_defined_terms():
            try:
                v = Terms.parse_term(str(raw_m.get_value(t)).lower())
                ty1, ty2 = Terms.type_of_term(t), Terms.type_of_term(v)

                if ty1 != ty2:
                    continue

                k, v = (t, ty1), (v, ty2)
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