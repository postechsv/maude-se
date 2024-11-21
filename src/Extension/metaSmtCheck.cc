#include "SMT_Info.hh"
#include "variableGenerator.hh"

bool
MetaLevelSmtOpSymbol::metaSmtCheck(FreeDagNode* subject, RewritingContext& context)
{
  if (ImportModule* m = metaLevel->downModule(subject->getArgument(0)))
    {
      if (Term* term = metaLevel->downTerm(subject->getArgument(1), m))
	{
	  m->protect();
	  term = term->normalize(false);
	  DagNode* d = term->term2Dag();

	  const SMT_Info& smtInfo = m->getSMT_Info();
	  VariableGenerator vg(smtInfo);
	  VariableGenerator::Result result = vg.checkDag(d);
	  switch (result)
	    {
	    case VariableGenerator::BAD_DAG:
	      {
		IssueAdvisory("term " << QUOTE(term) << " is not a valid SMT Boolean expression.");
		break;
	      }
	    case VariableGenerator::SAT_UNKNOWN:
	      {
		IssueAdvisory("sat solver could not determined satisfiability of " << QUOTE(term) << ".");
		break;
	      }
	    case VariableGenerator::UNSAT:
	    case VariableGenerator::SAT:
	      {
		DagNode* r = metaLevel->upBool(result == VariableGenerator::SAT);
		term->deepSelfDestruct();
		(void) m->unprotect();
		return context.builtInReplace(subject, r);
	      }
	    }

	  term->deepSelfDestruct();
	  (void) m->unprotect();
	}
    }
  return false;
}
