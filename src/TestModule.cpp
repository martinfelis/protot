#include "ObjectInterfacePerModule.h"
#include "IObject.h"

#include "rcpp/IUpdateable.h"
#include "rcpp/InterfaceIds.h"
#include <iostream>

class TestModule : public TInterface<IID_IUPDATEABLE,IUpdateable>
{
public:
	virtual void Update( float deltaTime )
	{
		std::cout << "TestModule:  1 Runtime Object 214 update called!\n";
	}
};

REGISTERCLASS(TestModule);
