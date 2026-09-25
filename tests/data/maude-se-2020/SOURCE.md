# Maude-SE 2020 examples

`gcd.maude` and `robot.maude` are Core Maude examples from the earlier
Maude-SE work, not examples introduced by the current Maude-SE paper.
The GCD system and symbolic search appear as Examples 1 and 3, and the robot
reachability study appears in Section 5 of *Maude-SE: a Tight Integration of
Maude and SMT Solvers* (WRLA 2020).

- Paper: https://wrla2020.webs.upv.es/pre-proceedings.pdf
- Earlier Maude-SE documentation, including the Core Maude versions:
  https://maude-se.github.io/old

`gcd.maude` is exercised by `../../regression/native/test_maude_se_features.sh`.
`robot.maude` is retained but is not yet a runnable regression input: it
references object-system modules that are not declared in that file.
