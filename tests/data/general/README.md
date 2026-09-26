# General Maude models

`bakery/bakery.maude` and `dining-philosophers/dining-philosophers.maude`
are exercised by `../../regression/native/test_general_models.sh` using
small reachable-state checks. The latter model had two conditional rules
written with `rl`; these were corrected to `crl` so Maude can parse them.

The Bakery file itself attributes its model to *All About Maude – A
High-Performance Logical Framework*. `bakery-analysis.maude` is preserved,
but its `tool2` dependency is absent and it is not part of the executable
test. The exact origin of this Dining Philosophers encoding remains
unverified. `abst.maude` and `filter.maude` are retained as additional
general models without automated assertions yet.
