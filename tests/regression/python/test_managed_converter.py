"""Prototype contract tests without requiring a compiled Maude extension."""

import importlib
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch
from dataclasses import dataclass


@dataclass(frozen=True)
class FakeDag:
    name: str
    children: tuple = ()
    variable: bool = False
    sort: str = "Integer"

    def isVariable(self):
        return self.variable

    def getSort(self):
        return self.sort

    def __str__(self):
        return self.name


class FakeSmtTerm:
    def __init__(self, value):
        self.value = value


class FakeConverter:
    def __init__(self):
        self.conversions = {}
        self.bindings = {}
        self.forward_bindings = {}

    def conversion_cache_find(self, dag):
        return self.conversions.get(dag)

    def conversion_cache_insert(self, dag, term):
        self.conversions[dag] = term

    def cache_find(self, term):
        if isinstance(term, FakeSmtTerm):
            return self.bindings.get(term.value)
        return self.forward_bindings.get(term)

    def cache_insert(self, dag, term):
        self.bindings[term.value] = dag
        self.forward_bindings[dag] = term


class FakeConnector:
    pass


class FakeSmtModel:
    def __init__(self):
        self.values = {}

    def set(self, key, value):
        self.values[key] = value


class ManagedConverterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        package = types.ModuleType("maudeSE")
        package.__path__ = [str(Path(__file__).resolve().parents[3] / "src/pysmt")]
        fake = types.ModuleType("maudeSE.maude")
        fake.Converter = FakeConverter
        fake.Connector = FakeConnector
        fake.SmtTerm = FakeSmtTerm
        fake.SmtModel = FakeSmtModel
        fake.get_data = lambda term: term.value
        cls.patch = patch.dict(sys.modules, {"maudeSE": package, "maudeSE.maude": fake})
        cls.patch.start()
        cls.converter = staticmethod(importlib.import_module("maudeSE.decorators").converter)
        cls.connector = staticmethod(importlib.import_module("maudeSE.decorators").connector)

    @classmethod
    def tearDownClass(cls):
        cls.patch.stop()
        sys.modules.pop("maudeSE.decorators", None)
        sys.modules.pop("maudeSE.decorators.converter", None)
        sys.modules.pop("maudeSE.decorators.connector", None)

    def test_wrapping_recursion_cache_and_reverse_binding(self):
        calls = []

        @self.converter
        class Example:
            def dag2term(self, dag):
                calls.append(dag)
                if dag.children:
                    return ("and", *(self.dag2term(child) for child in dag.children))
                return f"term:{dag}"

            def term2dag(self, expression):
                return f"reversed:{expression}"

        adapter = Example()
        x = FakeDag("x", variable=True)
        whole = FakeDag("and", (x, x))
        first = adapter.dag2term(whole)
        self.assertEqual(first.value, ("and", "term:x", "term:x"))
        self.assertEqual(calls, [whole, x])
        self.assertEqual(adapter.dag2term(whole).value, first.value)
        self.assertEqual(len(calls), 2)
        self.assertEqual(adapter.term2dag(FakeSmtTerm("term:x")), x)
        self.assertEqual(adapter.term2dag(FakeSmtTerm("other")), "reversed:other")

    def test_missing_callback_and_invalid_forward_result(self):
        with self.assertRaisesRegex(TypeError, "term2dag"):
            @self.converter
            class Missing:
                def dag2term(self, dag):
                    return dag

        @self.converter
        class Invalid:
            def dag2term(self, dag):
                return None

            def term2dag(self, expression):
                return None

        adapter = Invalid()
        with self.assertRaisesRegex(TypeError, "dag2term"):
            adapter.dag2term(FakeDag("x"))
        self.assertIsNone(adapter.term2dag(FakeSmtTerm("x")))

    def test_only_declared_smt_variable_sorts_are_reverse_bound(self):
        @self.converter
        class Example:
            def dag2term(self, dag):
                return f"term:{dag}"

            def term2dag(self, expression):
                return f"parsed:{expression}"

        adapter = Example()
        for sort in ("BooleanVar", "IntegerVar", "RealVar"):
            dag = FakeDag(sort, sort=sort)
            adapter.dag2term(dag)
            self.assertEqual(adapter.term2dag(FakeSmtTerm(f"term:{sort}")), dag)

        unrelated = FakeDag("CustomVar", sort="CustomVar")
        adapter.dag2term(unrelated)
        self.assertEqual(
            adapter.term2dag(FakeSmtTerm("term:CustomVar")),
            "parsed:term:CustomVar",
        )

    def test_connector_marker_rejects_incomplete_implementations(self):
        with self.assertRaisesRegex(TypeError, "Connector subclass"):
            self.connector(object)
        with self.assertRaisesRegex(TypeError, "check_sat"):
            self.connector(type("Incomplete", (FakeConnector,), {}))

    def test_connector_manages_boundary_values(self):
        class RawConnector(FakeConnector):
            def check_sat(self, consts):
                return consts

            def subsume(self, subst, prev, acc, cur):
                return subst, prev, acc, cur

            def add_const(self, acc, cur):
                return (acc or 0) + cur

            def get_model(self):
                return [("x", 7)]

            def simplify(self, term):
                return term + 1

            def push(self): pass
            def pop(self): pass
            def reset(self): pass
            def set_logic(self, logic): pass

            def get_converter(self):
                return self

            def dag2term(self, dag):
                return FakeSmtTerm(dag)

        managed = self.connector(RawConnector)()
        self.assertEqual(managed.check_sat([FakeSmtTerm(2)]), [2])
        self.assertEqual(managed.add_const(None, FakeSmtTerm(3)).value, 3)
        self.assertEqual(managed.add_const(FakeSmtTerm(2), FakeSmtTerm(3)).value, 5)
        self.assertEqual(managed.simplify(FakeSmtTerm(3)).value, 4)
        self.assertEqual(managed.subsume({2: 3}, FakeSmtTerm(4), FakeSmtTerm(5), FakeSmtTerm(6)),
                         ([(2, 3)], 4, 5, 6))
        self.assertEqual(managed.get_model().values, {"x": 7})


if __name__ == "__main__":
    unittest.main()
