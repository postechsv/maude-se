"""Adapt paper-style Python converters to the existing SWIG interface.

The decorated class implements raw ``dag2term`` and ``term2dag`` callbacks.
Its generated adapter implements the existing SWIG ``Converter`` interface.
"""

from weakref import proxy

SMT_VARIABLE_SORTS = frozenset(("BooleanVar", "IntegerVar", "RealVar"))


def converter(cls):
    """Adapt paper-named raw callbacks to the existing SWIG Converter contract.

    ``cls.dag2term(dag)`` returns a raw solver expression;
    ``cls.term2dag(expression)`` returns a Maude DAG.  The generated
    adapter owns SmtTerm wrapping and the C++-backed conversion cache.
    """
    from maudeSE.maude import Converter, SmtTerm, get_data

    for name in ("dag2term", "term2dag"):
        if not callable(getattr(cls, name, None)):
            raise TypeError(f"{cls.__name__} must define {name}(value)")

    class Implementation(cls):
        def dag2term(self, dag):
            # Recursive self.dag2term(child) uses the same managed cache.
            return self._managed_adapter._raw_dag2term(dag)

        # Existing converters may use a recursive string builder for reverse
        # conversion.  Apply the identity lookup at every recursive call.
        if callable(getattr(cls, "_term2dag", None)):
            def _term2dag(self, expression):
                cached = self._managed_adapter.cache_find(
                    self._managed_adapter._wrap(expression)
                )
                if cached is not None:
                    return str(cached)
                return super()._term2dag(expression)

    class ManagedConverter(Converter):
        def __init__(self, *args, **kwargs):
            super().__init__()
            self._implementation = Implementation(*args, **kwargs)
            self._implementation._managed_adapter = proxy(self)

        @staticmethod
        def _wrap(expression):
            return SmtTerm(expression)

        @property
        def solver(self):
            # cvc5 terms are owned by the converter's solver/term manager.
            # Its connector must receive that same instance.
            return self._implementation.solver

        def prepareFor(self, module):
            callback = getattr(self._implementation, "prepareFor", None)
            if callback is not None:
                callback(module)

        def _raw_dag2term(self, dag):
            cached = self.conversion_cache_find(dag)
            if cached is not None:
                return get_data(cached)
            bound = self.cache_find(dag)
            if bound is not None:
                return get_data(bound)
            expression = cls.dag2term(self._implementation, dag)
            if expression is None:
                raise TypeError("dag2term must return a solver expression, not None")
            wrapped = self._wrap(expression)
            self.conversion_cache_insert(dag, wrapped)
            # SMT-CHECK defines b/i/r : SMTVarId -> Boolean/Integer/RealVar.
            # Preserve those ground SMT variables and ordinary Maude variables
            # for reverse conversion; unrelated sorts ending in "Var" are not
            # SMT variables merely because of their names.
            if dag.isVariable() or str(dag.getSort()) in SMT_VARIABLE_SORTS:
                self.cache_insert(dag, wrapped)
            return expression

        def dag2term(self, dag):
            return self._wrap(self._raw_dag2term(dag))

        def term2dag(self, term):
            expression = get_data(term)
            cached = self.cache_find(self._wrap(expression))
            if cached is not None:
                return cached
            return cls.term2dag(self._implementation, expression)

    ManagedConverter.__name__ = cls.__name__
    ManagedConverter.__qualname__ = cls.__qualname__
    ManagedConverter.__module__ = cls.__module__
    ManagedConverter.__doc__ = cls.__doc__
    ManagedConverter.__wrapped__ = cls
    return ManagedConverter
