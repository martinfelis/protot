#include "ObjectInterfacePerModule.h"
#include "IObject.h"

#include "rcpp/IUpdateable.h"
#include "rcpp/InterfaceIds.h"
#include <iostream>


class SceneObject : public TInterface<IID_IUPDATEABLE,IUpdateable>
{
public:
	virtual void Update( float deltaTime )
	{
		std::cout << "Runtime Object 23 update called!\n";
	}
};

REGISTERCLASS(SceneObject);
