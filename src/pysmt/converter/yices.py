import ctypes

from maudeSE.util import *
from functools import reduce

from maudeSE.maude import *
from yices import *
from yices_api import *
import re


class YicesConverter(Converter):
    """A term converter from Maude to Yices"""

    def __init__(self):
        Converter.__init__(self)
        self._g = id_gen()
        self._symbol_info = dict()
        self._symbol_map = dict()
        self._fun_dict = dict()

        # smt.maude map
        self._op_dict = {
            "not_"          : Terms.ynot,
            "_and_"         : Terms.yand,
            "_or_"          : Terms.yor,
            "_xor_"         : Terms.xor,
            "_implies_"     : Terms.implies,
            "_===_"         : Terms.eq,
            "_=/==_"        : Terms.neq,
            "_?_:_"         : Terms.ite,

            "-_"            : Terms.neg,
            "_+_"           : Terms.add,
            "_-_"           : Terms.sub,
            "_*_"           : Terms.mul,
            "_div_"         : Terms.idiv,
            "_/_"           : Terms.division,
            "_mod_"         : Terms.imod,

            "_divisible_"   : Terms.divides_atom,

            "_<_"           : Terms.arith_lt_atom,
            "_<=_"          : Terms.arith_leq_atom,
            "_>_"           : Terms.arith_gt_atom,
            "_>=_"          : Terms.arith_geq_atom,

            "toInteger"     : Terms.floor,
            # "toReal"        : yices_to_real,
            "isInteger"     : Terms.is_int_atom,
        }

        self._bool_const = {
            "true"          : Terms.true,
            "false"         : Terms.false,
        }

        self._num_const = {
            "<Integers>"    : Terms.parse_rational,
            "<Reals>"       : Terms.parse_rational,
        }

        self._special_var_sort = {
            "IntegerVar"        : Types.int_type,
            "RealVar"           : Types.real_type,
            "BooleanVar"        : Types.bool_type,
        }

        self._sort_dict = {
            "Integer"           : Types.int_type,
            "Real"              : Types.real_type,
            "Boolean"           : Types.bool_type,
            "IntegerExpr"       : Types.int_type,
            "RealExpr"          : Types.real_type,
            "BooleanExpr"       : Types.bool_type,
            "Array"             : Types.new_function_type,
        }

        self._param_sort = dict()
        self._user_sort_dict = dict()

        # extra theory symbol map 
        self._theory_dict = {
            "array" : {
                "select"    : Terms.application,
                "store"     : Terms.update,
            }
        }

        self._func_dict = dict()
        self._module = None
        self._dag2term_memoize = dict()

    def prepareFor(self, module: Module):
        # clear previous
        self._param_sort.clear()
        self._user_sort_dict.clear()
        self._func_dict.clear()
        self._symbol_map.clear()
        self._symbol_info.clear()
        self._fun_dict.clear()
        self._dag2term_memoize.clear()
        self._module = None

        # recreate the id generator
        self._g = id_gen()

        self._symbol_info = get_symbol_info(module)

        # build symbol map table
        for k in self._symbol_info:
            user_symbol, sorts = k
            yices_sorts = [self._decl_sort(sort) for sort in sorts]

            th, sym = self._symbol_info[k]

            key = (user_symbol, tuple(yices_sorts))

            # euf
            if sym is None:
                assert th == "euf"
            
                f = self._decl_func(user_symbol, *yices_sorts)
                if key in self._symbol_map:
                    raise Exception("found ambiguous symbol ({} : {})".format(user_symbol, " ".join(sorts)))
                else:
                    self._symbol_map[key] = (f, th, th)
            else:
                # mapping an interpreted function
                if th not in self._theory_dict:
                    raise Exception(f"theory {th} is not registered")

                if sym not in self._theory_dict[th]:
                    raise Exception(f"a symbol {sym} does not exist in the theory {th}")
                
                self._symbol_map[key] = (self._theory_dict[th][sym], th, sym)
        
        self._module = module

    def _get_param_sort_info(self, sort: str):
        m = re.match(r'.*{.*}', sort)
        if m is not None:
            g = m.group().split('{')
            name, p_str = g[0], g[1].replace('}', '')

            # parametric sort name should be in sort dict
            if name not in self._sort_dict:
                raise Exception(f"failed to declare sort {sort}")

            # parse params
            p_str = p_str.replace(' ','')
            params = p_str.split(',')

            return (name, *params)
        
        return None

    def _decl_sort(self, sort: str):
        # check if sort is parametric
        paramInfo = self._get_param_sort_info(sort)
        if paramInfo is not None:
            (name, *params) = paramInfo
            param_sorts = [self._decl_sort(p_sort) for p_sort in params]

            k = (name, tuple(param_sorts))
            # check if it already declared
            if k in self._param_sort:
                return self._param_sort[k]
            
            # in Yices, this is a function sort
            doms, rng = param_sorts[:-1], param_sorts[-1]
            self._param_sort[k] = Types.new_function_type(doms, rng, name)

            return self._param_sort[k]

        if sort in self._sort_dict:
            return self._sort_dict[sort]()

        if sort not in self._user_sort_dict:
            self._user_sort_dict[sort] = Types.new_uninterpreted_type(sort)

        return self._user_sort_dict[sort]

    def _decl_func(self, func: str, *args):
        key = (func, *args)
        if key not in self._func_dict:
            raw_args = list(args)
            doms, rng = raw_args[:-1], raw_args[-1]
            self._func_dict[key] = Types.new_function_type(doms, rng, func)

        return self._func_dict[key]
    
    def term2dag(self, term):
        try:
            return self._module.parseTerm(self._term2dag(get_data(term)))
        except:
            return None

    def _term2dag(self, term):
        cached_dag = self.cache_find(SmtTerm(term))
        if cached_dag:
            return str(cached_dag)

        t, ty = term

        # variable or function
        constructor = Terms.constructor(t)
        if constructor == YICES_UNINTERPRETED_TERM:
            # variable
            sort_table = {Types.int_type() : "Integer", 
                          Types.real_type() : "Real", 
                          Types.bool_type() : "Boolean"}

            assert ty in sort_table

            return f"{Terms.get_name(t)}:{sort_table[ty]}"
        
        # numerical
        if constructor == YICES_ARITH_CONSTANT:
            if ty == Types.int_type():
                return f"({Terms.to_string(t)}).Integer"
            else:
                # hack
                if Terms.type_of_term(t) == Types.int_type():
                    return f"({Terms.to_string(t)}/1).Real"
                else:
                    return f"({Terms.to_string(t)}).Real"
        
        # Boolean
        if constructor == YICES_BOOL_CONSTANT:
            return f"({Terms.to_string(t)}).Boolean"
        

        bool_type = Types.bool_type()
        real_type = Types.real_type()
        int_type = Types.int_type()

        if constructor == YICES_NOT_TERM:
            c = yices_term_child(t, 0)
            child = self._term2dag((c, bool_type)) 
            return f"(not {child})"

        if constructor == YICES_OR_TERM: 
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]

            l = self._term2dag((ts[0], bool_type))
            r = self._term2dag((ts[1], bool_type))
            return f"({l} or {r})"
        
        if constructor == YICES_XOR_TERM:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]

            l = self._term2dag((ts[0], bool_type))
            r = self._term2dag((ts[1], bool_type))
            return f"({l} xor {r})"

        if constructor == YICES_EQ_TERM:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]

            # real type
            if Terms.type_of_term(ts[0]) == real_type or Terms.type_of_term(ts[1]) == real_type:
                l = self._term2dag((ts[0], real_type))
                r = self._term2dag((ts[1], real_type))
                return f"({l} === {r})"
            elif Terms.type_of_term(ts[0]) == int_type and Terms.type_of_term(ts[1]) == int_type:
                l = self._term2dag((ts[0], int_type))
                r = self._term2dag((ts[1], int_type))
                return f"({l} === {r})"
            else:
                l = self._term2dag((ts[0], bool_type))
                r = self._term2dag((ts[1], bool_type))
                return f"({l} === {r})"

        if constructor == YICES_ITE_TERM:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1), yices_term_child(t, 2)]

            if Terms.type_of_term(ts[1]) == int_type:
                c = self._term2dag((ts[0], bool_type))
                l = self._term2dag((ts[1], int_type))
                r = self._term2dag((ts[2], int_type))
                return f"({c} ? {l} : {r})"

            elif Terms.type_of_term(ts[1]) == real_type:
                c = self._term2dag((ts[0], bool_type))
                l = self._term2dag((ts[1], real_type))
                r = self._term2dag((ts[2], real_type))
                return f"({c} ? {l} : {r})"

            else:
                c = self._term2dag((ts[0], bool_type))
                l = self._term2dag((ts[1], bool_type))
                r = self._term2dag((ts[2], bool_type))
                return f"({c} ? {l} : {r})"

        if constructor == YICES_ARITH_GE_ATOM:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]
            if Terms.type_of_term(ts[0]) == real_type or Terms.type_of_term(ts[1]) == real_type:
                l = self._term2dag((ts[0], real_type))
                r = self._term2dag((ts[1], real_type))
                return f"({l} >= {r})"
            else:
                l = self._term2dag((ts[0], int_type))
                r = self._term2dag((ts[1], int_type))
                return f"({l} >= {r})"

        if constructor == YICES_IS_INT_ATOM:
            return f"(isInteger({self._term2dag((yices_term_child(t, 0), real_type))}))"

        if constructor == YICES_IDIV:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]
 
            l = self._term2dag((ts[0], int_type))
            r = self._term2dag((ts[1], int_type))
            return f"({l} div {r})"

        if constructor == YICES_RDIV:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]
 
            l = self._term2dag((ts[0], int_type))
            r = self._term2dag((ts[1], int_type))
            return f"({l} / {r})"

        if constructor == YICES_IMOD:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]
 
            l = self._term2dag((ts[0], int_type))
            r = self._term2dag((ts[1], int_type))
            return f"({l} mod {r})"
        
        if constructor == YICES_DIVIDES_ATOM:
            ts = [yices_term_child(t, 0), yices_term_child(t, 1)]
 
            l = self._term2dag((ts[0], int_type))
            r = self._term2dag((ts[1], int_type))
            return f"({l} divisible {r})"

        if constructor == YICES_FLOOR:
            child = yices_term_child(t, 0)
            r = self._term2dag((child, Terms.type_of_term(child)))
            return f"toInteger({r})"

        if constructor == YICES_POWER_PRODUCT:
            child_num = yices_term_num_children(t)
            args = list()
            
            for i in range(child_num):
                c_t = ctypes.c_int32()
                exp = ctypes.c_int32()
                yices_product_component(t, i, c_t, exp)
                args.append(self._term2dag((c_t.value, Terms.type_of_term(t))))

            r = " * ".join(args)
            return f"({r})"


        if constructor == YICES_ARITH_SUM:
            child_num = yices_term_num_children(t)
            args = list()

            for i in range(child_num):
                coeff = mpq_t()
                c_t = ctypes.c_int32()
                yices_sum_component(t, i, coeff, c_t)

                coeff_t = yices_mpq(coeff);

                if Terms.to_string(c_t) is None:
                    args.append(self._term2dag((coeff_t, Terms.type_of_term(t))))
                else:
                    coc = self._term2dag((coeff_t, Terms.type_of_term(t)))
                    c = self._term2dag((c_t.value, Terms.type_of_term(t)))

                    args.append(f"{coc} * {c}")

            r = " + ".join(args)
            return f"({r})"
        
        raise Exception("failed to apply term2dag")

    def dag2term(self, t: Term):
        """translate a maude term to a Yices SMT solver term

        :param t: A maude term
        :returns: A pair of
          an SMT solver term and its variables
        """
        return SmtTerm(self._dag2term(t))
    
    def _dag2term(self, t: Term):
        cached_term = self.cache_find(t)
        if cached_term:
            return get_data(cached_term)

        symbol, symbol_sort = str(t.symbol()), str(t.getSort())
        
        if symbol == "toReal":
            child = list(t.arguments())
            assert len(child) == 1
            c, ty = self._dag2term(child[0])

            assert ty == Types.int_type()
            return c, Types.real_type()

        if symbol_sort in self._special_var_sort:
            # remove "var" from type for backward compatibility
            name = f"{symbol}_{symbol_sort[:-3]}_{next(self._g)}"
            sort = self._special_var_sort[symbol_sort]()

            v = Terms.new_uninterpreted_term(sort, name)

            ns = (v, sort)
            self.cache_insert(t, SmtTerm(ns))
            return ns

        if t.isVariable():
            v_name = t.getVarName()

            v = None
            if symbol_sort in self._sort_dict:
                sort = self._sort_dict[symbol_sort]()
                v = Terms.new_uninterpreted_term(sort, v_name)
            
            if symbol_sort in self._user_sort_dict:
                sort = self._user_sort_dict[symbol_sort]
                v = Terms.new_uninterpreted_term(sort, v_name)

            paramInfo = self._get_param_sort_info(symbol_sort)
            if paramInfo is not None:
                (name, *params) = paramInfo
                param_sorts = [self._decl_sort(p_sort) for p_sort in params]

                k = (name, tuple(param_sorts))

                if k in self._param_sort:
                    sort = self._param_sort[k]
                    v = Terms.new_uninterpreted_term(sort, v_name)
            
            if v is not None:
                self.cache_insert(t, SmtTerm((v, sort)))
                return v, sort

            raise Exception("wrong variable {} with sort {}".format(v_name, symbol_sort))

        symbol, symbol_sort = str(t.symbol()), str(t.getSort())

        sorts = [self._decl_sort(str(arg.symbol().getRangeSort())) for arg in t.arguments()]
        sorts.append(self._decl_sort(str(t.symbol().getRangeSort())))
        k = (symbol, tuple(sorts))

        if k in self._symbol_map:
            p_args = [self._dag2term(arg) for arg in t.arguments()]

            sym, th, name = self._symbol_map[k]

            raw_args = list(map(lambda x: x[0], p_args))
            # v_s = reduce(lambda acc, cur: acc.union(cur[1]), p_args, set())

            fun_key = (sym, symbol)
            if th == "euf": 

                # get function type and make a function term
                if fun_key not in self._func_dict:
                    self._func_dict[fun_key] = Terms.new_uninterpreted_term(sym, symbol)

                _f = self._func_dict[fun_key]

                f = Terms.application(_f, raw_args)
            else:
                # interpreted, currently this is implemented in an ad-hoc way.
                if name == "select":
                    _f, a = raw_args[0], raw_args[1:]
                    f = Terms.application(_f, a)
                elif name == "store":
                    # func, args, value
                    assert len(raw_args) == 3
                    t, a, v = raw_args[0], raw_args[1:-1], raw_args[-1]

                    f = sym(t, a, v)
                else:
                    if fun_key not in self._func_dict:
                        self._func_dict[fun_key] = Terms.new_uninterpreted_term(sym, symbol)

                    _f = self._func_dict[fun_key]

                    f = Terms.application(f, raw_args)

            return f, Terms.type_of_term(f)

        if symbol in self._bool_const:
            return self._bool_const[symbol](), Types.bool_type()
        
        if symbol in self._num_const:
            val = str(t)
            # remove unnecessary postfix
            for s in self._sort_dict:
                val = val.replace(f".{s}", "")

            # remove parenthesis 
            val = val.replace("(", "").replace(")", "")
            c = self._num_const[symbol](val)
            ty = Types.real_type() if symbol == "<Reals>" else Types.int_type()
            return c, ty

        if symbol in self._op_dict:
            p_args = [self._dag2term(arg) for arg in t.arguments()]
            op = self._op_dict[symbol]

            raw_args = list(map(lambda x: x[0], p_args))
            # multinary
            if symbol in ["_and_", "_or_", "_xor_"]:
                return op(raw_args), Types.bool_type()
            elif symbol == "_?_:_":
                tys = list(map(lambda x: x[1], p_args))
                assert tys[1] == tys[2]

                return op(*raw_args), tys[1]
            elif symbol == "_divisible_":
                raw_args.reverse()
                st = op(*raw_args)
                return st, Types.bool_type()
            else:
                symbs = ["-_", "_+_", "_-_", "_*_", "_/_"]
                tys = list(map(lambda x: x[1], p_args))
                st = op(*raw_args)

                if symbol in symbs:
                    ty = tys[0]
                else:
                    ty = Terms.type_of_term(st)

                return st, ty
        
        raise Exception(f"fail to apply dag2term to \"{t}\"")