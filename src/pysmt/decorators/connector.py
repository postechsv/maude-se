"""Adapt raw solver callbacks to the SWIG Connector contract."""

from functools import wraps

from maudeSE.maude import Connector


_REQUIRED_CALLBACKS = (
    "check_sat", "subsume", "add_const", "get_model", "simplify",
    "push", "pop", "reset", "set_logic", "get_converter",
)


def connector(cls):
    """Keep the Connector subclass while managing Maude boundary values.

    Callback implementations receive raw solver expressions. ``get_model``
    returns an iterable of ``(variable, value)`` pairs.
    """
    if not isinstance(cls, type) or not issubclass(cls, Connector):
        raise TypeError("@connector requires a maudeSE.maude.Connector subclass")
    missing = [name for name in _REQUIRED_CALLBACKS if name not in cls.__dict__]
    if missing:
        raise TypeError(f"{cls.__name__} must define: {', '.join(missing)}")
    from maudeSE.maude import SmtModel, SmtTerm, get_data

    original = cls.check_sat
    @wraps(original)
    def check_sat(self, consts, _callback=original):
        return _callback(self, [get_data(const) for const in consts])
    cls.check_sat = check_sat

    original = cls.add_const
    @wraps(original)
    def add_const(self, acc, cur, _callback=original):
        result = _callback(self, None if acc is None else get_data(acc), get_data(cur))
        if result is None:
            raise TypeError("add_const must return a solver expression")
        return SmtTerm(result)
    cls.add_const = add_const

    original = cls.simplify
    @wraps(original)
    def simplify(self, term, _callback=original):
        result = _callback(self, get_data(term))
        if result is None:
            raise TypeError("simplify must return a solver expression")
        return SmtTerm(result)
    cls.simplify = simplify

    original = cls.subsume
    @wraps(original)
    def subsume(self, subst, prev, acc, cur, _callback=original):
        pairs = [
            (get_data(self.get_converter().dag2term(dag)),
             get_data(self.get_converter().dag2term(subst.get(dag))))
            for dag in subst.keys()
        ]
        return _callback(self, pairs, get_data(prev), get_data(acc), get_data(cur))
    cls.subsume = subsume

    original = cls.get_model
    @wraps(original)
    def get_model(self, _callback=original):
        pairs = _callback(self)
        model = SmtModel()
        for variable, value in pairs:
            model.set(variable, value)
        return model
    cls.get_model = get_model
    return cls
