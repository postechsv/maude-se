from yices import *
from maudeSE.maude import *
from maudeSE.decorators import connector

@connector
class YicesConnector(Connector):
    def __init__(self, converter: Converter, logic=None):
        super().__init__()
        self._c = converter

        _logic = "QF_LRA" if logic is None else logic

        # set solver
        self._cfg: Config = Config()
        self._cfg.default_config_for_logic(_logic)

        self._ctx: Context = Context(self._cfg)
        self._m = None

    def __del__(self):
        # The Python Yices bindings do not automatically dispose of either
        # native allocation when their wrappers are garbage-collected.
        context = getattr(self, "_ctx", None)
        if context is not None and context.context is not None:
            context.dispose()
        config = getattr(self, "_cfg", None)
        if config is not None and config.config is not None:
            config.dispose()
    
    def check_sat(self, consts):
        fs = list()
        for const in consts:
            c, _ = const
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
            body, _ = cur
        else:
            (acc_f, _), (cur_t, _) = acc, cur
            body = Terms.yand([acc_f, cur_t])

        return (body, Terms.type_of_term(body))
    
    def simplify(self, term):
        return term

    def subsume(self, subst, prev, acc, cur):
        t_v = [source[0] for source, _ in subst]
        t_l = [target[0] for _, target in subst]
        self._ctx.assert_formula(Terms.ynot(Terms.implies(Terms.yand([acc[0], cur[0]]), Terms.subst(t_v, t_l, prev[0]))))

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
        pairs = []
        for t in raw_m.collect_defined_terms():
            try:
                ty = Terms.type_of_term(t)

                value_term = raw_m.get_value_as_term(t)
                value = (value_term, ty)
                if Terms.to_string(value_term) is None:
                    value = self._model_definition(raw_m, t)
                k, v = (t, ty), value
                pairs.append((k, v))
            except:
                continue
        raw_m.dispose()
        return pairs
    
    def print_model(self):
        print(self._m.to_string(80, 100, 0))

    @staticmethod
    def model_term_to_string(term):
        if isinstance(term, str):
            return term
        return str(Terms.to_string(term[0]))

    @staticmethod
    def _model_definition(model, term):
        text = model.to_string(120, 1000, 0)
        marker = f"(function {Terms.to_string(term)}"
        start = text.find(marker)
        if start < 0:
            return text
        depth = 0
        for end in range(start, len(text)):
            depth += (text[end] == "(") - (text[end] == ")")
            if depth == 0:
                return text[start:end + 1]
        return text[start:]

    def set_logic(self, logic):
        self._ctx.dispose()

        self._cfg: Config = Config()
        self._cfg.default_config_for_logic(logic)

        self._ctx: Context = Context(self._cfg)
        self._m = None

    def get_converter(self):
        return self._c
