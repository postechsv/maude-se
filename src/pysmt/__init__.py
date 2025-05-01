import argparse
from .maude import *
from .factory import Factory
# from maudeSE.hook.check import *
# from maudeSE.hook.search import *

def main():
    solvers = ["z3","yices","cvc5"]
    default_s = solvers[0]

    s_help = ["set an underlying SMT solver",
              "* Supported solvers: {{{}}}".format(",".join(solvers)),
              "* Usage: -s {}".format(solvers[-1]), "* Default: -s {}".format(default_s)]
    
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file', nargs='?', type=str, help="input Maude file")
    parser.add_argument("-s", "-solver", metavar="SOLVER", nargs='?', type=str,
                        help="\n".join(s_help), default=default_s)
    parser.add_argument("-no-meta", help="no metaInterpreter", action="store_true")
    args = parser.parse_args()

    try:
        # instantiate our interface
        # factory = Factory()
        # factory.set_solver(args.s)
        # factorySetter = maudeSE.maude.SmtManagerFactorySetter()
        # factorySetter.setSmtManagerFactory(factory)
        setSmtSolver(args.s)
        # maudeSE.maude.cvar.smtManagerFactory = Factory()
        setSmtManagerFactory(Factory().__disown__())

        # initialize Maude interpreter
        init(advise=False)

        # conv = factory.createConverter()
        # conn = factory.createConnector()
        # conv = conn.get_converter()

        # register special hooks
        # searchPathHook = SearchHook(conn, conv, path=True)
        # maudeSE.maude.connectEqHook('SmtSearchPathSymbol', searchPathHook)

        # load preludes

        # maudeSE.maude.load('smt.maude')
        # maudeSE.maude.load('smt-check.maude')

        # load an input file

        if args.file is None:
            raise ValueError("should provide an input Maude file")
        
        load(args.file)

        if args.no_meta == False:
            load('maude-se-meta.maude')

    except Exception as err:
        print("error: {}".format(err))