// #include "smtManager.hh"
#include "extGlobal.hh"

SmtManagerFactory* smtManagerFactory = nullptr;
char* smtSolver = nullptr;

void setSmtSolver(char* solver){
    smtSolver = solver;
}

void setSmtManagerFactory(SmtManagerFactory* fac){
    smtManagerFactory = fac;    
}