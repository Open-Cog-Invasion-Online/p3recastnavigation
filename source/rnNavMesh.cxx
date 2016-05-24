/**
 * \file rnNavMesh.cpp
 *
 * \date 2016-03-16
 * \author consultit
 */

#include "rnNavMesh.h"

#include "rnCrowdAgent.h"
#include "rnNavMeshManager.h"
#include "camera.h"

#ifndef CPPPARSER
#include "library/DetourCommon.h"
#include "support/ConvexVolumeTool.h"
#include "support/NavMeshType_Solo.h"
#include "support/OffMeshConnectionTool.h"
#endif //CPPPARSER

/**
 *
 */
RNNavMesh::RNNavMesh(const string& name):PandaNode(name)
{
	do_reset();
}

/**
 *
 */
RNNavMesh::~RNNavMesh()
{
}

/**
 * Sets the underlying NavMeshType type: SOLO, TILE, OBSTACLES.
 * Can be set only before RNNavMesh is setup.
 */
void RNNavMesh::set_nav_mesh_type_enum(RNNavMeshTypeEnum typeEnum)
{
	nassertv_always(! mNavMeshType)

	mNavMeshTypeEnum = typeEnum;
}

/**
 * Sets up the underlying NavMeshType settings.
 */
void RNNavMesh::set_nav_mesh_settings(const RNNavMeshSettings& settings)
{
	mNavMeshSettings = settings;
	if(mNavMeshType)
	{
		//set navigation mesh settings
		mNavMeshType->setNavMeshSettings(mNavMeshSettings);
	}
}

/**
 * Sets the Recast area's traversal cost.
 */
void RNNavMesh::set_crowd_area_cost(int area, float cost)
{
	//add area with corresponding cost
	mPolyAreaCost[area] = cost;

	if(mNavMeshType)
	{
		//there is a crowd tool because the recast nav mesh
		//has been completely setup
		rnsup::CrowdTool* crowdTool =
				static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
		//set recast areas' costs
		dtQueryFilter* filter =
				crowdTool->getState()->getCrowd()->getEditableFilter(0);
		rnsup::NavMeshPolyAreaCost::const_iterator iterAC;
		for (iterAC = mPolyAreaCost.begin(); iterAC != mPolyAreaCost.end();
				++iterAC)
		{
			filter->setAreaCost((*iterAC).first, (*iterAC).second);
		}
	}
}

/**
 * Sets Recast Crowd include flags.
 * \note oredFlags: ored flags as specified in RNNavMeshPolyFlagsEnum.
 */
void RNNavMesh::set_crowd_include_flags(int oredFlags)
{
	mCrowdIncludeFlags = oredFlags;

	if(mNavMeshType)
	{
		//there is a crowd tool because the recast nav mesh
		//has been completely setup
		rnsup::CrowdTool* crowdTool =
				static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
		//set recast crowd include
		crowdTool->getState()->getCrowd()->getEditableFilter(0)->setIncludeFlags(
				mCrowdIncludeFlags);
	}
}

/**
 * Set Recast Crowd exclude flags.
 * \note oredFlags: ored flags as specified in RNNavMeshPolyFlagsEnum.
 */
void RNNavMesh::set_crowd_exclude_flags(int oredFlags)
{
	mCrowdExcludeFlags = oredFlags;

	if(mNavMeshType)
	{
		//there is a crowd tool because the recast nav mesh
		//has been completely setup
		rnsup::CrowdTool* crowdTool =
				static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
		//set recast crowd exclude flags
		crowdTool->getState()->getCrowd()->getEditableFilter(0)->setExcludeFlags(
				mCrowdExcludeFlags);
	}
}

/**
 * Sets up the underlying NavMeshType tile settings (only TILE and OBSTACLE).
 */
void RNNavMesh::set_nav_mesh_tile_settings(
		const RNNavMeshTileSettings& settings)
{
	mNavMeshTileSettings = settings;
	if (mNavMeshType)
	{
		//set navigation mesh tile settings
		if (mNavMeshTypeEnum == TILE)
		{
			static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->setTileSettings(
					mNavMeshTileSettings);
		}
		else if (mNavMeshTypeEnum == OBSTACLE)
		{
			static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->setTileSettings(
					mNavMeshTileSettings);
		}
	}
}

/**
 * Initialize the RNNavMesh with common settings.
 */
void RNNavMesh::do_initialize()
{
	WPT(RNNavMeshManager) mTmpl = RNNavMeshManager::get_global_ptr();
	//
	float value;
	int valueInt;
	string valueStr;
	//set RNNavMesh parameters (store internally for future use)
	//cell size
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("cell_size")).c_str(), NULL);
	mNavMeshSettings.set_cellSize(value >= 0.0 ? value : -value);
	//cell height
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("cell_height")).c_str(), NULL);
	mNavMeshSettings.set_cellHeight(value >= 0.0 ? value : -value);
	//agent height
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("agent_height")).c_str(), NULL);
	mNavMeshSettings.set_agentHeight(value >= 0.0 ? value : -value);
	//agent radius
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("agent_radius")).c_str(), NULL);
	mNavMeshSettings.set_agentRadius(value >= 0.0 ? value : -value);
	//agent max climb
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("agent_max_climb")).c_str(),
			NULL);
	mNavMeshSettings.set_agentMaxClimb(value >= 0.0 ? value : -value);
	//agent max slope
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("agent_max_slope")).c_str(),
			NULL);
	mNavMeshSettings.set_agentMaxSlope(value >= 0.0 ? value : -value);
	//region min size
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("region_min_size")).c_str(),
			NULL);
	mNavMeshSettings.set_regionMinSize(value >= 0.0 ? value : -value);
	//region merge size
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("region_merge_size")).c_str(),
			NULL);
	mNavMeshSettings.set_regionMergeSize(value >= 0.0 ? value : -value);
	//partition type
	valueStr = mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
			string("partition_type"));
	if (valueStr == string("monotone"))
	{
		mNavMeshSettings.set_partitionType(rnsup::NAVMESH_PARTITION_MONOTONE);
	}
	else if (valueStr == string("layer"))
	{
		mNavMeshSettings.set_partitionType(rnsup::NAVMESH_PARTITION_LAYERS);
	}
	else
	{
		//watershed
		mNavMeshSettings.set_partitionType(rnsup::NAVMESH_PARTITION_WATERSHED);
	}
	//edge max len
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("edge_max_len")).c_str(), NULL);
	mNavMeshSettings.set_edgeMaxLen(value >= 0.0 ? value : -value);
	//edge max error
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("edge_max_error")).c_str(),
			NULL);
	mNavMeshSettings.set_edgeMaxError(value >= 0.0 ? value : -value);
	//verts per poly
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("verts_per_poly")).c_str(),
			NULL);
	mNavMeshSettings.set_vertsPerPoly(value >= 0.0 ? value : -value);
	//detail sample dist
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("detail_sample_dist")).c_str(),
			NULL);
	mNavMeshSettings.set_detailSampleDist(value >= 0.0 ? value : -value);
	//detail sample max error
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("detail_sample_max_error")).c_str(),
			NULL);
	mNavMeshSettings.set_detailSampleMaxError(value >= 0.0 ? value : -value);
	//build all tiles
	mNavMeshTileSettings.set_buildAllTiles(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("build_all_tiles")) == string("true") ?
					true : false);
	//max tiles
	valueInt = strtol(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("max_tiles")).c_str(), NULL, 0);
	mNavMeshTileSettings.set_maxTiles(valueInt >= 0 ? valueInt : -valueInt);
	//max polys per tile
	valueInt = strtol(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("max_polys_per_tile")).c_str(), NULL, 0);
	mNavMeshTileSettings.set_maxPolysPerTile(
			valueInt >= 0 ? valueInt : -valueInt);
	//tile size
	value = STRTOF(
			mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
					string("tile_size")).c_str(), NULL);
	mNavMeshTileSettings.set_tileSize(value >= 0.0 ? value : -value);
	///
	//0: get navmesh type
	valueStr = mTmpl->get_parameter_value(RNNavMeshManager::NAVMESH,
			string("navmesh_type"));
	if (valueStr == string("tile"))
	{
		mNavMeshTypeEnum = TILE;
	}
	else if (valueStr == string("obstacle"))
	{
		mNavMeshTypeEnum = OBSTACLE;
	}
	else
	{
		mNavMeshTypeEnum = SOLO;
	}
	//get string parameters
	//1: get the input from xml
	//area-flags-cost settings
	plist<string> mAreaFlagsCostParam = mTmpl->get_parameter_values(RNNavMeshManager::NAVMESH,
			string("area_flags_cost"));
	plist<string>::iterator iterStr;

	///get area ability flags and cost (for crowd)
	mPolyAreaFlags.clear();
	mPolyAreaCost.clear();
	for (iterStr = mAreaFlagsCostParam.begin();
			iterStr != mAreaFlagsCostParam.end(); ++iterStr)
	{
		//any "area_flags" string is a "compound" one, i.e. could have the form:
		// "area@flag1|flag2...|flagN@cost"
		pvector<string> areaFlagsCostStr = parseCompoundString(
				*iterStr, '@');
		//check only if there is a triple
		if (areaFlagsCostStr.size() == 3)
		{
			//default area: NAVMESH_POLYAREA_GROUND (== 0)
			int area = (
					! areaFlagsCostStr[0].empty() ?
							strtol(areaFlagsCostStr[0].c_str(), NULL, 0) :
							rnsup::NAVMESH_POLYAREA_GROUND);
			//iterate over flags
			pvector<string> flags = parseCompoundString(
					areaFlagsCostStr[1], ':');
			pvector<string>::const_iterator iterF;
			//default flag: NAVMESH_POLYFLAGS_WALK (== 0x01)
			int oredFlags = (flags.size() == 0 ? 0x01 : 0x0);
			for (iterF = flags.begin(); iterF != flags.end(); ++iterF)
			{
				int flag = (
						! (*iterF).empty() ?
								strtol((*iterF).c_str(), NULL, 0) :
								rnsup::NAVMESH_POLYFLAGS_WALK);
				//or flag
				oredFlags |= flag;
			}
			//add area with corresponding ored ability flags
			mPolyAreaFlags[area] = oredFlags;

			//default cost: 1.0
			float cost = STRTOF(areaFlagsCostStr[2].c_str(), NULL);
			if (cost <= 0.0)
			{
				cost = 1.0;
			}
			//add area with corresponding cost
			mPolyAreaCost[area] = cost;
		}
	}

	///get crowd include & exclude flags
	pvector<string>::const_iterator iterIEFStr;
	pvector<string> ieFlagsStr;
	//1:iterate over include flags
	string mCrowdIncludeFlagsParam = mTmpl->get_parameter_value(
			RNNavMeshManager::NAVMESH, string("crowd_include_flags"));
	ieFlagsStr = parseCompoundString(mCrowdIncludeFlagsParam, ':');
	//default include flag: NAVMESH_POLYFLAGS_WALK (== 0x01)
	mCrowdIncludeFlags = (ieFlagsStr.empty() ? rnsup::NAVMESH_POLYFLAGS_WALK : 0x0);
	for (iterIEFStr = ieFlagsStr.begin(); iterIEFStr != ieFlagsStr.end();
			++iterIEFStr)
	{
		int flag = (
				! (*iterIEFStr).empty() ?
						strtol((*iterIEFStr).c_str(), NULL, 0) :
						rnsup::NAVMESH_POLYFLAGS_WALK);
		//or flag
		mCrowdIncludeFlags |= flag;
	}
	//2:iterate over exclude flags
	string mCrowdExcludeFlagsParam = mTmpl->get_parameter_value(
			RNNavMeshManager::NAVMESH, string("crowd_exclude_flags"));
	ieFlagsStr = parseCompoundString(mCrowdExcludeFlagsParam, ':');
	//default exclude flag: NAVMESH_POLYFLAGS_DISABLED (== 0x10)
	mCrowdExcludeFlags = (ieFlagsStr.empty() ? rnsup::NAVMESH_POLYFLAGS_DISABLED : 0x0);
	for (iterIEFStr = ieFlagsStr.begin(); iterIEFStr != ieFlagsStr.end();
			++iterIEFStr)
	{
		int flag = (
				! (*iterIEFStr).empty() ?
						strtol((*iterIEFStr).c_str(), NULL, 0) :
						rnsup::NAVMESH_POLYFLAGS_DISABLED);
		//or flag
		mCrowdExcludeFlags |= flag;
	}

	///get convex volumes
	plist<string> mConvexVolumesParam = mTmpl->get_parameter_values(RNNavMeshManager::NAVMESH,
			string("convex_volume"));
	mConvexVolumes.clear();
	for (iterStr = mConvexVolumesParam.begin();
			iterStr != mConvexVolumesParam.end(); ++iterStr)
	{
		//any "convex_volume" string is a "compound" one, i.e. could have the form:
		// "x1,y1,z1:x2,y2,z2...:xN,yN,zN@area_type"
		pvector<string> pointsAreaTypeStr = parseCompoundString(
				*iterStr, '@');
		//check only if there is (at least) a pair
		if (pointsAreaTypeStr.size() < 2)
		{
			continue;
		}
		//an empty point sequence is ignored
		if (pointsAreaTypeStr[0].empty())
		{
			continue;
		}
		//
		int areaType = (
				! pointsAreaTypeStr[1].empty() ?
						strtol(pointsAreaTypeStr[1].c_str(), NULL, 0) :
						rnsup::NAVMESH_POLYAREA_GROUND);
		//iterate over points
		ValueList<LPoint3f> pointList;
		pvector<string> pointsStr = parseCompoundString(
				pointsAreaTypeStr[0], ':');
		pvector<string>::const_iterator iterP;
		for (iterP = pointsStr.begin(); iterP != pointsStr.end(); ++iterP)
		{
			pvector<string> posStr = parseCompoundString(*iterP, ',');
			LPoint3f point = LPoint3f::zero();
			for (unsigned int i = 0; (i < 3) && (i < posStr.size()); ++i)
			{
				point[i] = STRTOF(posStr[i].c_str(), NULL);
			}
			//insert the point to the list
			pointList.add_value(point);
		}
		//insert convex volume to the list
		RNConvexVolumeSettings settings;
		settings.set_area(areaType);
		settings.set_flags(mPolyAreaFlags[areaType]);
		int ref = unique_ref();
		settings.set_ref(ref);
		mConvexVolumes.push_back(PointListConvexVolumeSettings(pointList, settings));
	}

	///get off mesh connections
	plist<string> mOffMeshConnectionsParam = mTmpl->get_parameter_values(
			RNNavMeshManager::NAVMESH, string("offmesh_connection"));
	mOffMeshConnections.clear();
	for (iterStr = mOffMeshConnectionsParam.begin();
			iterStr != mOffMeshConnectionsParam.end(); ++iterStr)
	{
		//any "offmesh_connection" string is a "compound" one, i.e. has the form:
		// "xB,yB,zB:xE,yE,zE@bidirectional", with bidirectional=true by default.
		pvector<string> pointPairBidirStr = parseCompoundString(
				*iterStr, '@');
		//check only if there is (at least) a pair
		if (pointPairBidirStr.size() < 2)
		{
			continue;
		}
		//an empty point pair is ignored
		if (pointPairBidirStr[0].empty())
		{
			continue;
		}
		bool bidir = (
				string(pointPairBidirStr[1]) == string("false") ?
						false : true);
		//iterate over the first 2 points
		//each point defaults to LPoint3f::zero()
		ValueList<LPoint3f> pointPair;
		pvector<string> pointsStr = parseCompoundString(
				pointPairBidirStr[0], ':');
		pvector<string>::const_iterator iterPStr;
		int k;
		for (k = 0, iterPStr = pointsStr.begin();
				k < 2 && (iterPStr != pointsStr.end()); ++k, ++iterPStr)
		{
			pvector<string> posStr = parseCompoundString(*iterPStr,
					',');
			LPoint3f point = LPoint3f::zero();
			for (unsigned int i = 0; (i < 3) && (i < posStr.size()); ++i)
			{
				point[i] = STRTOF(posStr[i].c_str(), NULL);
			}
			//insert the point to the pair
			switch (k)
			{
			case 0:
				pointPair.add_value(point);
				break;
			case 1:
				pointPair.add_value(point);
				break;
			default:
				break;
			}
		}
		//insert off mesh connection to the list
		RNOffMeshConnectionSettings settings;
		settings.set_rad(mNavMeshSettings.get_agentRadius());
		settings.set_bidir(bidir);
		settings.set_area(POLYAREA_JUMP);
		settings.set_flags(POLYFLAGS_JUMP);
		int ref = unique_ref();
		settings.set_ref(ref);
		mOffMeshConnections.push_back(PointPairOffMeshConnectionSettings(pointPair, settings));
	}

	//2: add settings for nav mesh
	//set nav mesh type enum: already done
	//set mov type: already done
	//set nav mesh settings: already done
	//set nav mesh tile settings: already done
	//set reference node path
	mReferenceNP = NodePath::any_path(this);
	//
#ifdef RN_DEBUG
	// un-setup debug node path
	NodePath debugNodePathUnsetup = mReferenceNP.attach_new_node(
				string("DebugNodePathUsetup") + get_name());
	//no collide mask for all their children
	debugNodePathUnsetup.set_collide_mask(BitMask32::all_off());
	// enable un-setup debug drawing
	mDDUnsetup = new rnsup::DebugDrawPanda3d(debugNodePathUnsetup);
	do_debug_static_render_unsetup();
#endif //RN_DEBUG
}

/**
 * Sets up RNNavMesh to be ready for RNCrowdAgents handling.
 * This method can be called at startup or during program execution
 * repeatedly.
 */
int RNNavMesh::setup()
{
	// go on if nav mesh has not been setup yet
	nassertr_always(!mNavMeshType, RN_SUCCESS)

	// go on if owner object not empty
	nassertr_always(!mOwnerObject.is_empty(), RN_NAVMESH_NULL)

	set_name(mOwnerObject.get_name() + string("_RNNavMesh"));

	//detach any old child node path: owner, crowd agents, obstacles
	NodePathCollection children = NodePath::any_path(this).get_children();
	for (int i; i < children.size(); ++i)
	{
		children[i].detach_node();
	}

	//reparent mOwnerObject to the reference node path
	mOwnerObject.reparent_to(mReferenceNP);

	//setup the build context
	mCtx = new rnsup::BuildContext;

	//do the real setup
	//setup navigation mesh, otherwise the same
	//operations must be performed by program.
	PRINT_DEBUG("RNNavMesh::setup");

	//don't load model mesh more than once
	if (!mGeom)
	{
		//load the mesh from the owner node path
		if (!do_load_model_mesh(mOwnerObject))
		{
			cleanup();
			return RN_ERROR;
		}
	}

	//set up the type of navigation mesh
	switch (mNavMeshTypeEnum)
	{
	case SOLO:
	{
		do_create_nav_mesh_type(new rnsup::NavMeshType_Solo());
		//set navigation mesh settings
		mNavMeshType->setNavMeshSettings(mNavMeshSettings);
	}
		break;
	case TILE:
	{
		do_create_nav_mesh_type(new rnsup::NavMeshType_Tile());
		//set navigation mesh settings
		mNavMeshType->setNavMeshSettings(mNavMeshSettings);
		//set navigation mesh tile settings
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->setTileSettings(
				mNavMeshTileSettings);
	}
		break;
	case OBSTACLE:
	{
		do_create_nav_mesh_type(new rnsup::NavMeshType_Obstacle());
		//set navigation mesh settings
		mNavMeshType->setNavMeshSettings(mNavMeshSettings);
		//set navigation mesh tile settings...
		//	evaluate m_maxTiles & m_maxPolysPerTile from m_tileSize
		const float* bmin = get_recast_input_geom()->getMeshBoundsMin();
		const float* bmax = get_recast_input_geom()->getMeshBoundsMax();
		int gw = 0, gh = 0;
		rcCalcGridSize(bmin, bmax, mNavMeshSettings.get_cellSize(), &gw, &gh);
		const int ts = (int) mNavMeshTileSettings.get_tileSize();
		const int tw = (gw + ts - 1) / ts;
		const int th = (gh + ts - 1) / ts;
		//	Max tiles and max polys affect how the tile IDs are calculated.
		// 	There are 22 bits available for identifying a tile and a polygon.
		int tileBits = rcMin((int) rnsup::ilog2(rnsup::nextPow2(tw * th)), 14);
		if (tileBits > 14)
		{
			tileBits = 14;
		}
		int polyBits = 22 - tileBits;
		mNavMeshTileSettings.set_maxTiles(1 << tileBits);
		mNavMeshTileSettings.set_maxPolysPerTile(1 << polyBits);
		//...effectively
		static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->setTileSettings(
				mNavMeshTileSettings);
	}
		break;
	default:
		break;
	}

	//set recast areas' flags table
	mNavMeshType->setFlagsAreaTable(mPolyAreaFlags);

	{
		//set recast convex volumes
		//mConvexVolumes could be modified during iteration so use this pattern:
		rnsup::ConvexVolumeTool* tool = new rnsup::ConvexVolumeTool();
		mNavMeshType->setTool(tool);
		pvector<PointListConvexVolumeSettings>::iterator iter =
				mConvexVolumes.begin();
		while (iter != mConvexVolumes.end())
		{
			ValueList<LPoint3f>& points = iter->first();
			//check if there are at least 3 points for
			//complete a convex volume
			if (points.size() < 3)
			{
				continue;
			}
			int area = iter->second().get_area();
			if (area < 0)
			{
				area = rnsup::NAVMESH_POLYAREA_GROUND;
			}

			//1: set recast area type
			tool->setAreaType(area);

			//2: iterate, compute and insert recast points
			float recastPos[3];
			for (int iterP = 0; iterP != points.size(); ++iterP)
			{
				//point is given wrt mOwnerObject node path but
				//it has to be wrt mReferenceNP
				LPoint3f refPos = mReferenceNP.get_relative_point(mOwnerObject,
						points[iterP]);
				//insert convex volume point
				rnsup::LVecBase3fToRecast(refPos, recastPos);
				mNavMeshType->getTool()->handleClick(NULL, recastPos, false);
			}
			//re-insert the last point (to close convex volume)
			mNavMeshType->getTool()->handleClick(NULL, recastPos, false);
			//checks if really it was inserted
			int idx = tool->getConvexVolumeIdx();
			if (idx != -1)
			{
				//inserted
				//now make sure mConvexVolumes and mGeom::m_volumes are synchronized
				points.clear();
				rnsup::ConvexVolume convexVol = mGeom->getConvexVolumes()[idx];
				for (int i = 0; i < convexVol.nverts; ++i)
				{
					points.add_value(
							rnsup::RecastToLVecBase3f(&convexVol.verts[i * 3]));
				}
				//mConvexVolumes and mGeom::m_volumes have the same order
				nassertr_always(mGeom->getConvexVolumeCount() - 1 == idx,
						RN_ERROR)
				nassertr_always((iter - mConvexVolumes.begin()) == idx,
						RN_ERROR)

				mConvexVolumes[idx].set_first(points);
			}
			else
			{
				//not inserted: remove
				//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				iter = mConvexVolumes.erase(iter);
				continue;
			}
			//increment iterator
			++iter;
		}
		mNavMeshType->setTool(NULL);
	}

	{
		//set recast off mesh connections
		rnsup::OffMeshConnectionTool* tool = new rnsup::OffMeshConnectionTool();
		mNavMeshType->setTool(tool);
		pvector<PointPairOffMeshConnectionSettings>::iterator iter =
				mOffMeshConnections.begin();
		while (iter != mOffMeshConnections.end())
		{
			ValueList<LPoint3f>& pointPair = iter->first();
			bool bidir = iter->second().get_bidir();

			//1: set recast connection bidir
			tool->setBidir(bidir);

			//2: iterate, compute and insert recast points
			//points are given wrt mOwnerObject node path but
			//they have to be wrt mReferenceNP
			float recastPos[3];
			LPoint3f refPos;
			//compute and insert first recast point
			refPos = mReferenceNP.get_relative_point(mOwnerObject,
					pointPair[0]);
			rnsup::LVecBase3fToRecast(refPos, recastPos);
			mNavMeshType->getTool()->handleClick(NULL, recastPos, false);
			//compute and insert second recast point
			refPos = mReferenceNP.get_relative_point(mOwnerObject,
					pointPair[1]);
			rnsup::LVecBase3fToRecast(refPos, recastPos);
			mNavMeshType->getTool()->handleClick(NULL, recastPos, false);
			//checks if really it was inserted
			int idx = tool->getOffMeshConnectionIdx();
			if (idx != -1)
			{
				//inserted
				//now make sure mOffMeshConnections and mGeom's off mesh connections are synchronized
				pointPair.clear();
				const float* v = &(mGeom->getOffMeshConnectionVerts()[idx * 3 * 2]);
				//spos
				pointPair.add_value(rnsup::RecastToLVecBase3f(&v[0]));
				//epos
				pointPair.add_value(rnsup::RecastToLVecBase3f(&v[3]));
				//mOffMeshConnections and mGeom's off mesh connections have the same order
				nassertr_always(mGeom->getOffMeshConnectionCount() - 1 == idx,
						RN_ERROR)
				nassertr_always((iter - mOffMeshConnections.begin()) == idx,
						RN_ERROR)

				mOffMeshConnections[idx].set_first(pointPair);
			}
			else
			{
				//not inserted: remove
				//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				iter = mOffMeshConnections.erase(iter);
				continue;
			}
			//increment iterator
			++iter;
		}
		mNavMeshType->setTool(NULL);
	}

	///build navigation mesh actually
	do_build_navMesh();

	//set crowd tool: this will be always on when nav mesh is setup
	rnsup::CrowdTool* crowdTool = new rnsup::CrowdTool();
	mNavMeshType->setTool(crowdTool);

	{
		//set recast areas' costs
		dtQueryFilter* filter =
				crowdTool->getState()->getCrowd()->getEditableFilter(0);
		rnsup::NavMeshPolyAreaCost::const_iterator iter;
		for (iter = mPolyAreaCost.begin(); iter != mPolyAreaCost.end(); ++iter)
		{
			filter->setAreaCost((*iter).first, (*iter).second);
		}
	}

	//set recast crowd include & exclude flags
	crowdTool->getState()->getCrowd()->getEditableFilter(0)->setIncludeFlags(
			mCrowdIncludeFlags);
	crowdTool->getState()->getCrowd()->getEditableFilter(0)->setExcludeFlags(
			mCrowdExcludeFlags);

	//initialize the tester tool
	mTesterTool.init(mNavMeshType,
			crowdTool->getState()->getCrowd()->getEditableFilter(0));

	//<this code is executed only when in manual setup:
	{
		//add to recast previously added CrowdAgents.
		//mCrowdAgents could be modified during iteration so use this pattern:
		PTA(PT(RNCrowdAgent))::iterator iter = mCrowdAgents.begin();
		while (iter != mCrowdAgents.end())
		{
			PT(RNCrowdAgent)crowdAgent = *iter;
			//check if adding to recast was successful
			if (! do_add_crowd_agent_to_recast_update(crowdAgent))
			{
				//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				iter = mCrowdAgents.erase(iter);
				continue;
			}
			//increment iterator
			++iter;
		}
	}

	//handle obstacles
	if (mNavMeshTypeEnum == OBSTACLE)
	{
		//add to recast previously added obstacles.
		//mObstacles could be modified during iteration so use this pattern:
		PTA(Obstacle)::iterator iter = mObstacles.begin();
		while (iter != mObstacles.end())
		{
			//check if adding to recast was successful
			if (do_add_obstacle_to_recast((*iter).second(),
							iter - mObstacles.begin()) < 0)
			{
				//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
				iter = mObstacles.erase(iter);
				continue;
			}
			//increment iterator
			++iter;
		}
	}
	else
	{
		//remove all obstacles
		mObstacles.clear();
	}
	//>
	//
#ifdef RN_DEBUG
	// disable un-setup debug drawing
	mDDUnsetup->reset();
	delete mDDUnsetup;
	mDDUnsetup = NULL;
#endif //RN_DEBUG
	//
	return RN_SUCCESS;
}

/**
 * Builds the navigation mesh for the loaded model mesh.
 */
bool RNNavMesh::do_build_navMesh()
{
#ifdef RN_DEBUG
	mCtx->resetLog();
#endif //RN_DEBUG
	//build navigation mesh
	bool result = mNavMeshType->handleBuild();
#ifdef RN_DEBUG
	mCtx->dumpLog("Build log %s:", mMeshName.c_str());
#endif //RN_DEBUG
	return result;
}

/**
 * Adds a convex volume given its points (at least 3) and the area type.
 * Can be added only before RNNavMesh is setup.
 * Returns the convex volume's unique reference, or a negative number on error.
 * \note The added convex volume is temporary: after setup this convex volume
 * can be eliminated, so reference validity should always be verified before
 * use.
 */
int RNNavMesh::add_convex_volume(const ValueList<LPoint3f>& points,
		int area)
{
	// go on if nav mesh has not been already setup
	nassertr_always((!mNavMeshType) && (points.size() >= 3), RN_ERROR)

	// add to convex volumes
	RNConvexVolumeSettings settings;
	settings.set_area(area);
	settings.set_flags(mPolyAreaFlags[area]);
	int ref = unique_ref();
	settings.set_ref(ref);
	mConvexVolumes.push_back(PointListConvexVolumeSettings(points, settings));
#ifdef RN_DEBUG
	do_debug_static_render_unsetup();
#endif //RN_DEBUG
	return ref;
}

/**
 * Removes a convex volume given an internal point.
 * Can be removed only before RNNavMesh is setup.
 * \note The first one found volume will be removed.
 * Returns the convex volume's reference, or a negative number on error.
 */
int RNNavMesh::remove_convex_volume(const LPoint3f& insidePoint)
{
	// go on if nav mesh has not been already setup
	nassertr_always(!mNavMeshType, RN_ERROR)

	//set oldRef=-1 in case of error
	int oldRef = -1;
	///HACK: use support functionality to remove convex volumes
	//create fake InputGeom, NavMeshType and ConvexVolumeTool
	rnsup::InputGeom* geom = new rnsup::InputGeom;
	rnsup::NavMeshType* navMeshType = new rnsup::NavMeshType_Solo();
	rnsup::ConvexVolumeTool* cvTool = new rnsup::ConvexVolumeTool();
	navMeshType->handleMeshChanged(geom);
	navMeshType->setTool(cvTool);
	//cycle the existing convex volumes (see setup()) and
	//check if they can be added then removed given the insidePoint
	pvector<PointListConvexVolumeSettings>::iterator cvI;
	for (cvI = mConvexVolumes.begin(); cvI != mConvexVolumes.end(); ++cvI)
	{
		//try to add this convex volume (the NavMeshType has none)
		ValueList<LPoint3f> points = cvI->first();
		cvTool->setAreaType(cvI->second().get_area());
		float recastPos[3];
		for (int i = 0; i != points.size(); ++i)
		{
			rnsup::LVecBase3fToRecast(points[i], recastPos);
			cvTool->handleClick(NULL, recastPos, false);
		}
		cvTool->handleClick(NULL, recastPos, false);
		//check if convex volume has been actually added
		if (cvTool->getConvexVolumeIdx() == -1)
		{
			continue;
		}
		//now try to remove this convex volume
		rnsup::LVecBase3fToRecast(insidePoint, recastPos);
		cvTool->handleClick(NULL, recastPos, true);
		//check if the convex volume has been removed
		if (cvTool->getConvexVolumeIdx() != -1)
		{
			//this convex volume has been removed, i.e. insidePoint
			//is indeed "inside" it, so calculate oldRef (>=0), remove
			//the convex volume and then stop cycle
			oldRef = (*cvI).second().get_ref();
			mConvexVolumes.erase(cvI);
			break;
		}
		// remove this convex volume and continue cycle
		navMeshType->getInputGeom()->deleteConvexVolume(0);
	}
	//delete fake objects
	navMeshType->setTool(NULL);
	delete navMeshType;
	delete geom;
#ifdef RN_DEBUG
	do_debug_static_render_unsetup();
#endif //RN_DEBUG
	//return oldRef or -1
	return oldRef;
}

/**
 * Gets the convex volume with the point inside.
 * Returns the index of the convex volume, or -1 if none is found.
 */
int RNNavMesh::do_get_convex_volume_from_point(const LPoint3f& insidePoint) const
{
	//get hit point
	float hitPos[3];
	rnsup::LVecBase3fToRecast(insidePoint, hitPos);
	//check if a convex volume was hit (see: Delete case of ConvexVolumeTool::handleClick)
	int convexVolumeID = -1;
	const rnsup::ConvexVolume* vols =
			mNavMeshType->getInputGeom()->getConvexVolumes();
	for (int i = 0; i < mNavMeshType->getInputGeom()->getConvexVolumeCount();
			++i)
	{
		if (rnsup::pointInPoly(vols[i].nverts, vols[i].verts, hitPos)
				&& hitPos[1] >= vols[i].hmin && hitPos[1] <= vols[i].hmax)
		{
			convexVolumeID = i;
			break;
		}
	}
	return convexVolumeID;
}

/**
 * Finds polygons of a convex volume.
 */
int RNNavMesh::do_find_polys_of_convex_volume(int convexVolumeID,
		dtQueryFilter& filter, dtPolyRef* polys, int& npolys,
		const int MAX_POLYS) const
{
	///https://groups.google.com/forum/?fromgroups#!searchin/recastnavigation/door/recastnavigation/K2C44OCpxGE/a2Zn6nu0dIIJ
	const float *queryPolyPtr =
			mNavMeshType->getInputGeom()->getConvexVolumes()[convexVolumeID].verts;
	int nverts =
			mNavMeshType->getInputGeom()->getConvexVolumes()[convexVolumeID].nverts;

	float *queryPoly = new float[nverts * 3];

	for (int i = 0; i < nverts * 3; ++i)
	{
		queryPoly[i] = queryPolyPtr[i];
	}

	rnsup::reverseVector(queryPoly, nverts);

	float centerPos[3];
	centerPos[0] = centerPos[1] = centerPos[2] = 0;
	for (int i = 0; i < nverts; ++i)
	{
		dtVadd(centerPos, centerPos, &queryPoly[i * 3]);
	}
	dtVscale(centerPos, centerPos, 1.0f / nverts);

	filter.setIncludeFlags(POLYFLAGS_ALL);
	filter.setExcludeFlags(0);
	dtPolyRef startRef;
	float polyPickExt[3] =
	{ 2, 4, 2 };
	dtStatus status;
	status = mNavMeshType->getNavMeshQuery()->findNearestPoly(centerPos,
			polyPickExt, &filter, &startRef, 0);

	if (!(dtStatusSucceed(status)))
	{
		delete[] queryPoly;
		return RN_ERROR;
	}

	dtPolyRef parent[MAX_POLYS];
	status = mNavMeshType->getNavMeshQuery()->findPolysAroundShape(startRef,
			queryPoly, nverts, &filter, polys, parent, 0, &npolys,
			MAX_POLYS);

	delete[] queryPoly;

	nassertr_always(dtStatusSucceed(status), RN_ERROR)

	return RN_SUCCESS;
}

/**
 * Updates settings of the convex volume with the point inside.
 * Can be set only after RNNavMesh is setup.
 * Returns the index of the convex volume, or -1 if none is found or on error.
 */
int RNNavMesh::set_convex_volume_settings(const LPoint3f& insidePoint,
		const RNConvexVolumeSettings settings)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_ERROR)

	int convexVolumeID = do_get_convex_volume_from_point(insidePoint);

	// check if a convex volume was it
	if (convexVolumeID != -1)
	{
		dtQueryFilter filter;
		static const int MAX_POLYS = 256;
		dtPolyRef polys[MAX_POLYS];
		int npolys;
		dtStatus status, status2;

		nassertr_always(
				do_find_polys_of_convex_volume(convexVolumeID, filter, polys, npolys, MAX_POLYS) == RN_SUCCESS,
				RN_ERROR)

		int area = (
				settings.get_area() >= 0 ?
						settings.get_area() : -settings.get_area());
		int p;
		for (p = 0; p < npolys; ++p)
		{
			if (mNavMeshType->getNavMeshQuery()->isValidPolyRef(polys[p],
					&filter))
			{
				status = mNavMeshType->getNavMesh()->setPolyArea(polys[p],
						area);
				status2 = mNavMeshType->getNavMesh()->setPolyFlags(polys[p],
						settings.get_flags());
				if (!(dtStatusSucceed(status) && dtStatusSucceed(status2)))
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		//check if all OK
		if (p == npolys)
		{
			//mConvexVolumes and mGeom::m_volumes are synchronized
			//update mConvexVolume settings
			mConvexVolumes[convexVolumeID].set_second(settings);
		}
		else
		{
			//errors: reset to previous values
			int oldArea =
					mConvexVolumes[convexVolumeID].get_second().get_area();
			int oldFlags =
					mConvexVolumes[convexVolumeID].get_second().get_flags();
			for (int i = 0; i < p; ++i)
			{
				mNavMeshType->getNavMesh()->setPolyArea(polys[p], oldArea);
				mNavMeshType->getNavMesh()->setPolyFlags(polys[p], oldFlags);
			}
			convexVolumeID = -1;
		}
	}
#ifdef RN_DEBUG
	if (!mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//return
	return convexVolumeID;
}

/**
 * Returns settings of the convex volume given a point inside.
 * Can be get only after RNNavMesh is setup.
 * Returns RNConvexVolumeSettings::ref = -1 on error.
 */
RNConvexVolumeSettings RNNavMesh::get_convex_volume_settings(
		const LPoint3f& insidePoint) const
{
	RNConvexVolumeSettings settings;
	settings.set_ref(-1);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, settings)

	int convexVolumeID = do_get_convex_volume_from_point(insidePoint);
	//
	if (convexVolumeID != -1)
	{
		//mConvexVolumes and mGeom::m_volumes are synchronized
		settings = mConvexVolumes[convexVolumeID].get_second();
	}
	//return settings
	return settings;
}

/**
 * Returns settings of the convex volume given its ref.
 * Can be get only after RNNavMesh is setup.
 * Returns RNConvexVolumeSettings::ref = -1 on error.
 */
RNConvexVolumeSettings RNNavMesh::get_convex_volume_settings(int ref) const
{
	RNConvexVolumeSettings settings;
	settings.set_ref(-1);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, settings)

	pvector<PointListConvexVolumeSettings>::const_iterator iter;
	for (iter = mConvexVolumes.begin(); iter != mConvexVolumes.end(); ++iter)
	{
		if ((*iter).get_second().get_ref() == ref)
		{
			settings = (*iter).get_second();
			break;
		}
	}
	//return settings
	return settings;
}

/**
 * Gets a convex volume's point list given its unique reference (>0).
 * Returns an empty list on error.
 */
ValueList<LPoint3f> RNNavMesh::get_convex_volume_by_ref(int ref) const
{
	ValueList<LPoint3f> pointList;

	pvector<PointListConvexVolumeSettings>::const_iterator iter;
	for (iter = mConvexVolumes.begin(); iter != mConvexVolumes.end(); ++iter)
	{
		if ((*iter).get_second().get_ref() == ref)
		{
			// break: LPoint3f by ref is present
			pointList = (*iter).get_first();
			break;
		}
	}
	return pointList;
}

/**
 * Adds an off mesh connection given its two points and if bidirectional.
 * Can be added only before RNNavMesh is setup.
 * \note pointPair[0] = begin point, pointPair[1] = end point
 * Returns the off mesh connection's unique reference, or a negative number
 * on error.
 * \note The added off mesh connection is temporary: after setup this off mesh
 * connection can be eliminated, so reference validity should always be
 * verified before use.
 */
int RNNavMesh::add_off_mesh_connection(const ValueList<LPoint3f>& points,
		bool bidirectional)
{
	// go on if nav mesh has not been already setup
	nassertr_always((!mNavMeshType) && (points.size() >= 2), RN_ERROR)

	// add to off mesh connections
	RNOffMeshConnectionSettings settings;
	settings.set_rad(mNavMeshSettings.get_agentRadius());
	settings.set_bidir(bidirectional);
	settings.set_area(POLYAREA_JUMP);
	settings.set_flags(POLYFLAGS_JUMP);
	int ref = unique_ref();
	settings.set_ref(ref);
	mOffMeshConnections.push_back(
			PointPairOffMeshConnectionSettings(points, settings));
#ifdef RN_DEBUG
	do_debug_static_render_unsetup();
#endif //RN_DEBUG
	return ref;
}

/**
 * Removes an off mesh connection given the begin or end point.
 * Can be removed only before RNNavMesh is setup.
 * Returns the off mesh connection's reference, or a negative number on error.
 */
int RNNavMesh::remove_off_mesh_connection(const LPoint3f& beginOrEndPoint)
{
	// go on if nav mesh has not been already setup
	nassertr_always(!mNavMeshType, RN_ERROR)

	//set oldRef=-1 in case of error
	int oldRef = -1;
	///HACK: use support functionality to remove off mesh connections
	//create fake InputGeom, NavMeshType and OffMeshConnectionTool
	rnsup::InputGeom* geom = new rnsup::InputGeom;
	rnsup::NavMeshType* navMeshType = new rnsup::NavMeshType_Solo();
	rnsup::OffMeshConnectionTool* omcTool = new rnsup::OffMeshConnectionTool();
	navMeshType->handleMeshChanged(geom);
	navMeshType->setTool(omcTool);
	//cycle the existing off mesh connections (see setup()) and
	//check if they can be added then removed given the insidePoint
	pvector<PointPairOffMeshConnectionSettings>::iterator omcI;
	for (omcI = mOffMeshConnections.begin(); omcI != mOffMeshConnections.end();
			++omcI)
	{
		//try to add this off mesh connection (the NavMeshType has none)
		ValueList<LPoint3f> pointPair = omcI->first();
		omcTool->setBidir(omcI->second().get_bidir());
		float recastPos[3];
		rnsup::LVecBase3fToRecast(pointPair[0], recastPos);
		omcTool->handleClick(NULL, recastPos, false);
		rnsup::LVecBase3fToRecast(pointPair[1], recastPos);
		omcTool->handleClick(NULL, recastPos, false);
		//check if off mesh connection has been actually added
		if (omcTool->getOffMeshConnectionIdx() == -1)
		{
			continue;
		}
		//now try to remove this off mesh connection
		rnsup::LVecBase3fToRecast(beginOrEndPoint, recastPos);
		omcTool->handleClick(NULL, recastPos, true);
		//check if the off mesh connection has been removed
		if (omcTool->getOffMeshConnectionIdx() == -1)
		{
			//this off mesh connection has been removed, so
			//calculate oldRef (>=0), remove the off mesh
			//connection and then stop cycle
			oldRef = (*omcI).second().get_ref();
			mOffMeshConnections.erase(omcI);
			break;
		}
		// remove this convex volume and continue cycle
		geom->deleteOffMeshConnection(0);
	}
	//delete fake objects
	navMeshType->setTool(NULL);
	delete navMeshType;
	delete geom;
#ifdef RN_DEBUG
	do_debug_static_render_unsetup();
#endif //RN_DEBUG
	//return oldRef or -1
	return oldRef;
}

/**
 * Gets the off mesh connection given the begin or end point.
 * Returns the index of the convex volume, or -1 if none is found.
 */
int RNNavMesh::do_get_off_mesh_connection_from_point(
		const LPoint3f& startEndPoint) const
{
	//get hit point
	float hitPos[3];
	rnsup::LVecBase3fToRecast(startEndPoint, hitPos);
	//check if a off mesh connection was hit (see: Delete case of OffMeshConnectionTool::handleClick)
	int offMeshConnectionID = -1;
	// Find nearest link end-point
	float nearestDist = FLT_MAX;
	int nearestIndex = -1;
	const float* verts =
			mNavMeshType->getInputGeom()->getOffMeshConnectionVerts();
	for (int i = 0;
			i < mNavMeshType->getInputGeom()->getOffMeshConnectionCount() * 2;
			++i)
	{
		const float* v = &verts[i * 3];
		float d = rcVdistSqr(hitPos, v);
		if (d < nearestDist)
		{
			nearestDist = d;
			nearestIndex = i / 2; // Each link has two vertices.
		}
	}
	// If end point close enough, got it.
	if (nearestIndex != -1
			&& sqrtf(nearestDist)
					< mOffMeshConnections[nearestIndex].get_second().get_rad())
	{
		offMeshConnectionID = nearestIndex;
	}
	return offMeshConnectionID;
}


int RNNavMesh::do_find_poly_of_off_mesh_connection(int offMeshConnectionID,
		dtPolyRef* poly, dtOffMeshConnection& offmeshlink) const// XXX
{
	//get the start pos
	const float* pos =
			&(mNavMeshType->getInputGeom()->getOffMeshConnectionVerts()[offMeshConnectionID
					* 3]);
//	//or get the end pos
//	float* pos =
//			&(mNavMeshType->getInputGeom()->getOffMeshConnectionVerts()[offMeshConnectionID
//					* 3 + 3]);

	//https://groups.google.com/forum/#!searchin/recastnavigation/how$20to$20search$20off$20mesh$20connection|sort:relevance/recastnavigation/kYbahygY96s/JIUz-bb5C4IJ
	const dtNavMesh& mesh = *mNavMeshType->getNavMesh();
	int ctx = 0, cty = 0;
	mesh.calcTileLoc(pos, &ctx, &cty);

//	for (int i = 0; i < mNavMeshType->getNavMesh()->getMaxTiles(); ++i)
	for (int ty = cty - 1; ty <= cty + 1; ty++)
	{
		for (int tx = ctx - 1; tx <= ctx + 1; tx++)
		{
			// If using tile cache, use the one which returns multiple tiles.
			const dtMeshTile* tile = mesh.getTileAt(tx, ty, 0);
			if (!tile)
			{
				continue;
			}
			// Handle tile...
//		const dtMeshTile* tile = mesh.getTile(i);
//		if (!tile->header)
//		{
//			continue;
//		}
			for (int j = 0; j < tile->header->offMeshConCount; ++j)
			{

				/*dtOffMeshConnection& */offmeshlink = tile->offMeshCons[j];// XXX
				const float* startpos = offmeshlink.pos;
				const float* endpos = &offmeshlink.pos[3];
				float radius = offmeshlink.rad;
				if (dtVdist(pos, startpos) > radius
						&& dtVdist(pos, endpos) > radius)
				{
					continue;
				}

				dtPolyRef base = mNavMeshType->getNavMesh()->getPolyRefBase(
						tile);
				/*dtPolyRef*/*poly = base | (dtPolyRef) offmeshlink.poly;

//				bool enable;//fake
//				unsigned short flags;
//				dtStatus status = mNavMeshType->getNavMesh()->getPolyFlags(
//						*poly, &flags);
//				if (dtStatusSucceed(status))
//				{
//					if (enable)
//						flags &= ~POLYFLAGS_DISABLED;
//					else
//						flags |= POLYFLAGS_DISABLED;
//					mNavMeshType->getNavMesh()->setPolyFlags(*poly, flags);
//				}
			}
		}
	}

	///////////////////////////////////////////////////
//	int ctx = 0, cty = 0;
//	mesh.calcTileLoc(pos, &ctx, &cty);
//
//	for (int ty = cty - 1; ty <= cty + 1; ty++)
//	{
//		for (int tx = ctx - 1; tx <= ctx + 1; tx++)
//		{
//			// If using tile cache, use the one which returns multiple tiles.
//			const dtMeshTile* tile = mesh.getTileAt(tx, ty, 0);
//			if (!tile)
//				continue;
//			// Handle tile...
//		}
//	}

	return RN_SUCCESS;
}


/**
 * Updates settings of the off mesh connection given the begin or end point.
 * Can be set only after RNNavMesh is setup.
 * Returns the index of the off mesh connection, or -1 if none is found or on error.
 */
int RNNavMesh::set_off_mesh_connection_settings(const LPoint3f& beginOrEndPoint,
	const RNOffMeshConnectionSettings settings)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_ERROR)

	int offMeshConnectionID = do_get_off_mesh_connection_from_point(
			beginOrEndPoint);

	// check if a off mesh connection was it
	if (offMeshConnectionID != -1)
	{
		dtPolyRef poly;
		dtOffMeshConnection offmeshlink;	/// XXX
		dtStatus status, status2;

		nassertr_always(
				do_find_poly_of_off_mesh_connection(offMeshConnectionID, &poly, offmeshlink) == RN_SUCCESS,
				RN_ERROR)

		int area = (
				settings.get_area() >= 0 ?
						settings.get_area() : -settings.get_area());

		// XXX rad dynamically changeable ? test!
		// if not possible remove parameter dtOffMeshConnection& offmeshlink
		// from do_find_poly_of_off_mesh_connection()
		int rad = (
				settings.get_rad() >= 0 ?
						settings.get_rad() : -settings.get_rad());// XXX
		offmeshlink.rad = rad; // XXX
		status = mNavMeshType->getNavMesh()->setPolyArea(poly, area);
		status2 = mNavMeshType->getNavMesh()->setPolyFlags(poly,
				settings.get_flags());
		if (dtStatusSucceed(status) && dtStatusSucceed(status2))
		{
			//mOffMeshConnections and mGeom's off mesh connections are synchronized
			//update mOffMeshConnections settings
			mOffMeshConnections[offMeshConnectionID].set_second(settings);
		}
		else
		{
			//errors: reset to previous values
			int oldRad =
					mOffMeshConnections[offMeshConnectionID].get_second().get_rad(); // XXX
			int oldArea =
					mOffMeshConnections[offMeshConnectionID].get_second().get_area();
			int oldFlags =
					mOffMeshConnections[offMeshConnectionID].get_second().get_flags();
			offmeshlink.rad = oldRad; // XXX
			mNavMeshType->getNavMesh()->setPolyArea(poly, oldArea);
			mNavMeshType->getNavMesh()->setPolyFlags(poly, oldFlags);
			offMeshConnectionID = -1;
		}
	}

#ifdef RN_DEBUG
	if (!mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//return
	return offMeshConnectionID;
}

/**
 * Returns settings of the off mesh connection given the begin or end point.
 * Can be get only after RNNavMesh is setup.
 * Returns RNOffMeshConnectionSettings::ref = -1 on error.
 */
RNOffMeshConnectionSettings RNNavMesh::get_off_mesh_connection_settings(
	const LPoint3f& beginOrEndPoint) const
{
	RNOffMeshConnectionSettings settings;
	settings.set_ref(-1);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, settings)

	int offMeshConnectionID = do_get_off_mesh_connection_from_point(
			beginOrEndPoint);
	//
	if (offMeshConnectionID != -1)
	{
		//mOffMeshConnections and mGeom's off mesh connections are synchronized
		settings = mOffMeshConnections[offMeshConnectionID].get_second();
	}
	//return settings
	return settings;
}


/**
 * Returns settings of the off mesh connection given its ref.
 * Can be get only after RNNavMesh is setup.
 * Returns RNOffMeshConnectionSettings::ref = -1 on error.
 */
RNOffMeshConnectionSettings RNNavMesh::get_off_mesh_connection_settings(
		int ref) const
{
	RNOffMeshConnectionSettings settings;
	settings.set_ref(-1);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, settings)

	pvector<PointPairOffMeshConnectionSettings>::const_iterator iter;
	for (iter = mOffMeshConnections.begin(); iter != mOffMeshConnections.end(); ++iter)
	{
		if ((*iter).get_second().get_ref() == ref)
		{
			settings = (*iter).get_second();
			break;
		}
	}
	//return settings
	return settings;
}

/**
 * Gets an off mesh connection's point list given its unique reference (>0).
 * Returns an empty list on error.
 */
ValueList<LPoint3f> RNNavMesh::get_off_mesh_connection_by_ref(int ref) const
{
	ValueList<LPoint3f> pointList;

	pvector<PointPairOffMeshConnectionSettings>::const_iterator iter;
	for (iter = mOffMeshConnections.begin(); iter != mOffMeshConnections.end();
			++iter)
	{
		if ((*iter).get_second().get_ref() == ref)
		{
			// break: LPoint3f by ref is present
			pointList = (*iter).get_first();
			break;
		}
	}
	return pointList;
}

/**
 * On destruction cleanup.
 * Gives a RNNavMesh the ability to do any required cleanup just
 * when being destroyed.
 */
void RNNavMesh::do_finalize()
{
	//remove all handled CrowdAgents (if any)
	PTA(PT(RNCrowdAgent))::iterator iter = mCrowdAgents.begin();
	while (iter != mCrowdAgents.end())
	{
		//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
		do_remove_crowd_agent_from_update_list(*iter);
		do_remove_crowd_agent_from_recast_update(*iter);
	}

	//cleanup RNNavMesh
	cleanup();

	//remove this NodePath
	NodePath::any_path(this).remove_node();
	//
#ifdef RN_DEBUG
	// delete un-setup debug drawing
	mDDUnsetup->reset();
	delete mDDUnsetup;
#endif //RN_DEBUG
	//
	do_reset();
}

/**
 * Cleans up the RNNavMesh.
 * \note crowd agents, and obstacles are detached after this call.
 */
int RNNavMesh::cleanup()
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_SUCCESS)

	// remove all obstacles from recast
	if (mNavMeshTypeEnum == OBSTACLE)
	{
		PTA(Obstacle)::iterator iterO;
		for (iterO = mObstacles.begin(); iterO != mObstacles.end(); ++iterO)
		{
			do_remove_obstacle_from_recast(iterO->second(),
					iterO->first());
		}
	}

	// remove crowd agents from recast update
	PTA(PT(RNCrowdAgent))::const_iterator iterC;
	for (iterC = mCrowdAgents.begin(); iterC != mCrowdAgents.end(); ++iterC)
	{
		do_remove_crowd_agent_from_recast_update(*iterC);
	}

	//do real cleanup
	if (mNavMeshType)
	{
		//reset NavMeshTypeTool
		mNavMeshType->setTool(NULL);
	}

	//delete old navigation mesh type
	delete mNavMeshType;
	mNavMeshType = NULL;

	//delete old model mesh
	delete mGeom;
	mGeom = NULL;

	//delete build context
	delete mCtx;
	mCtx = NULL;

	//reset owner object
	mOwnerObject.clear();

	//reset name
	set_name("RNNavMesh");

#ifdef RN_DEBUG
	// disable debug drawing;
	disable_debug_drawing();
	// un-setup debug node path
	NodePath debugNodePathUnsetup = mReferenceNP.attach_new_node(
				string("DebugNodePathUsetup") + get_name());
	//no collide mask for all their children
	debugNodePathUnsetup.set_collide_mask(BitMask32::all_off());
	// enable un-setup debug drawing
	mDDUnsetup = new rnsup::DebugDrawPanda3d(debugNodePathUnsetup);
	do_debug_static_render_unsetup();
#endif //RN_DEBUG

	//
	return RN_SUCCESS;
}

/**
 * Gets the position of a RNNavMesh's tile (TILE).
 * Can be get only after RNNavMesh is setup.
 */
LVecBase2i RNNavMesh::get_tile_pos(const LPoint3f& pos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType,
			LVecBase2i(RN_NAVMESH_NULL, RN_NAVMESH_NULL))

	int tx, ty;
	float recastPos[3];
	rnsup::LVecBase3fToRecast(pos, recastPos);
	if (mNavMeshTypeEnum == TILE)
	{
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->getTilePos(
				recastPos, tx, ty);
	}
	else if (mNavMeshTypeEnum == OBSTACLE)
	{
		static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->getTilePos(
				recastPos, tx, ty);
	}
	//
	return LVecBase2i(tx, ty);
}

/**
 * Builds a RNNavMesh's tile (TILE).
 * Can be built only after RNNavMesh is setup.
 */
int RNNavMesh::build_tile(const LPoint3f& pos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	if (mNavMeshTypeEnum == TILE)
	{
		float recastPos[3];
		rnsup::LVecBase3fToRecast(pos, recastPos);
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->buildTile(
				recastPos);
		PRINT_DEBUG("'" << get_owner_node_path() << "' build_tile : " << pos);
#ifdef RN_DEBUG
		if (! mDebugCamera.is_empty())
		{
			do_debug_static_render();
		}
#endif //RN_DEBUG
	}
	//
	return RN_SUCCESS;
}

/**
 * Removes a RNNavMesh's tile (TILE).
 * Can be get only after RNNavMesh is setup.
 */
int RNNavMesh::remove_tile(const LPoint3f& pos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	if (mNavMeshTypeEnum == TILE)
	{
		float recastPos[3];
		rnsup::LVecBase3fToRecast(pos, recastPos);
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->removeTile(
				recastPos);
		PRINT_DEBUG("'" << get_owner_node_path() << "' remove_tile : " << pos);
#ifdef RN_DEBUG
		if (! mDebugCamera.is_empty())
		{
			do_debug_static_render();
		}
#endif //RN_DEBUG
	}
	//
	return RN_SUCCESS;
}

/**
 * Builds all RNNavMesh's tiles (TILE).
 * Can be get only after RNNavMesh is setup.
 */
int RNNavMesh::build_all_tiles()
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	if (mNavMeshTypeEnum == TILE)
	{
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->buildAllTiles();
#ifdef RN_DEBUG
		if (! mDebugCamera.is_empty())
		{
			do_debug_static_render();
		}
#endif //RN_DEBUG
	}
	//
	return RN_SUCCESS;
}

/**
 * Removes all RNNavMesh's tiles (TILE).
 * Can be get only after RNNavMesh is setup.
 */
int RNNavMesh::remove_all_tiles()
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	if (mNavMeshTypeEnum == TILE)
	{
		static_cast<rnsup::NavMeshType_Tile*>(mNavMeshType)->removeAllTiles();
#ifdef RN_DEBUG
		if (! mDebugCamera.is_empty())
		{
			do_debug_static_render();
		}
#endif //RN_DEBUG
	}
	//
	return RN_SUCCESS;
}

/**
 * Adds an obstacle as NodePath (OBSTACLE).
 * Returns the obstacle's unique reference (>0), or a negative number on error.
 */
int RNNavMesh::add_obstacle(NodePath objectNP)
{

	// go on if not empty node paths and we have OBSTACLE
	// nav mesh type and mReferenceNP is not empty
	nassertr_always(
			(!objectNP.is_empty()) && (mNavMeshTypeEnum == OBSTACLE)
					&& (! mReferenceNP.is_empty()), RN_ERROR)

	// return error if objectNP is already present
	pvector<Obstacle>::iterator iterO;
	for (iterO = mObstacles.begin(); iterO < mObstacles.end(); ++iterO)
	{
		if ((*iterO).second().node() == objectNP.node())
		{
			// break: objectNP is already present
			break;
		}
	}
	nassertr_always(iterO == mObstacles.end(), RN_ERROR)

	// insert obstacle with invalid ref: later will be corrected
	mObstacles.push_back(Obstacle(-1, objectNP));

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	// return the result of adding obstacle to recast
	return do_add_obstacle_to_recast(objectNP, mObstacles.size() - 1);
}

/**
 * Adds obstacle to recast.
 */
int RNNavMesh::do_add_obstacle_to_recast(NodePath& objectNP, int index)
{
		//get obstacle dimensions
		LVecBase3f modelDims;
		LVector3f modelDeltaCenter;
		float modelRadius =
				RNNavMeshManager::get_global_ptr()->get_bounding_dimensions(
						objectNP, modelDims, modelDeltaCenter);

		//the obstacle is reparented to the RNNavMesh's reference node path
		objectNP.wrt_reparent_to(mReferenceNP);

		//calculate pos wrt reference node path
		LPoint3f pos = objectNP.get_pos();
		//add detour obstacle
		dtObstacleRef obstacleRef;
		float recastPos[3];
		rnsup::LVecBase3fToRecast(pos, recastPos);
		dtTileCache* tileCache =
				static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->getTileCache();
		// go on if obstacle addition to tile cache is successful
		nassertr_always(
				tileCache->addObstacle(recastPos, modelRadius,
						modelDims.get_z(), &obstacleRef) == DT_SUCCESS, RN_ERROR)

		//update tiles cache: repeat for all the tiles touched
		for (int c = 0; c < DT_MAX_TOUCHED_TILES; ++c)
		{
			tileCache->update(0, mNavMeshType->getNavMesh());
		}
		//correct to the obstacle ref
		mObstacles[index].set_first(obstacleRef);
		PRINT_DEBUG(
				"'" << get_owner_node_path() << "' add_obstacle: '" << objectNP << "' at pos: " << pos);
#ifdef RN_DEBUG
		if (! mDebugCamera.is_empty())
		{
			do_debug_static_render();
		}
#endif //RN_DEBUG
		// obstacle added: return the last index
		return (int) obstacleRef;
}

/**
 * Removes an obstacle as NodePath (OBSTACLE).
 * Returns the old obstacle's unique reference (>0), or a negative number on error.
 */
int RNNavMesh::remove_obstacle(NodePath objectNP)
{
	// go on if not empty node paths and we have OBSTACLE
	// nav mesh type and mReferenceNP is not empty
	nassertr_always(
			(!objectNP.is_empty()) && (mNavMeshTypeEnum == OBSTACLE)
					&& (! mReferenceNP.is_empty()), RN_ERROR)

	// return error if objectNP is not yet present
	pvector<Obstacle>::iterator iterO;
	for (iterO = mObstacles.begin(); iterO < mObstacles.end(); ++iterO)
	{
		if ((*iterO).second().node() == objectNP.node())
		{
			// break: objectNP is present
			break;
		}
	}
	nassertr_always(iterO != mObstacles.end(), RN_ERROR)

	// remove obstacle: save ref for later removing from recast
	int obstacleRef = iterO->first();
	mObstacles.erase(iterO);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	// return the result of removing obstacle from recast
	return do_remove_obstacle_from_recast(objectNP, obstacleRef);
}

/**
 * Removes obstacle from recast.
 */
int RNNavMesh::do_remove_obstacle_from_recast(NodePath& objectNP,
		int obstacleRef)
{
	//remove recast obstacle
	dtTileCache* tileCache =
			static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->getTileCache();
	// go on if obstacle removal from tile cache is successful
	nassertr_always(tileCache->removeObstacle(obstacleRef) == DT_SUCCESS, RN_ERROR)

	//update tiles cache: repeat for all the tiles touched
	for (int c = 0; c < DT_MAX_TOUCHED_TILES; ++c)
	{
		tileCache->update(0, mNavMeshType->getNavMesh());
	}
	//detach_node the obstacle's node path
	objectNP.detach_node();
	//index and remove from obstacle from the list
	PRINT_DEBUG(
			"'" << get_owner_node_path() << "' remove_obstacle: '" << objectNP << "'");
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	// obstacle removed
	return (int) obstacleRef;
}

/**
 * Gets an obstacle's NodePath by its unique reference (>0).
 * Return an empty NodePath on error.
 */
NodePath RNNavMesh::get_obstacle_by_ref(int ref) const
{
	NodePath obstacleNP;
	nassertr_always((mNavMeshTypeEnum == OBSTACLE) && (ref > 0), obstacleNP);

	pvector<Obstacle>::const_iterator iter;
	for (iter = mObstacles.begin(); iter != mObstacles.end(); ++iter)
	{
		if ((*iter).get_first() == ref)
		{
			// break: obstacleNP by ref is present
			obstacleNP = (*iter).get_second();
			break;
		}
	}
	return obstacleNP;
}

/**
 * Removes all obstacles (OBSTACLE).
 */
int RNNavMesh::remove_all_obstacles()
{
	nassertr_always(mNavMeshTypeEnum == OBSTACLE, RN_ERROR)

	PTA(Obstacle)::iterator iter = mObstacles.begin();
	while (iter != mObstacles.end())
	{
		//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
		remove_obstacle(iter->second());
	}

	//
	return RN_SUCCESS;
}

/**
 * Adds a RNCrowdAgent to this RNNavMesh (i.e. to the dtCrowd handling mechanism).
 */
int RNNavMesh::add_crowd_agent(NodePath crowdAgentNP)
{
	nassertr_always(
			(!crowdAgentNP.is_empty())
					&& (crowdAgentNP.node()->is_of_type(
							RNCrowdAgent::get_class_type())), RN_ERROR)

	PT(RNCrowdAgent)crowdAgent = DCAST(RNCrowdAgent, crowdAgentNP.node());

	bool result;
	// go on if crowdAgent doesn't belong to any mesh
	nassertr_always(! crowdAgent->mNavMesh, RN_ERROR)

	//do real adding to update list
	do_add_crowd_agent_to_update_list(crowdAgent);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	//do real adding to recast update
	result = do_add_crowd_agent_to_recast_update(crowdAgent);
	//check if adding to recast was successful
	if (! result)
	{
		//remove RNCrowdAgent from update too (if previously added)
		mCrowdAgents.erase(
				find(mCrowdAgents.begin(), mCrowdAgents.end(), crowdAgent));
	}
	//
	return (result == true ? RN_SUCCESS : RN_ERROR);
}

/**
 * Adds RNCrowdAgent to update list.
 */
void RNNavMesh::do_add_crowd_agent_to_update_list(PT(RNCrowdAgent)crowdAgent)
{
	//add to update list
	PTA(PT(RNCrowdAgent))::iterator iterCA;
	//check if crowdAgent has not been already added
	iterCA = find(mCrowdAgents.begin(), mCrowdAgents.end(), crowdAgent);
	if(iterCA == mCrowdAgents.end())
	{
		//RNCrowdAgent needs to be added
		//set RNCrowdAgent's RNNavMesh owner object
		crowdAgent->mNavMesh = this;
		//add RNCrowdAgent
		mCrowdAgents.push_back(crowdAgent);
	}
}

/**
 * Adds RNCrowdAgent to recast update.
 */
bool RNNavMesh::do_add_crowd_agent_to_recast_update(PT(RNCrowdAgent)crowdAgent)
{
	//there is a crowd tool because the recast nav mesh
	//has been completely setup
	//check if crowdAgent has not been already added to recast
	rnsup::CrowdTool* crowdTool = static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
	if(crowdAgent->mAgentIdx == -1)
	{
		//RNNavMesh object updates CrowdAgents pos/vel wrt its reference node path
		//the RNCrowdAgent node path is reparented to the reference node path
		NodePath::any_path(crowdAgent).wrt_reparent_to(mReferenceNP);
		crowdAgent->mReferenceNP = mReferenceNP;

		//get the actual pos
		LPoint3f pos = NodePath::any_path(crowdAgent).get_pos();
		//get recast p (y-up)
		float p[3];
		rnsup::LVecBase3fToRecast(pos, p);

		//get crowdAgent dimensions
		LVecBase3f modelDims;
		LVector3f modelDeltaCenter;
		float modelRadius =
				RNNavMeshManager::get_global_ptr()->get_bounding_dimensions(
						NodePath::any_path(crowdAgent), modelDims,
						modelDeltaCenter);

		//update RNNavMeshSettings temporarily (mandatory)
		RNNavMeshSettings settings, oldNavMeshSettings;
		oldNavMeshSettings = settings = get_nav_mesh_settings();
		settings.set_agentRadius(modelRadius);
		settings.set_agentHeight(modelDims.get_z());
		set_nav_mesh_settings(settings);

		//update dtCrowdAgentParams
		dtCrowdAgentParams ap = crowdAgent->mAgentParams;
		ap.radius = modelRadius;
		ap.height = modelDims.get_z();

		//set height correction
		crowdAgent->mHeigthCorrection = LVector3f(0.0, 0.0, modelDims.get_z());

		//add recast agent and set the index of the crowd agent
		crowdAgent->mAgentIdx = crowdTool->getState()->addAgent(p, &ap);
		if(crowdAgent->mAgentIdx == -1)
		{
			//agent has not been added to recast
			//detach the node path
			NodePath::any_path(crowdAgent).detach_node();
			crowdAgent->mReferenceNP.clear();
			//restore RNNavMeshSettings
			set_nav_mesh_settings(oldNavMeshSettings);
			return false;
		}
		//agent has been added to recast
		//update the (possibly) modified params
		crowdAgent->mAgentParams = ap;
		//set crowd agent other settings
		do_set_crowd_agent_other_settings(crowdAgent, crowdTool);

		//restore RNNavMeshSettings
		set_nav_mesh_settings(oldNavMeshSettings);
	}
	//agent has been added to recast
	return true;
}

/**
 * Removes a RNCrowdAgent from this RNNavMesh (i.e. from the dtCrowd handling mechanism).
 */
int RNNavMesh::remove_crowd_agent(NodePath crowdAgentNP)
{
	nassertr_always(
			(!crowdAgentNP.is_empty())
					&& (crowdAgentNP.node()->is_of_type(
							RNCrowdAgent::get_class_type())), RN_ERROR)

	PT(RNCrowdAgent)crowdAgent = DCAST(RNCrowdAgent, crowdAgentNP.node());

	// go on if crowdAgent belongs to this nav mesh
	nassertr_always(crowdAgent->mNavMesh == this, RN_ERROR)

	// remove from update list
	do_remove_crowd_agent_from_update_list(crowdAgent);

	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	//remove from recast update
	do_remove_crowd_agent_from_recast_update(crowdAgent);
	//
	return RN_SUCCESS;
}

/**
 * Removes RNCrowdAgent from update list.
 */
void RNNavMesh::do_remove_crowd_agent_from_update_list(PT(RNCrowdAgent)crowdAgent)
{
	//remove from update list
	PTA(PT(RNCrowdAgent))::iterator iterCA;
	//check if RNCrowdAgent has been already removed or not
	iterCA = find(mCrowdAgents.begin(), mCrowdAgents.end(), crowdAgent);
	if(iterCA != mCrowdAgents.end())
	{
		//RNCrowdAgent needs to be removed
		mCrowdAgents.erase(iterCA);
		//set RNCrowdAgent RNNavMesh reference to NULL
		crowdAgent->mNavMesh.clear();
	}
}

/**
 * Removes RNCrowdAgent from recast update.
 */
void RNNavMesh::do_remove_crowd_agent_from_recast_update(PT(RNCrowdAgent)crowdAgent)
{
	//there is a crowd tool because the recast nav mesh
	//has been completely setup
	//and check if crowdAgent has been already added to recast
	rnsup::CrowdTool* crowdTool = static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
	if (crowdAgent->mAgentIdx != -1)
	{
		//remove recast agent
		crowdTool->getState()->removeAgent(crowdAgent->mAgentIdx);
		//set the index of the crowd agent to -1
		crowdAgent->mAgentIdx = -1;
		//detach this crowd agent
		NodePath::any_path(crowdAgent).detach_node();
	}
}

/**
 * Sets CrowdAgentParams for a given added RNCrowdAgent.
 */
int RNNavMesh::do_set_crowd_agent_params(PT(RNCrowdAgent)crowdAgent,
const RNCrowdAgentParams& params)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	//there is a crowd tool because the recast nav mesh
	//has been completely setup
	//check if crowdAgent has been already added to recast
	if (crowdAgent->mAgentIdx != -1)
	{
		//second parameter should be the parameter to update
		dtCrowdAgentParams ap = params;
		static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool())->
				getState()->getCrowd()->updateAgentParameters(crowdAgent->mAgentIdx, &ap);
	}
	crowdAgent->mAgentParams = params;
	//
	return RN_SUCCESS;
}

/**
 * Gets the underlying dtTileCache (OBSTACLE).
 * Can be get only after RNNavMesh is setup.
 */
dtTileCache* RNNavMesh::get_recast_tile_cache() const
{
	// go on if nav mesh has been already setup and is of type OBSTACLE
	nassertr_always(mNavMeshType && (mNavMeshTypeEnum == OBSTACLE), NULL)

	return static_cast<rnsup::NavMeshType_Obstacle*>(mNavMeshType)->getTileCache();
}

/**
 * Sets the target for a given added RNCrowdAgent.
 * Can be get only after RNNavMesh is setup.
 */
int RNNavMesh::do_set_crowd_agent_target(PT(RNCrowdAgent)crowdAgent,
const LPoint3f& moveTarget)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	//there is a crowd tool because the recast nav mesh
	//has been completely setup
	//check if crowdAgent has been already added to recast
	if (crowdAgent->mAgentIdx != -1)
	{
		float p[3];
		rnsup::LVecBase3fToRecast(moveTarget, p);
		static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool())->
		getState()->setMoveTarget(crowdAgent->mAgentIdx, p);
	}
	crowdAgent->mMoveTarget = moveTarget;
	//
	return RN_SUCCESS;
}

/**
 * Sets the target velocity for a given added RNCrowdAgent.
 * Can be get only after RNNavMesh is setup.
 */
int RNNavMesh::do_set_crowd_agent_velocity(PT(RNCrowdAgent)crowdAgent,
const LVector3f& moveVelocity)
{
	//return if NavMeshType has not been setup yet
	nassertr_always(mNavMeshType, RN_NAVMESH_NULL)

	//there is a crowd tool because the recast nav mesh
	//has been completely setup
	//check if crowdAgent has been already added to recast
	if (crowdAgent->mAgentIdx != -1)
	{
		float v[3];
		rnsup::LVecBase3fToRecast(moveVelocity, v);
		static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool())->
		getState()->setMoveVelocity(crowdAgent->mAgentIdx, v);
	}
	crowdAgent->mMoveVelocity = moveVelocity;
	//
	return RN_SUCCESS;
}

/**
 * Helper: only called by other methods.
 */
void RNNavMesh::do_set_crowd_agent_other_settings(PT(RNCrowdAgent)crowdAgent,
		rnsup::CrowdTool* crowdTool)
{
	//update move target
	float target[3];
	rnsup::LVecBase3fToRecast(crowdAgent->mMoveTarget, target);
	crowdTool->getState()->setMoveTarget(crowdAgent->mAgentIdx, target);
	//update move velocity (if length != 0)
	if(length(crowdAgent->mMoveVelocity) > 0.0)
	{
		float velocity[3];
		rnsup::LVecBase3fToRecast(crowdAgent->mMoveVelocity, velocity);
		crowdTool->getState()->setMoveVelocity(crowdAgent->mAgentIdx, velocity);
	}
}

#ifdef RN_DEBUG
/**
 * Renders static geometry on debug.
 */
void RNNavMesh::do_debug_static_render()
{
	// debug render with DebugDrawPanda3d
	mDD->reset();
	mNavMeshType->handleRender(*mDD);
	mNavMeshType->getInputGeom()->drawConvexVolumes(mDD);
	mNavMeshType->getInputGeom()->drawOffMeshConnections(mDD, true);
	mTesterTool.handleRender(*mDD);
}
/**
 * Renders static geometry on debug with mNavMeshType un-setup.
 */
void RNNavMesh::do_debug_static_render_unsetup()
{
	// add convex volumes and off mesh connections to a geom
	rnsup::InputGeom* geomUnsetup = new rnsup::InputGeom();
	{
		pvector<PointListConvexVolumeSettings>::iterator iter =
				mConvexVolumes.begin();
		while (iter != mConvexVolumes.end())
		{
			ValueList<LPoint3f>& points = iter->first();
			int area = iter->second().get_area();
			// iterate, compute recast points
			float* recastPos = new float[points.size() * 3];
			float minh = FLT_MAX, maxh = 0;
			for (int i = 0; i != points.size(); ++i)
			{
				rnsup::LVecBase3fToRecast(points[i], &recastPos[i * 3]);
				minh = rcMin(minh, recastPos[i*3+1]);
			}
			minh -= 1.0;
			maxh = minh + 6.0;
			//insert convex volume
			geomUnsetup->addConvexVolume(recastPos, points.size(), minh, maxh,
					area);
			// delete recastPos
			delete[] recastPos;
			//increment iterator
			++iter;
		}
	}
	{
		pvector<PointPairOffMeshConnectionSettings>::iterator iter =
				mOffMeshConnections.begin();
		while (iter != mOffMeshConnections.end())
		{
			ValueList<LPoint3f>& pointPair = iter->first();
			bool bidir = iter->second().get_bidir();

			//compute and insert first recast point
			float recastPos[3 * 2];
			rnsup::LVecBase3fToRecast(pointPair[0], &recastPos[0]);
			rnsup::LVecBase3fToRecast(pointPair[1], &recastPos[3]);
			mNavMeshType->getTool()->handleClick(NULL, recastPos, false);
			// insert off mesh connection
			geomUnsetup->addOffMeshConnection(&recastPos[0], &recastPos[3],
					mNavMeshSettings.get_agentRadius(), bidir, POLYAREA_JUMP,
					POLYFLAGS_JUMP);
			//increment iterator
			++iter;
		}
	}
	//debug render with DebugDrawPanda3d
	mDDUnsetup->reset();
	geomUnsetup->drawConvexVolumes(mDDUnsetup);
	geomUnsetup->drawOffMeshConnections(mDDUnsetup, true);
	delete geomUnsetup;
}
#endif //RN_DEBUG

/**
 * Loads the mesh from a model NodePath.
 */
bool RNNavMesh::do_load_model_mesh(NodePath model)
{
	bool result = true;
	mGeom = new rnsup::InputGeom;
	mMeshName = model.get_name();
	//
	if ((!mGeom) || (!mGeom->loadMesh(mCtx, string(), model, mReferenceNP)))
	{
		delete mGeom;
		mGeom = NULL;
#ifdef RN_DEBUG
		mCtx->dumpLog("Geom load log %s:", mMeshName.c_str());
#endif //RN_DEBUG
		result = false;
	}
	return result;
}

/**
 * Creates the navigation mesh type for the loaded model mesh.
 */
void RNNavMesh::do_create_nav_mesh_type(rnsup::NavMeshType* navMeshType)
{
	//delete old navigation mesh type
	delete mNavMeshType;
	//set the navigation mesh type
	mNavMeshType = navMeshType;
	//set rcContext
	mNavMeshType->setContext(mCtx);
	//handle Mesh Changed
	mNavMeshType->handleMeshChanged(mGeom);
}


/**
 * Updates position/orientation of all added RNCrowdAgents along their navigation paths.
 */
void RNNavMesh::update(float dt)
{
	// go on if nav mesh has been already setup
	RETURN_ON_COND(!mNavMeshType,)

#ifdef TESTING
	dt = 0.016666667; //60 fps
#endif

	// there is a crowd tool when nav mesh is setup
	// so update is done only when there is one
	rnsup::CrowdTool* crowdTool =
			static_cast<rnsup::CrowdTool*>(mNavMeshType->getTool());
	dtCrowd* crowd = crowdTool->getState()->getCrowd();

	//update crowd agents' pos/vel
	mNavMeshType->handleUpdate(dt);

	//post-update all agent positions
	pvector<PT(RNCrowdAgent)>::iterator iter;
	for (iter = mCrowdAgents.begin(); iter != mCrowdAgents.end(); ++iter)
	{
		int agentIdx = (*iter)->mAgentIdx;
		//give RNCrowdAgent a chance to update its pos/vel
		LPoint3f agentPos = rnsup::RecastToLVecBase3f(
				crowd->getAgent(agentIdx)->npos);
		LVector3f agentDir = rnsup::RecastToLVecBase3f(
				crowd->getAgent(agentIdx)->vel);
		(*iter)->do_update_pos_dir(dt, agentPos, agentDir);
	}
	//
#ifdef RN_DEBUG
	if (mEnableDrawUpdate)
	{
		if (mDDM)
		{
			mDDM->reset();
			mNavMeshType->renderToolStates(*mDDM);
		}
	}
#endif //RN_DEBUG
}

/**
 * Finds a path from the start point to the end point.
 * Can be get only after RNNavMesh is setup.
 * Returns a list of points, empty on error.
 */
ValueList<LPoint3f> RNNavMesh::path_find_follow(const LPoint3f& startPos,
		const LPoint3f& endPos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, ValueList<LPoint3f>())

	ValueList<LPoint3f> pointList;
	//set the extremes
	float recastStart[3], recastEnd[3];
	rnsup::LVecBase3fToRecast(startPos, recastStart);
	rnsup::LVecBase3fToRecast(endPos, recastEnd);
	mTesterTool.setStartEndPos(recastStart, recastEnd);
	//select tester tool mode
	mTesterTool.setToolMode(rnsup::NavMeshTesterTool::TOOLMODE_PATHFIND_FOLLOW);
	//recalculate path
	mTesterTool.recalc();
	//get the list of points
	for (int i = 0; i < mTesterTool.getNumSmoothPath(); ++i)
	{
		float* path = mTesterTool.getSmoothPath();
		pointList.add_value(
				rnsup::Recast3fToLVecBase3f(path[i * 3], path[i * 3 + 1],
						path[i * 3 + 2]));
	}
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//reset tester tool
	mTesterTool.reset();
	//
	return pointList;
}

/**
 * Finds a straight path from the start point to the end point.
 * Can be get only after RNNavMesh is setup.
 * Returns a list of points, empty on error.
 */
RNNavMesh::PointFlagList RNNavMesh::path_find_straight(
		const LPoint3f& startPos, const LPoint3f& endPos,
		RNStraightPathOptions crossingOptions)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, PointFlagList())

	PointFlagList pointFlagList;
	//set the extremes
	float recastStart[3], recastEnd[3];
	rnsup::LVecBase3fToRecast(startPos, recastStart);
	rnsup::LVecBase3fToRecast(endPos, recastEnd);
	mTesterTool.setStartEndPos(recastStart, recastEnd);
	//select tester tool mode
	mTesterTool.setToolMode(
			rnsup::NavMeshTesterTool::TOOLMODE_PATHFIND_STRAIGHT);
	//set crossing options
	mTesterTool.setStraightOptions(crossingOptions);
	//recalculate path
	mTesterTool.recalc();
	//get the list of points and flags
	float* points = mTesterTool.getStraightPath();
	unsigned char* flags = mTesterTool.getStraightPathFlags();
	for (int i = 0; i < mTesterTool.getNumStraightPath(); ++i)
	{
		pointFlagList.add_value(
				Pair<LPoint3f, unsigned char>(
						rnsup::Recast3fToLVecBase3f(points[i * 3],
								points[i * 3 + 1], points[i * 3 + 2]),
						flags[i]));
	}
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//reset tester tool
	mTesterTool.reset();
	//
	return pointFlagList;
}

/**
 * Casts a walkability/visibility ray from the start point toward the end point.
 * Can be casted only after RNNavMesh is setup.
 * Returns the first hit point if not walkable, or the end point if walkable.
 * This method is meant only for short distance checks.
 */
LPoint3f RNNavMesh::ray_cast(const LPoint3f& startPos,
		const LPoint3f& endPos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, LPoint3f(FLT_MAX, FLT_MAX, FLT_MAX))

	LPoint3f hitPoint = endPos;
	//set the extremes
	float recastStart[3], recastEnd[3];
	rnsup::LVecBase3fToRecast(startPos, recastStart);
	rnsup::LVecBase3fToRecast(endPos, recastEnd);
	mTesterTool.setStartEndPos(recastStart, recastEnd);
	//select tester tool mode
	mTesterTool.setToolMode(rnsup::NavMeshTesterTool::TOOLMODE_RAYCAST);
	//recalculate path
	mTesterTool.recalc();
	//get the hit point if any
	if (mTesterTool.getHitResult())
	{
		float* hitPos = mTesterTool.getHitPos();
		hitPoint = rnsup::Recast3fToLVecBase3f(hitPos[0], hitPos[1], hitPos[2]);
	}
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//reset tester tool
	mTesterTool.reset();
	//
	return hitPoint;
}

/**
 * Finds the distance from the specified position to the nearest polygon wall.
 * Can be found only after RNNavMesh is setup.
 */
float RNNavMesh::distance_to_wall(const LPoint3f& pos)
{
	// go on if nav mesh has been already setup
	nassertr_always(mNavMeshType, FLT_MAX)

	float distance = FLT_MAX;
	//set the extremes
	float recastPos[3];
	rnsup::LVecBase3fToRecast(pos, recastPos);
	//select tester tool mode
	mTesterTool.setToolMode(rnsup::NavMeshTesterTool::TOOLMODE_DISTANCE_TO_WALL);
	//recalculate path
	mTesterTool.recalc();
	//get the hit point if any
	distance = mTesterTool.getDistanceToWall();
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		do_debug_static_render();
	}
#endif //RN_DEBUG
	//reset tester tool
	mTesterTool.reset();
	//
	return distance;
}

/**
 * Writes a sensible description of the RNNavMesh to the indicated output
 * stream.
 */
void RNNavMesh::output(ostream &out) const
{
	out << get_type() << " " << get_name();
}

/**
 * Enables the debug drawing, only if nav mesh has been already setup.
 * Can be enabled only after RNNavMesh is setup.
 * A camera node path should be passed as argument.
 */
void RNNavMesh::enable_debug_drawing(NodePath debugCamera)
{
#ifdef RN_DEBUG
	nassertv_always(mDebugCamera.is_empty() && mNavMeshType)

	if ((!debugCamera.is_empty()) &&
			(!debugCamera.find(string("**/+Camera")).is_empty()))
	{
		mDebugCamera = debugCamera;
		//set the recast debug node path as child of mReferenceNP node path
		mDebugNodePath = mReferenceNP.attach_new_node(
				string("RecastDebugNodePath_") + get_name());
		mDebugNodePath.set_bin(string("fixed"), 10);
		//by default mDebugNodePath is hidden
		mDebugNodePath.hide();
		//no collide mask for all their children
		mDebugNodePath.set_collide_mask(BitMask32::all_off());
		//create new DebugDrawers
		mDD = new rnsup::DebugDrawPanda3d(mDebugNodePath);
		NodePath meshDrawerCamera = mDebugCamera;
		if (! mDebugCamera.node()->is_of_type(Camera::get_class_type()))
		{
			meshDrawerCamera = mDebugCamera.find(string("**/+Camera"));
		}
		mDDM = new rnsup::DebugDrawMeshDrawer(mDebugNodePath, meshDrawerCamera,
				100);
		//debug static render
		do_debug_static_render();
	}
#endif //RN_DEBUG
}

/**
 * Disables the debug drawing.
 */
void RNNavMesh::disable_debug_drawing()
{
#ifdef RN_DEBUG
	if (! mDebugCamera.is_empty())
	{
		//set the recast debug camera to empty node path
		mDebugCamera.clear();
		//remove the recast debug node path
		mDebugNodePath.remove_node();
	}
	//reset the DebugDrawers
	if (mDD)
	{
		delete mDD;
		mDD = NULL;
	}
	if (mDDM)
	{
		delete mDDM;
		mDDM = NULL;
	}
#endif //RN_DEBUG
}

/**
 * Enables/disables debugging.
 */
int RNNavMesh::toggle_debug_drawing(bool enable)
{
#ifdef RN_DEBUG
	// go on if both mDebugCamera and mDebugNodePath are not empty
	nassertr_always(
			(!mDebugCamera.is_empty()) && (! mDebugNodePath.is_empty()),
			RN_ERROR)

	if (enable)
	{
		if (mDebugNodePath.is_hidden())
		{
			mDebugNodePath.show();
			mEnableDrawUpdate = true;
			if (mDDM)
			{
				mDDM->clear();
			}
		}
	}
	else
	{
		if (! mDebugNodePath.is_hidden())
		{
			mDebugNodePath.hide();
			mEnableDrawUpdate = false;
			if (mDDM)
			{
				mDDM->clear();
			}
		}
	}
	//
#endif //RN_DEBUG
	return RN_SUCCESS;
}

//TypedWritable API
/**
 * Tells the BamReader how to create objects of type RNNavMesh.
 */
void RNNavMesh::register_with_read_factory()
{
	BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

/**
 * Writes the contents of this object to the datagram for shipping out to a
 * Bam file.
 */
void RNNavMesh::write_datagram(BamWriter *manager, Datagram &dg)
{
	PandaNode::write_datagram(manager, dg);

	///Name of this RNNavMesh.
	dg.add_string(get_name());

	///Current underlying NavMeshType type.
	dg.add_uint8((uint8_t) mNavMeshTypeEnum);

	///RNNavMesh's NavMeshSettings equivalent.
	mNavMeshSettings.write_datagram(dg);

	///RNNavMesh's NavMeshTileSettings equivalent.
	mNavMeshTileSettings.write_datagram(dg);

	///Area types with ability flags settings (see support/NavMeshType.h).
	dg.add_uint32(mPolyAreaFlags.size());
	{
		rnsup::NavMeshPolyAreaFlags::iterator iter;
		for (iter = mPolyAreaFlags.begin(); iter != mPolyAreaFlags.end();
				++iter)
		{
			dg.add_int32((*iter).first);
			dg.add_int32((*iter).second);
		}
	}

	///Area types with cost settings (see support/NavMeshType.h).
	dg.add_uint32(mPolyAreaCost.size());
	{
		rnsup::NavMeshPolyAreaCost::iterator iter;
		for (iter = mPolyAreaCost.begin(); iter != mPolyAreaCost.end(); ++iter)
		{
			dg.add_int32((*iter).first);
			dg.add_stdfloat((*iter).second);
		}
	}

	///Crowd include & exclude flags settings (see library/DetourNavMeshQuery.h).
	dg.add_int32(mCrowdIncludeFlags);
	dg.add_int32(mCrowdExcludeFlags);

	///Convex volumes (see support/ConvexVolumeTool.h).
	dg.add_uint32(mConvexVolumes.size());
	{
		pvector<PointListConvexVolumeSettings>::iterator iter;
		for (iter = mConvexVolumes.begin(); iter != mConvexVolumes.end();
				++iter)
		{
			//save this PointListConvexVolumeSettings
			ValueList<LPoint3f> pointList = (*iter).first();
			dg.add_uint32(pointList.size());
			for (int i = 0; i != pointList.size(); ++i)
			{
				pointList[i].write_datagram(dg);
			}
			(*iter).second().write_datagram(dg);
		}
	}

	///Off mesh connections (see support/OffMeshConnectionTool.h).
	dg.add_uint32(mOffMeshConnections.size());
	{
		pvector<PointPairOffMeshConnectionSettings>::iterator iter;
		for (iter = mOffMeshConnections.begin();
				iter != mOffMeshConnections.end(); ++iter)
		{
			//save this point pair
			ValueList<LPoint3f> pointPair = (*iter).first();
			pointPair[0].write_datagram(dg);
			pointPair[1].write_datagram(dg);
			(*iter).second().write_datagram(dg);
		}
	}

	///Unique ref.
	dg.add_uint32(mRef);

	/// Pointers
	///The owner object NodePath this RNNavMesh is associated to.
	manager->write_pointer(dg, mOwnerObject.node());

	///Crowd agents.
	dg.add_uint32(mCrowdAgents.size());
	{
		pvector<PT(RNCrowdAgent)>::iterator iter;
		for (iter = mCrowdAgents.begin(); iter != mCrowdAgents.end(); ++iter)
		{
			manager->write_pointer(dg, (*iter));
		}
	}

	/// Obstacles table.
	dg.add_uint32(mObstacles.size());
	{
		pvector<Obstacle>::iterator iter;
		for (iter = mObstacles.begin(); iter != mObstacles.end(); ++iter)
		{
			//obstacle ref will be recomputed in setup: so not saved
			PT(PandaNode)pandaNode = (*iter).second().node();
			manager->write_pointer(dg, pandaNode);
		}
	}
}

/**
 * Receives an array of pointers, one for each time manager->read_pointer()
 * was called in fillin(). Returns the number of pointers processed.
 */
int RNNavMesh::complete_pointers(TypedWritable **p_list, BamReader *manager)
{
	int pi = PandaNode::complete_pointers(p_list, manager);

	/// Pointers
	///The owner object NodePath this RNNavMesh is associated to.
	PT(PandaNode)ownerObjectPandaNode = DCAST(PandaNode, p_list[pi++]);
	mOwnerObject = NodePath::any_path(ownerObjectPandaNode);

	///Crowd agents.
	{
		pvector<PT(RNCrowdAgent)>::iterator iter;
		for (iter = mCrowdAgents.begin(); iter != mCrowdAgents.end(); ++iter)
		{
			(*iter) = DCAST(RNCrowdAgent, p_list[pi++]);
		}
	}

	/// Obstacles list.
	{
		pvector<Obstacle>::iterator iter;
		for (iter = mObstacles.begin(); iter != mObstacles.end(); ++iter)
		{
			PT(PandaNode)realPandaNode = DCAST(PandaNode, p_list[pi++]);
			(*iter) = Obstacle(-1, NodePath::any_path(realPandaNode));
		}
	}

	//reset mNavMeshType: a setup will be needed any way
	mNavMeshType = NULL;

	return pi;
}

/**
 * This function is called by the BamReader's factory when a new object of
 * type RNNavMesh is encountered in the Bam file.  It should create the
 * RNNavMesh and extract its information from the file.
 */
TypedWritable *RNNavMesh::make_from_bam(const FactoryParams &params)
{
	// go on only if RNNavMeshManager exists
	nassertr_always(RNNavMeshManager::get_global_ptr(), NULL)

	// create a RNNavMesh with default parameters' values: they'll be restored later
	RNNavMeshManager::get_global_ptr()->set_parameters_defaults(
			RNNavMeshManager::NAVMESH);
	RNNavMesh *node = DCAST(RNNavMesh,
			RNNavMeshManager::get_global_ptr()->create_nav_mesh().node());

	DatagramIterator scan;
	BamReader *manager;

	parse_params(params, scan, manager);
	node->fillin(scan, manager);

	return node;
}

/**
 * This internal function is called by make_from_bam to read in all of the
 * relevant data from the BamFile for the new RNNavMesh.
 */
void RNNavMesh::fillin(DatagramIterator &scan, BamReader *manager)
{
	PandaNode::fillin(scan, manager);

	///Name of this RNNavMesh.
	set_name(scan.get_string());

	///Current underlying NavMeshType type.
	mNavMeshTypeEnum = (RNNavMeshTypeEnum) scan.get_uint8();

	///RNNavMesh's NavMeshSettings equivalent.
	mNavMeshSettings.read_datagram(scan);

	///RNNavMesh's NavMeshTileSettings equivalent.
	mNavMeshTileSettings.read_datagram(scan);

	///Area types with ability flags settings (see support/NavMeshType.h).
	unsigned int size = scan.get_uint32();
	for (unsigned int i = 0; i < size; ++i)
	{
		int area = scan.get_int32();
		int flags = scan.get_int32();
		// insert into mPolyAreaFlags
		mPolyAreaFlags[area] = flags;
	}

	///Area types with cost settings (see support/NavMeshType.h).
	size = scan.get_uint32();
	for (unsigned int i = 0; i < size; ++i)
	{
		int area = scan.get_int32();
		float cost = scan.get_stdfloat();
		// insert into mPolyAreaCost
		mPolyAreaCost[area] = cost;
	}

	///Crowd include & exclude flags settings (see library/DetourNavMeshQuery.h).
	mCrowdIncludeFlags = scan.get_int32();
	mCrowdExcludeFlags = scan.get_int32();

	///Convex volumes (see support/ConvexVolumeTool.h).
	size = scan.get_uint32();
	for (unsigned int i = 0; i < size; ++i)
	{
		// restore this PointListArea
		ValueList<LPoint3f> pointList;
		unsigned int sizeP = scan.get_uint32();
		for (unsigned int i = 0; i < sizeP; ++i)
		{
			LPoint3f point;
			point.read_datagram(scan);
			pointList.add_value(point);
		}
		RNConvexVolumeSettings settings;
		settings.read_datagram(scan);
		// insert into mConvexVolumes
		mConvexVolumes.push_back(
				PointListConvexVolumeSettings(pointList, settings));
	}

	///Off mesh connections (see support/OffMeshConnectionTool.h).
	size = scan.get_uint32();
	for (unsigned int i = 0; i < size; ++i)
	{
		// restore this point pair
		ValueList<LPoint3f> pointPair;
		LPoint3f point;
		point.read_datagram(scan);
		pointPair.add_value(point);
		point.read_datagram(scan);
		pointPair.add_value(point);
		RNOffMeshConnectionSettings settings;
		settings.read_datagram(scan);
		// insert into mOffMeshConnections
		mOffMeshConnections.push_back(
				PointPairOffMeshConnectionSettings(pointPair, settings));
	}

	///Unique ref.
	mRef = scan.get_int32();

	/// Pointers
	///The owner object NodePath this RNNavMesh is associated to.
	manager->read_pointer(scan);

	///Crowd agents.
	//resize mCrowdAgents: will be restored in complete_pointers()
	size = scan.get_uint32();
	mCrowdAgents.resize(size);
	{
		for (unsigned int i = 0; i < mCrowdAgents.size(); ++i)
		{
			manager->read_pointer(scan);
		}
	}

	/// Obstacles list.
	//resize mObstacles: will be restored in complete_pointers()
	size = scan.get_uint32();
	mObstacles.resize(size);
	{
		for (unsigned int i = 0; i < mObstacles.size(); ++i)
		{
			manager->read_pointer(scan);
		}
	}
}

//TypedObject semantics: hardcoded
TypeHandle RNNavMesh::_type_handle;

