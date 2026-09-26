//
//	Interface to Maude terms and operations
//

%{
#include "rewriteSmtSequenceSearch.hh"
#include "rewriteSmtSearchFactory.hh"
#include "userLevelRewritingContext.hh"
#include "mixfixModule.hh"
#include <memory>
#include <stdexcept>
%}


/**
 * An iterator through the solutions of a search.
 */
class RewriteSmtSequenceSearch {
public:
	RewriteSmtSequenceSearch() = delete;

	%newobject getSubstitution;
	%newobject getStateTerm;
	%newobject getStateConstraint;
	%newobject getFinalConstraint;
	%newobject __next;

	%extend {
		/**
		 * Get the number of rewrites until this term has been found.
		 */
		int getRewriteCount() {
			return $self->getContext()->getTotalCount();
		}

		/**
		 * Get the matching substitution of the solution into the pattern.
		 */
		EasySubstitution* getSubstitution() {
			if (!$self->hasCurrentMatch()) return nullptr;
			return new EasySubstitution($self->getSubstitution(),
						    $self->getGoal(),
						    nullptr);
		}

		/**
		 * Get the rule leading to the given state.
		 * 
		 * @param stateNr The number of a state in the search graph
		 * or -1 for the current one.
		 */
		Rule* getRule(int stateNr = -1) {
			if (stateNr == -1) {
				if (!$self->hasCurrentMatch()) return nullptr;
				stateNr = $self->getStateNr();
			}
			return stateNr > 0 && stateNr < $self->getNrStates()
				? $self->getStateRule(stateNr) : nullptr;
		}

		/**
		 * Get the term of a given state.
		 * 
		 * @param stateNr The number of a state in the search graph.
		 */
		EasyTerm* getStateTerm(int stateNr) {
			return stateNr >= 0 && stateNr < $self->getNrStates()
				? new EasyTerm($self->getStateDag(stateNr)) : nullptr;
		}

		EasyTerm* getStateConstraint(int stateNr) {
			if (stateNr < 0 || stateNr >= $self->getNrStates()) return nullptr;
			DagHandle constraint = $self->getStateConstDag(stateNr);
			return constraint ? new EasyTerm(constraint.get()) : nullptr;
		}

		EasyTerm* getFinalConstraint() {
			if (!$self->hasCurrentMatch()) return nullptr;
			DagHandle constraint = $self->getFinalConstraint();
			return constraint ? new EasyTerm(constraint.get()) : nullptr;
		}


		/**
		 * Get the next match.
		 *
		 * @return A term or a null pointer if there is no more matches.
		 */
		EasyTerm* __next() {
			bool hasNext = $self->findNextMatch();
			return hasNext ? new EasyTerm($self->getStateDag($self->getStateNr())) : nullptr;
		}

		int getStateNr() const {
			return $self->hasCurrentMatch() ? $self->getStateNr() : -1;
		}

		int getStateParent(int stateNr) const {
			return stateNr >= 0 && stateNr < $self->getNrStates()
				? $self->getStateParent(stateNr) : -1;
		}
	}

	/** Whether a successful match is available to the result getters. */
	bool hasCurrentMatch() const;
	bool isSmtUnknown() const;
	bool hasInvalidRewriteResult() const;

	%unprotectDestructor(RewriteSmtSequenceSearch);
};

%extend RewriteSmtSequenceSearch {
%pythoncode %{
	def __iter__(self):
		return self

	def __next__(self):
		term = self.__next()
		if term is None:
			if self.hasInvalidRewriteResult():
				raise RuntimeError("invalid symbolic rewrite result")
			if self.isSmtUnknown():
				raise RuntimeError("SMT solver returned unknown")
			raise StopIteration
		return term
%}
}

%newobject EasyTerm::smtSearch;
%feature("kwargs") EasyTerm::smtSearch;
%exception EasyTerm::smtSearch {
	try { $action }
	catch (const std::exception &error) {
		SWIG_exception(SWIG_RuntimeError, error.what());
	}
}

%extend EasyTerm {
	/**
	 * Invoke the Maude-SE core search engine on this module and term.
	 * Unlike metaSmtSearch, this does not transform source rules or abstract
	 * SMT subterms; callers needing source-level semantics must use the meta
	 * interface until a high-level term API provides that preparation.
	 * smtGoal is the initial Boolean SMT constraint; condition constrains the
	 * target pattern as in Term.search(). The returned search owns its Maude
	 * context and can be iterated for matching state terms. After each match,
	 * getFinalConstraint() and getSubstitution() describe that solution.
	 */
	RewriteSmtSequenceSearch* smtSearch(
		SearchType type, EasyTerm* target, EasyTerm* smtGoal,
		const Vector<ConditionFragment*>& condition = EasyTerm::NO_CONDITION,
		int depth = -1, bool fold = false, bool merge = false,
		const char* logic = "QF_LRA") {
		if (!target || !smtGoal)
			throw std::invalid_argument("target and smtGoal are required");
		if (depth < -1 || type == NORMAL_FORM || type == BRANCH)
			throw std::invalid_argument("unsupported SMT search type or depth");
		auto* module = dynamic_cast<VisibleModule*>($self->symbol()->getModule());
		if (!module || target->symbol()->getModule() != module ||
		    smtGoal->symbol()->getModule() != module)
			throw std::invalid_argument("all search terms must belong to the same module");

		std::unique_ptr<Term, void (*)(Term*)> targetTerm(
		    target->termCopy(), [](Term* term) { term->deepSelfDestruct(); });
		VariableInfo variables;
		if (module->findSMT_Symbol(targetTerm.get()) ||
		    MixfixModule::findNonlinearVariable(targetTerm.get(), variables))
			throw std::invalid_argument("target cannot contain SMT operators or nonlinear variables");

		Vector<ConditionFragment*> conditionCopy;
		ImportModule::deepCopyCondition(nullptr, condition, conditionCopy);
		std::unique_ptr<Pattern> goal(
		    new Pattern(targetTerm.release(), false, conditionCopy));
		std::unique_ptr<Pattern> constraintGoal(
		    new Pattern(smtGoal->termCopy(), false));
		std::unique_ptr<RewritingContext> context(
		    new UserLevelRewritingContext($self->getDag()));
		module->protect();
		try {
			return makeRewriteSmtSearch(
			    module, context.release(),
			    static_cast<RewriteSmtSequenceSearch::SearchType>(type),
			    goal.release(), constraintGoal.release(), logic,
			    fold, merge, depth);
		} catch (...) {
			module->unprotect();
			throw;
		}
	}
}
