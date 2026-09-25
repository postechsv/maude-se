import argparse

def main():
    from maudeSE.installer import SOLVERS, check_solver
    from maudeSE.maude import init, load, setSmtSolver
    from maudeSE.factory import Factory
    from maudeSE.util import (
        check_config, load_class_from_file, load_config, load_user_config,
        update_config,
    )
    import os
    
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file', nargs='?', type=str, help="input Maude file")
    parser.add_argument("-cfg", "-config", metavar="CONFIG", type=str, 
                        help="a directory to a configuration file (default: \"config.yml\")")
    parser.add_argument("-s", "-solver", metavar="SOLVER", type=str, help="solver name")
    parser.add_argument("-native", action="store_true",
                        help="use the native C++ SMT connection")
    parser.add_argument("-no-meta", help="no metaInterpreter", action="store_true")
    args = parser.parse_args()

    try:
        if args.file is None:
            raise ValueError("should provide an input Maude file")

        # load configurations
        cfg = load_config(os.path.dirname(__file__))
        check_config(cfg)

        user_cfg = load_user_config(args)
        cfg = update_config(cfg, user_cfg)
        check_config(cfg)

        s = cfg["solver"]
        if not args.native and s in SOLVERS:
            ready, detail = check_solver(s)
            if not ready:
                raise RuntimeError(
                    "{} solver unavailable: {}. Run: maude-se-installer install {}".format(
                        s, detail, s
                    )
                )

        # instantiate our interface
        setSmtSolver(s)
        factory = Factory()

        if args.native:
            factory.install_native(s)
        else:
            s_def = cfg["solver-def"][s]
            conv = load_class_from_file(s_def["converter"]["dir"], s_def["converter"]["name"])
            conn = load_class_from_file(s_def["connector"]["dir"], s_def["connector"]["name"])
            factory.register(s, conv, conn)
            factory.install(s)

        # initialize Maude interpreter
        init(advise=False)
        
        # load an input file
        load(args.file)

        if cfg["no-meta"] == False:
            load('smt-check.maude')
            load('maude-se-meta.maude')

    except Exception as err:
        print("error: {}".format(err))
        return 1
