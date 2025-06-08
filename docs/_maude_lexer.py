from pygments.lexer import RegexLexer
from pygments.token import Keyword, Name, Comment, String, Operator, Text

class MaudeLexer(RegexLexer):
    name = 'Maude'
    aliases = ['maude']
    filenames = ['*.maude']

    tokens = {
        'root': [
            (r'/\\|\\/', Operator),  # /\ or \/
            (r'\"', String),
            (r'\\', String),
            (r'\$', Text),
            (r'\'', Text),
            (r'\`', Text),
            (r'"([^"\\]|\\.)*"', String),
            (r'\b(omod|endom|mod|endm|sort|sorts|subsort|subsorts|op|ops|var|vars|eq|ceq|rl|crl|if|then|else|fi|including|protecting|extends|ctor|comm|assoc|nonexec|id:|fmod|endfm|reduce|smt-search|smt-search2|such that|show smt-path|check|show model|using|search|rew|red|select)\b', Keyword),
            (r'---.*$', Comment.Single),
            (r'[:=<>+*/|,.\[\](){};!@#%^&~?-]', Operator),
            (r'\s+', Text),
            (r"[A-Za-z_][\w']*|\b\d+\b", Name),
        ],
    }
