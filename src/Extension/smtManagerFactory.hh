#ifndef SMT_MANAGER_FACTORY_HH
#define SMT_MANAGER_FACTORY_HH

// forward declaration
class SmtManager;
class SMT_Info;

class SmtManagerFactoryInterface
{
public:
    virtual SmtManager* create(const SMT_Info& smtInfo) = 0;
};

#endif
