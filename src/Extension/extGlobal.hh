#include "smtManager.hh"

extern SmtManagerFactory* smtManagerFactory;
extern char* smtSolver;

extern void setSmtSolver(char* solver);
extern void setSmtManagerFactory(SmtManagerFactory* fac);
extern bool loadNativeSmtPlugin(const char* path, const char* solver);
extern const char* nativeSmtPluginError();
