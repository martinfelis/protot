#include "RuntimeCompiledCpp/RuntimeObjectSystem/ObjectInterfacePerModule.h"
#include "RuntimeCompiledCpp/RuntimeObjectSystem/IObject.h"

#include "IUpdateable.h"
#include "InterfaceIds.h"
#include <iostream>


class SceneObject : public TInterface<IID_IUPDATEABLE,IUpdateable>
{
public:
	virtual void Update( float deltaTime )
	{
		std::cout << "Runtime Object 01231 update called!\n";
	}
};

REGISTERCLASS(SceneObject);
