//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

/**
 * \file OffMeshConnectionTool.h
 *
 * \date 2016-03-16
 * \author Mikko Mononen (modified by consultit)
 */

#ifndef OFFMESHCONNECTIONTOOL_H
#define OFFMESHCONNECTIONTOOL_H

#include "NavMeshType.h"

namespace rnsup
{

// Tool to create off-mesh connection for InputGeom

class OffMeshConnectionTool : public NavMeshTypeTool
{
	NavMeshType* m_sample;
	float m_hitPos[3];
	bool m_hitPosSet;
	bool m_bidir;
	unsigned char m_oldFlags;
	
public:
	OffMeshConnectionTool();
	~OffMeshConnectionTool();
	
	virtual int type() { return TOOL_OFFMESH_CONNECTION; }
	virtual void init(NavMeshType* sample);
	virtual void reset();
//	virtual void handleMenu();
	virtual void handleClick(const float* s, const float* p, bool shift);
	virtual void handleToggle();
	virtual void handleStep();
	virtual void handleUpdate(const float dt);
	virtual void handleRender(duDebugDraw& dd);
//	virtual void handleRenderOverlay(double* proj, double* model, int* view);

	void setBidir(bool bidir) { m_bidir = bidir; }
};

} // namespace rnsup

#endif // OFFMESHCONNECTIONTOOL_H