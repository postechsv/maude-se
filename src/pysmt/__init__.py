import argparse
from maudeSE import *
from maudeSE.factory import *
from maudeSE.util import *

def main():
    
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file', nargs='?', type=str, help="input Maude file")
    parser.add_argument("-cfg", "-config", metavar="CONFIG", type=str, 
                        help="a directory to a configuration file (default: \"config.yml\")")
    parser.add_argument("-s", "-solver", metavar="SOLVER", type=str, help="solver name")
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

        # instantiate our interface
        setSmtSolver(s)
        factory = Factory().__disown__()

        s_def = cfg["solver-def"][s]

        conv = load_class_from_file(s_def["converter"]["dir"], s_def["converter"]["name"])
        conn = load_class_from_file(s_def["connector"]["dir"], s_def["connector"]["name"])

        factory.register(s, conv, conn)

        setSmtManagerFactory(factory)

        # initialize Maude interpreter
        init(advise=False)
        
        # load an input file
        load(args.file)

        if cfg["no-meta"] == False:
            load('smt-check.maude')
            load('maude-se-meta.maude')

    except Exception as err:
        print("error: {}".format(err))