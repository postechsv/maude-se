import os
import sys
import importlib.util
import yaml
from typing import Dict
from maudeSE.maude import *

def indented_str(s: str, indent: int):
    li = [" " for _ in range(indent)]
    li.append(s)
    return "".join(li)


def id_gen():
    count = 0
    while True:
        yield count
        count += 1


def get_symbol_info(module: Module):
    if module is None:
        return dict()
    
    symbol_info = dict()
    symbols = module.getSymbols()
    for symbol in symbols:
        decls = symbol.getOpDeclarations()
        user_symb = str(symbol)
        for n, decl in enumerate(decls):
            meta = symbol.getMetadata(n)
            if meta is None or "smt" not in meta:
                continue

            if "euf" in meta:
                sorts = tuple([str(s) for s in decl.getDomainAndRange()])
                
                key = (user_symb, sorts)
                if key not in symbol_info:
                    symbol_info[key] = ("euf", None)
                else:
                    print(f"ambiguous symbol found during symbol info creation ({user_symb} with {symbol_info[0]}:{symbol_info[1]})")
                
            else:
                p_l = meta.split(" ")

                if len(p_l) != 2:
                    raise Exception("incorrect metadata")
                
                if p_l[0] != "smt":
                    raise Exception("incorrect metadata (should be started with \"smt\")")
                
                # theory:symbol
                ts = p_l[1].split(":")
                
                if len(ts) != 2:
                    raise Exception(f"unsupported theory and symbol metadata ({p_l[1]})")
                
                theory, symb = ts
                sorts = tuple([str(s) for s in decl.getDomainAndRange()])

                key = (user_symb, sorts)
                if key not in symbol_info:
                    symbol_info[key] = (theory, symb)
                else:
                    print(f"ambiguous symbol found during symbol info creation ({user_symb} with {symbol_info[0]}:{symbol_info[1]})")

    return symbol_info


def load_config(config_path):
    # load config file
    if not os.path.exists(config_path):
        raise ValueError("Config file \"{}\" not found".format(config_path))

    with open("{}/config.yml".format(config_path)) as f: 
        d = yaml.load(f, Loader=yaml.FullLoader)
        for sol in d["solver-def"]:            
            for wrap in ["connector", "converter"]:
                sd = d["solver-def"][sol][wrap]["dir"]
                d["solver-def"][sol][wrap]["dir"] = "{}/{}".format(config_path, sd)
        return d


def load_user_config(args):
    if args.cfg:
        # load config file
        if os.path.exists(args.cfg):
            with open(args.cfg) as f: 
                d = yaml.load(f, Loader=yaml.FullLoader)
    else:
        d = dict()

    if args.s:
        d["solver"] = args.s

    if args.no_meta:
        d["no-meta"] = True

    return d


def update_config(original, updates):
    """
    Overrides the original nested dictionary with values from the updates dictionary.
    """
    for key, value in updates.items():
        if isinstance(value, dict):
            original_value = original.get(key, {})
            if isinstance(original_value, dict):
                update_config(original_value, value)
            else:
                original[key] = value
        else:
            original[key] = value
    return original
    

def check_config(conf: Dict):
    if "solver" not in conf:
        raise ValueError("No default solver set")
    
    if conf["solver"] not in conf["solver-def"]:
        raise ValueError("unsupported solver {}".format(conf["solver"]))

    if "solver-def" not in conf:
        raise ValueError("No solver defined")

    if "solver-def" in conf:
        for sol in conf["solver-def"]:            
            wraps = ["connector", "converter"]
            for wrap in wraps:
                if wrap not in conf["solver-def"][sol]:
                    raise ValueError("No {} found in {}".format(wrap, sol))
        
            for wrap in wraps:
                nep = ["name", "dir"]
                for n in nep:
                    if n not in conf["solver-def"][sol][wrap]:
                        raise ValueError("No {} found in {}'s {}".format(n, sol, wrap))
            

def load_class_from_file(file_path, class_name):
    """
    Loads a class from a given Python file path.

    Args:
        file_path (str): The path to the Python file.
        class_name (str): The name of the class to load.

    Returns:
        type: The loaded class, or None if an error occurs.
    """
    try:
        spec = importlib.util.spec_from_file_location("module.name", file_path)
        module = importlib.util.module_from_spec(spec)
        sys.modules["module.name"] = module
        spec.loader.exec_module(module)
        return getattr(module, class_name)
    except Exception as e:
        # import traceback
        # print(traceback.print_exc())
        # print(f"Error loading class: {e}")
        # return None
        raise ValueError("Error loading class (reason: {})".format(e))
    

def initiate_class(file_path, class_name, *args, **kwargs):
    """
    Loads a class from a file and initiates it with given arguments.

    Args:
        file_path (str): Path to the Python file containing the class.
        class_name (str): Name of the class to initiate.
        *args: Positional arguments for the class constructor.
        **kwargs: Keyword arguments for the class constructor.

    Returns:
        object: An instance of the loaded class, or None if an error occurs.
    """
    loaded_class = load_class_from_file(file_path, class_name)
    if loaded_class:
        return loaded_class(*args, **kwargs)
    else:
        return None