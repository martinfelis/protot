#include "ObjectInterfacePerModule.h"
#include "IObject.h"

#include "rcpp/IUpdateable.h"
#include "rcpp/InterfaceIds.h"
#include <iostream>
#include <sstream>

#include <unistd.h>

#include "Renderer.h"
#include "3rdparty/ocornut-imgui/imgui.h"
#include "imgui/imgui.h"

class TestModule : public TInterface<IID_IUPDATEABLE,IUpdateable>
{
public:
	virtual void Update( float deltaTime )
	{
		bool enabled = true;
		ImGui::Begin("yoyoyo");
		if (ImGui::Button("Baem")) {
			std::cout << "Clicked on Baem!" << std::endl;
		}
		ImGui::End();
		
		std::ostringstream s;
		s << "TestModule:  2 Runtime Object 4 " << deltaTime << " update called!";

		bgfx::dbgTextPrintf(1, 20, 0x6f, s.str().c_str());

		// wait a little so that we don't hog the CPU.
		usleep (1000 * 10);
	}
};

REGISTERCLASS(TestModule);
