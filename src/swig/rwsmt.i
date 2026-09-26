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
			return $self->getStateRule(stateNr == -1
				? $self->getStateNr() : stateNr);
		}

		/**
		 * Get the term of a given state.
		 * 
		 * @param stateNr The number of a state in the search graph.
		 */
		EasyTerm* getStateTerm(int stateNr) {
			return new EasyTerm($self->getStateDag(stateNr));
		}

		EasyTerm* getFinalConstraint() {
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

		/**
		 * Get the number of rewrites until this term has been found.
		 */
		SmtTerm getStateConst(int stateNr) {
			return $self->getStateConst(stateNr);
		}
	}

	/**
	 * Get an internal state number that allows reconstructing 
	 * the path to this term.
	 */
	int getStateNr() const;

	/**
	 * Get the parent state.
	 *
	 * @param stateNr The number of a state in the search graph.
	 *
	 * @return The number of the parent or -1 for the root.
	 */
	int getStateParent(int stateNr) const;
	bool isSmtUnknown() const;
	bool hasInvalidRewriteResult() const;

	/**
	 * Get the constraint of a constrained term.
	 *
	 * @param stateNr The number of a state in the search graph.
	 *
	 * @return An SMT constraint
	 */
	SmtTerm getStateConst(int stateNr);

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
	 * Search symbolically from this term using the installed Maude-SE solver.
	 * smtGoal is the initial Boolean SMT constraint; condition constrains the
	 * target pattern as in Term.search(). The returned search owns its Maude
	 * context and can be iterated for matching state terms. After each match,
	 * getFinalConstraint() and getSubstitution() describe that solution.
	 */
	RewriteSmtSequenceSearch* smtSearch(
		SearchType type, EasyTerm* target, EasyTerm* smtGoal,
		const Vector<ConditionFragment*>& condition = EasyTerm::NO_CONDITION,
		int depth = -1, bool fold = true, bool merge = false,
		const char* logic = "QF_LRA") {
		if (!target || !smtGoal)
			throw std::invalid_argument("target and smtGoal are required");
		if (depth < -1 || type == NORMAL_FORM || type == BRANCH)
			throw std::invalid_argument("unsupported SMT search type or depth");
		auto* module = dynamic_cast<VisibleModule*>($self->symbol()->getModule());
		if (!module || target->symbol()->getModule() != module ||
		    smtGoal->symbol()->getModule() != module)
			throw std::invalid_argument("all search terms must belong to the same module");

		std::unique_ptr<Term> targetTerm(target->termCopy());
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
