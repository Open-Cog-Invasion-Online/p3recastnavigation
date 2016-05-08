/**
 * \file rnNavMeshManager.cxx
 *
 * \date 2016-03-29
 * \author consultit
 */

#include "../rnNavMeshManager.h"

#include "../rnCrowdAgent.h"
#include "../rnNavMesh.h"
#include "asyncTaskManager.h"
#include "bamFile.h"

///RNNavMeshManager
/**
 *
 */
RNNavMeshManager::RNNavMeshManager(const NodePath& root,
		const CollideMask& mask) :
		mRoot(root), mMask(mask), mCollisionHandler(nullptr), mPickerRay(nullptr), mCTrav(
		nullptr)
{
	PRINT_DEBUG(
			"RNNavMeshManager::RNNavMeshManager: creating the singleton manager.");

	set_parameters_defaults(NAVMESH);
	set_parameters_defaults(CROWDAGENT);
	//
	mUpdateData.clear();
	mUpdateTask.clear();
	//
	if (not mRoot.is_empty())
	{
		mCTrav = new CollisionTraverser();
		mCollisionHandler = new CollisionHandlerQueue();
		mPickerRay = new CollisionRay();
		PT(CollisionNode)pickerNode = new CollisionNode(string("RNNavMeshManager::pickerNode"));
		pickerNode->add_solid(mPickerRay);
		pickerNode->set_from_collide_mask(mMask);
		pickerNode->set_into_collide_mask(BitMask32::all_off());
		mCTrav->add_collider(mRoot.attach_new_node(pickerNode),
				mCollisionHandler);
	}
}

/**
 *
 */
RNNavMeshManager::~RNNavMeshManager()
{
	PRINT_DEBUG("RNNavMeshManager::~RNNavMeshManager: destroying the singleton manager.");

	//stop any default update
	stop_default_update();
	//destroy all RNCrowdAgents
	PTA(PT(RNCrowdAgent))::iterator iterC = mCrowdAgents.begin();
	while (iterC != mCrowdAgents.end())
	{
		//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
		//give a chance to RNCrowdAgent to cleanup itself before being destroyed.
		(*iterC)->do_finalize();
		//remove the RNCrowdAgents from the inner list (and from the update task)
		mCrowdAgents.erase(iterC);
	}

	//destroy all RNNavMeshes
	PTA(PT(RNNavMesh))::iterator iterN = mNavMeshes.begin();
	while (iterN != mNavMeshes.end())
	{
		//\see http://stackoverflow.com/questions/596162/can-you-remove-elements-from-a-stdlist-while-iterating-through-it
		//give a chance to RNCrowdAgent to cleanup itself before being destroyed.
		(*iterN)->do_finalize();
		//remove the RNCrowdAgents from the inner list (and from the update task)
		mNavMeshes.erase(iterN);
	}

	//clear parameters' tables
	mNavMeshesParameterTable.clear();
	mCrowdAgentsParameterTable.clear();

	//
	delete mCTrav;
}

/**
 * Creates a RNNavMesh.
 */
NodePath RNNavMeshManager::create_nav_mesh()
{
	PT(RNNavMesh) newNavMesh = new RNNavMesh();
	nassertr_always(newNavMesh, NodePath())

	//initialize the new NavMesh
	newNavMesh->do_initialize();

	//add the new NavMesh to the inner list (and to the update task)
	mNavMeshes.push_back(newNavMesh);
	//
	return NodePath::any_path(newNavMesh);
}

/**
 * Destroys a RNNavMesh.
 */
bool RNNavMeshManager::destroy_nav_mesh(NodePath navMeshNP)
{
	nassertr_always(navMeshNP.node()->is_of_type(RNNavMesh::get_class_type()),
			false)

	PT(RNNavMesh) navMesh = DCAST(RNNavMesh, navMeshNP.node());
	NavMeshList::iterator iter = find(mNavMeshes.begin(), mNavMeshes.end(),
			navMesh);
	nassertr_always(iter != mNavMeshes.end(), false)

	//give a chance to NavMesh to cleanup itself before being destroyed.
	navMesh->do_finalize();

	//remove the NavMesh from the inner list (and from the update task)
	mNavMeshes.erase(iter);
	//
	return true;
}

/**
 * Gets a RNNavMesh by index.
 */
NodePath RNNavMeshManager::get_nav_mesh(int index) const
{
	nassertr_always((index >= 0) and (index < (int ) mNavMeshes.size()), NodePath());

	return NodePath::any_path(mNavMeshes[index]);
}

/**
 * Creates a RNCrowdAgent with a given (not empty) name.
 */
NodePath RNNavMeshManager::create_crowd_agent(const string& name)
{
	nassertr_always(not name.empty(), NodePath())

	PT(RNCrowdAgent) newCrowdAgent = new RNCrowdAgent(name);
	nassertr_always(newCrowdAgent, NodePath())

	//initialize the new CrowdAgent
	newCrowdAgent->do_initialize();

	//add the new CrowdAgent to the inner list
	mCrowdAgents.push_back(newCrowdAgent);
	//
	return NodePath::any_path(newCrowdAgent);
}

/**
 * Destroys a RNCrowdAgent.
 */
bool RNNavMeshManager::destroy_crowd_agent(NodePath crowdAgentNP)
{
	nassertr_always(
			crowdAgentNP.node()->is_of_type(RNCrowdAgent::get_class_type()),
			false)

	PT(RNCrowdAgent)crowdAgent = DCAST(RNCrowdAgent, crowdAgentNP.node());
	CrowdAgentList::iterator iter = find(mCrowdAgents.begin(),
			mCrowdAgents.end(), crowdAgent);
	nassertr_always(iter != mCrowdAgents.end(), false)

	//give a chance to CrowdAgent to cleanup itself before being destroyed.
	crowdAgent->do_finalize();
	//remove the CrowdAgent from the inner list (and from the update task)
	mCrowdAgents.erase(iter);
	//
	return true;
}

/**
 * Gets a RNCrowdAgent by index.
 */
NodePath RNNavMeshManager::get_crowd_agent(int index) const
{
	nassertr_always((index >= 0) and (index < (int ) mCrowdAgents.size()), NodePath());

	return NodePath::any_path(mCrowdAgents[index]);
}

/**
 * Sets a RNNavMeshManager's parameter to custom values.
 * This function sets a parameter to custom values which
 * overwrite the existing ones.
 */
void RNNavMeshManager::set_parameter_values(RNType type, const string& paramName,
		const ValueListString& paramValues)
{
	pair<ParameterTableIter, ParameterTableIter> iterRange;
	if (type == NAVMESH)
	{
		//find from mParameterTable the paramName's values to be overwritten
		iterRange = mNavMeshesParameterTable.equal_range(paramName);
		//...and erase them
		mNavMeshesParameterTable.erase(iterRange.first, iterRange.second);
		//insert the new values
		for (int idx = 0; idx < paramValues.size(); ++idx)
		{
			mNavMeshesParameterTable.insert(
					ParameterNameValue(paramName, paramValues[idx]));
		}
	}
	else if (type == CROWDAGENT)
	{
		//find from mParameterTable the paramName's values to be overwritten
		iterRange = mCrowdAgentsParameterTable.equal_range(paramName);
		//...and erase them
		mCrowdAgentsParameterTable.erase(iterRange.first, iterRange.second);
		//insert the new values
		for (int idx = 0; idx < paramValues.size(); ++idx)
		{
			mCrowdAgentsParameterTable.insert(
					ParameterNameValue(paramName, paramValues[idx]));
		}
	}
}

/**
 * Gets the multiple values of a (actually set) parameter of the RNNavMeshManager.
 */
ValueListString RNNavMeshManager::get_parameter_values(RNType type, const string& paramName)
{
	ValueListString strList;
	ParameterTableIter iter;
	pair<ParameterTableIter, ParameterTableIter> iterRange;
	if (type == NAVMESH)
	{
		iterRange = mNavMeshesParameterTable.equal_range(paramName);
		if (iterRange.first != iterRange.second)
		{
			for (iter = iterRange.first; iter != iterRange.second; ++iter)
			{
				strList.add_value(iter->second);
			}
		}
	}
	else if (type == CROWDAGENT)
	{
		iterRange = mCrowdAgentsParameterTable.equal_range(paramName);
		if (iterRange.first != iterRange.second)
		{
			for (iter = iterRange.first; iter != iterRange.second; ++iter)
			{
				strList.add_value(iter->second);
			}
		}
	}
	//
	return strList;
}

/**
 * Sets the single value (i.e. the first one) of a parameter of the RNNavMesh.
 */
void RNNavMeshManager::set_parameter_value(RNType type, const string& paramName, const string& value)
{
	ValueListString valueList;
	valueList.add_value(value);
	set_parameter_values(type, paramName, valueList);
}

/**
 * Gets the single value (i.e. the first one) of a parameter of the RNNavMesh.
 */
string RNNavMeshManager::get_parameter_value(RNType type, const string& paramName)
{
	ValueListString valueList = get_parameter_values(type, paramName);
	return (valueList.size() != 0 ? valueList[0] : string(""));
}

/**
 * Gets a list of the names of the parameters actually set into the RNNavMeshManager.
 */
ValueListString RNNavMeshManager::get_parameter_name_list(RNType type)
{
	ValueListString strList;
	ParameterTableIter iter;
	if (type == NAVMESH)
	{
		for (iter = mNavMeshesParameterTable.begin();
				iter != mNavMeshesParameterTable.end(); ++iter)
		{
			string name = (*iter).first;
			if (not strList.has_value(name))
			{
				strList.add_value(name);
			}
		}
	}
	else if (type == CROWDAGENT)
	{
		for (iter = mCrowdAgentsParameterTable.begin();
				iter != mCrowdAgentsParameterTable.end(); ++iter)
		{
			string name = (*iter).first;
			if (not strList.has_value(name))
			{
				strList.add_value(name);
			}
		}

	}
	//
	return strList;
}

/**
 * Sets the RNNavMeshManager parameters to their default values (if any).
 */
void RNNavMeshManager::set_parameters_defaults(RNType type)
{
	if (type == NAVMESH)
	{
		///mNavMeshesParameterTable must be the first cleared
		mNavMeshesParameterTable.clear();
		//sets the (mandatory) parameters to their default values:
		mNavMeshesParameterTable.insert(
				ParameterNameValue("navmesh_type", "solo"));
		mNavMeshesParameterTable.insert(ParameterNameValue("cell_size", "0.3"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("cell_height", "0.2"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("agent_height", "2.0"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("agent_radius", "0.6"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("agent_max_climb", "0.9"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("agent_max_slope", "45.0"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("region_min_size", "8"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("region_merge_size", "20"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("partition_type", "watershed"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("edge_max_len", "12.0"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("edge_max_error", "1.3"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("verts_per_poly", "6.0"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("detail_sample_dist", "6.0"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("detail_sample_max_error", "1.0"));
		//nav mesh tile
		mNavMeshesParameterTable.insert(
				ParameterNameValue("build_all_tiles", "false"));
		mNavMeshesParameterTable.insert(ParameterNameValue("max_tiles", "128"));
		mNavMeshesParameterTable.insert(
				ParameterNameValue("max_polys_per_tile", "32768"));
		mNavMeshesParameterTable.insert(ParameterNameValue("tile_size", "32"));
		//area flags cost
		//NAVMESH_POLYAREA_GROUND@NAVMESH_POLYFLAGS_WALK@1.0
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "0@0x01@1.0"));
		//NAVMESH_POLYAREA_WATER@NAVMESH_POLYFLAGS_SWIM@10.0
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "1@0x02@10.0"));
		//NAVMESH_POLYAREA_ROAD@NAVMESH_POLYFLAGS_WALK@1.0
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "2@0x01@1.0"));
		//NAVMESH_POLYAREA_DOOR@NAVMESH_POLYFLAGS_WALK:NAVMESH_POLYFLAGS_DOOR@1.0
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "3@0x01:0x04@1.0"));
		//NAVMESH_POLYAREA_GRASS@NAVMESH_POLYFLAGS_WALK@2.0
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "4@0x01@2.0"));
		//NAVMESH_POLYAREA_JUMP@NAVMESH_POLYFLAGS_JUMP@1.5
		mNavMeshesParameterTable.insert(ParameterNameValue("area_flags_cost", "5@0x08@1.5"));
		//crowd include flags = NAVMESH_POLYFLAGS_ALL ^ NAVMESH_POLYFLAGS_DISABLED = 0xffef
		mNavMeshesParameterTable.insert(ParameterNameValue("crowd_include_flags", "0xffef"));
		//crowd exclude flags = NAVMESH_POLYFLAGS_DISABLED = 0x10
		mNavMeshesParameterTable.insert(ParameterNameValue("crowd_exclude_flags", "0x10"));
	}
	else if (type == CROWDAGENT)
	{
		///mCrowdAgentsParameterTable must be the first cleared
		mCrowdAgentsParameterTable.clear();
		//sets the (mandatory) parameters to their default values:
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("add_to_navmesh", ""));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("mov_type", "recast"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("move_target", "0.0,0.0,0.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("move_velocity", "0.0,0.0,0.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("max_acceleration", "8.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("max_speed", "3.5"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("collision_query_range", "12.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("path_optimization_range", "30.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("separation_weight", "2.0"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("update_flags", "0x1b"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("obstacle_avoidance_type", "3"));
		mCrowdAgentsParameterTable.insert(
				ParameterNameValue("ray_mask", "all_on"));
	}
}

/**
 * Updates RNNavMeshes and their RNCrowdAgents.
 *
 * Will be called automatically in a task.
 */
AsyncTask::DoneStatus RNNavMeshManager::update(GenericAsyncTask* task)
{
	float dt = ClockObject::get_global_clock()->get_dt();

#ifdef TESTING
	dt = 0.016666667; //60 fps
#endif

	// call all audio components update functions, passing delta time
	for (PTA(PT(RNNavMesh))::size_type index = 0; index < mNavMeshes.size();	++index)
	{
		mNavMeshes[index]->update(dt);
	}
	//
	return AsyncTask::DS_cont;
}

/**
 * Adds a task to repeatedly call RNNavMeshes' updates.
 */
void RNNavMeshManager::start_default_update()
{
	//create the task for updating AI components
	mUpdateData = new TaskInterface<RNNavMeshManager>::TaskData(this,
			&RNNavMeshManager::update);
	mUpdateTask = new GenericAsyncTask(string("RNNavMeshManager::update"),
			&TaskInterface<RNNavMeshManager>::taskFunction,
			reinterpret_cast<void*>(mUpdateData.p()));
	//Adds mUpdateTask to the active queue.
	AsyncTaskManager::get_global_ptr()->add(mUpdateTask);
}

/**
 * Removes a task to repeatedly call RNNavMeshes' updates.
 */
void RNNavMeshManager::stop_default_update()
{
	if (mUpdateTask)
	{
		AsyncTaskManager::get_global_ptr()->remove(mUpdateTask);
	}
	//
	mUpdateData.clear();
	mUpdateTask.clear();
}


/**
 * Gets bounding dimensions of a model NodePath.
 * Puts results into the out parameters: modelDims, modelDeltaCenter and returns
 * modelRadius.
 * - modelDims = absolute dimensions of the model
 * - modelCenter + modelDeltaCenter = origin of coordinate system
 * - modelRadius = radius of the containing sphere
 */
float RNNavMeshManager::get_bounding_dimensions(NodePath modelNP,
		LVecBase3f& modelDims, LVector3f& modelDeltaCenter)
{
	//get "tight" dimensions of model
	LPoint3f minP, maxP;
	modelNP.calc_tight_bounds(minP, maxP);
	//
	LVecBase3 delta = maxP - minP;
	LVector3f deltaCenter = -(minP + delta / 2.0);
	//
	modelDims.set(abs(delta.get_x()), abs(delta.get_y()), abs(delta.get_z()));
	modelDeltaCenter.set(deltaCenter.get_x(), deltaCenter.get_y(),
			deltaCenter.get_z());
	float modelRadius = max(max(modelDims.get_x(), modelDims.get_y()),
			modelDims.get_z()) / 2.0;
	return modelRadius;
}

/**
 * Throws a ray downward (-z direction default) from rayOrigin.
 * If collisions are found returns a PairBoolFloat == (true, height),
 * with height equal to the z-value of the first one.
 * If collisions are not found returns a PairBoolFloat == (false, 0.0).
 */
PairBoolFloat RNNavMeshManager::get_collision_height(const LPoint3f& rayOrigin,
		const NodePath& space)
{
	//traverse downward starting at rayOrigin
	mPickerRay->set_direction(LVecBase3f(0.0, 0.0, -1.0));
	mPickerRay->set_origin(rayOrigin);
	mCTrav->traverse(mRoot);
	if (mCollisionHandler->get_num_entries() > 0)
	{
		mCollisionHandler->sort_entries();
		CollisionEntry *entry0 =
				mCollisionHandler->get_entry(0);
		LPoint3f target = entry0->get_surface_point(space);
		float collisionHeight = target.get_z();
		return PairBoolFloat(true, collisionHeight);
	}
	//
	return PairBoolFloat(false, 0.0);
}

/**
 * Writes to a bam file the entire collections of nav meshes, crowd agents
 * and related geometries (i.e. models' NodePaths)
 */
bool RNNavMeshManager::write_to_bam_file(const string& fileName)
{
	string errorReport;
	// write to bam file
	BamFile outBamFile;
	if (outBamFile.open_write(Filename(fileName)))
	{
		cout << "Current system Bam version: " << outBamFile.get_current_major_ver() << "."
				<< outBamFile.get_current_minor_ver() << endl;
		BamWriter* manager = outBamFile.get_writer();
		DatagramSink* dgSink = manager->get_target();
		Datagram dg;

		//save nav meshes' number
		unsigned int navMeshNum = mNavMeshes.size();
		dg.add_uint32(navMeshNum);
		dgSink->put_datagram(dg);
		//for each nav mesh do:
		for (auto& navMesh : mNavMeshes)
		{
			//current underlying NavMeshType: used as flag for setup()
			dg.clear();
			&navMesh->get_nav_mesh_type() != nullptr ? dg.add_bool(true) : dg.add_bool(false);
			dgSink->put_datagram(dg);
			//write the the nav mesh
			if (not outBamFile.write_object(navMesh))
			{
				errorReport += string("Error writing ")
						+ string(navMesh->get_name()) + string("\n");
			}
		}
		// close the file
		outBamFile.close();
	}
	else
	{
		errorReport += string("\nERROR: cannot open ") + fileName;
	}
	//check
	if (errorReport.empty())
	{
		cout
				<< "SUCCESS: all nav mesh and crowd agent collections were written to "
				<< fileName << endl;
	}
	else
	{
		cerr << errorReport << endl;
	}
	return errorReport.empty();
}

/**
 * Reads from a bam file the entire hierarchy of nav meshes, crowd agents
 * and related geometries (i.e. models' NodePaths)
 */
bool RNNavMeshManager::read_from_bam_file(const string& fileName)
{
	string errorReport;
	//read from bamFile
	BamFile inBamFile;
	if (inBamFile.open_read(Filename(fileName)))
	{
		cout << "Current system Bam version: " << inBamFile.get_current_major_ver() << "."
				<< inBamFile.get_current_minor_ver() << endl;
		cout << "Bam file version: " << inBamFile.get_file_major_ver() << "."
				<< inBamFile.get_file_minor_ver() << endl;
		BamReader* manager = inBamFile.get_reader();
		DatagramGenerator* dgGenerator = manager->get_source();
		Datagram dg;

		//read the nav meshes' number
		dgGenerator->get_datagram(dg);
		DatagramIterator scan(dg);
		unsigned int navMeshNum = scan.get_uint32();
		//for each nav mesh do:
		for (unsigned int i = 0; i < navMeshNum; ++i)
		{
			//read the flag for setup()
			dgGenerator->get_datagram(dg);
			scan.assign(dg);
			bool setupNavMesh = scan.get_bool();
			//read the nav mesh
			TypedWritable* navMesh = inBamFile.read_object();
			if (navMesh)
			{
				//resolve pointers
				if (inBamFile.resolve())
				{
					//setup if requested
					if (setupNavMesh)
					{
						DCAST(RNNavMesh, navMesh)->setup();
					}
				}
				else
				{
					errorReport += string(
							"Error resolving pointers for nav mesh ") + str(i)
							+ string("\n");
				}
			}
			else
			{
				errorReport += string("Error reading nav mesh ") + str(i)
						+ string("\n");
			}
		}
		// close the file
		inBamFile.close();
	}
	else
	{
		errorReport += string("\nERROR: cannot open ") + fileName;
	}
	//check
	if (errorReport.empty())
	{
		cout << "SUCCESS: all nav meshes and crowd agents were read from "
				<< fileName << endl;
	}
	else
	{
		cerr << errorReport << endl;
	}
	return errorReport.empty();
}

//TypedObject semantics: hardcoded
TypeHandle RNNavMeshManager::_type_handle;