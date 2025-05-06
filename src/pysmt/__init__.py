import argparse
from .maude import *
from .factory import Factory

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
        if args.s not in solvers:
            raise ValueError("Unsupported solver : {}".format(args.s))

        setSmtSolver(args.s)
        setSmtManagerFactory(Factory().__disown__())

        # initialize Maude interpreter
        init(advise=False)

        if args.file is None:
            raise ValueError("should provide an input Maude file")
        
        # load an input file
        load(args.file)

        if args.no_meta == False:
            load('maude-se-meta.maude')

    except Exception as err:
        print("error: {}".format(err))