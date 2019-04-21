#include "afnodefarm.h"

#include "../libafanasy/affarm.h"
#include "../libafanasy/regexp.h"

#include "action.h"
#include "monitorcontainer.h"
#include "poolsrv.h"

#define AFOUTPUT
#undef AFOUTPUT
#include "../include/macrooutput.h"
#include "../libafanasy/logger.h"

AfNodeFarm::AfNodeFarm(af::Node * i_node, af::Farm * i_farm, PoolSrv * i_parent, const std::string & i_store_dir):
	AfNodeSrv(i_node, i_store_dir),
	m_parent(i_parent),
	m_farm(i_farm)
{
}

AfNodeFarm::~AfNodeFarm()
{
}

void AfNodeFarm::actionFarm(Action & i_action)
{
	const JSON & operation = (*i_action.data)["operation"];
	std::string mode;
	if (false == af::jr_string("mode", mode, operation))
	{
		appendLog("Service operation mode is not set by " + i_action.author);
		i_action.answer_kind = "error";
		i_action.answer = "Service operation mode is not set.\
			Valid modes are: 'service_add','service_remove','service_enable','service_disable'.";
		return;
	}

	if (mode == "service_add")
	{
		std::string name;
		if (false == af::jr_string("name", name, operation))
		{
			appendLog("Service add: Service name is not specified by " + i_action.author);
			i_action.answer_kind = "error";
			i_action.answer = "Service name is not specified.";
			return;
		}

		if (name.size() == 0)
		{
			appendLog("Service add: Service name is empty by " + i_action.author);
			i_action.answer_kind = "error";
			i_action.answer = "Service name is empty.";
			return;
		}

		if (hasService(name))
		{
			appendLog("Service add: Service \"" + name + "\" already exists by " + i_action.author);
			i_action.answer_kind = "error";
			i_action.answer = "Service \"" + name + "\" already exists.";
			return;
		}

		m_farm->m_services.push_back(name);

		i_action.monitors->addEvent(af::Monitor::EVT_pools_change, m_node->getId());

		appendLog("Service \"" + name + "\" added by " + i_action.author);
		i_action.answer_kind = "info";
		i_action.answer = "Service \"" + name + "\" added.";

		return;
	}

	if ((mode == "service_remove") || (mode == "service_enable") || (mode == "service_disable"))
	{
		af::RegExp mask;
		if (false == af::jr_regexp("mask", mask, operation))
		{
			appendLog("Service add: Invalid service mask by " + i_action.author);
			i_action.answer_kind = "error";
			i_action.answer = "Invalid service mask.";
			return;
		}

		bool found = false;
		std::vector<std::string>::iterator it = m_farm->m_services.begin();
		while (it != m_farm->m_services.end())
		{
			if (mask.match(*it))
			{

				if (mode == "service_remove")
				{
					it = m_farm->m_services.erase(it);
					found = true;
					continue;
				}
//				if (mode == "service_disable")
//				{
//					if (false == isServiceDisabled(*it))
//				}
//				else if (mode == "service_enable")
//				{
//				}
			}

			it++;
		}

		if (false == found)
		{
			appendLog("No services found matching mask \"" + mask.getPattern() + "\" by " + i_action.author);
			i_action.answer_kind = "error";
			i_action.answer = "No services found mathing pattern " + mask.getPattern();
			return;
		}
	}
	else
	{
		appendLog("Unknown farm operation mode \"" + mode + "\" by " + i_action.author);
		i_action.answer_kind = "error";
		i_action.answer = "Unknown farm operation mode: " + mode;
		return;
	}
}

bool AfNodeFarm::hasService(const std::string & i_service_name) const
{
	for (const std::string & s : m_farm->m_services)
		if (s == i_service_name)
			return true;
	return false;
}

bool AfNodeFarm::isServiceDisabled(const std::string & i_service_name) const
{
	for (const std::string & s : m_farm->m_services_disabled)
		if (s == i_service_name)
			return true;
	return false;
}

bool AfNodeFarm::canRunService(const std::string & i_service_name) const
{
	if (m_farm->m_services.size() == 0)
	{
		if (m_parent)
			return m_parent->canRunService(i_service_name);
		else
			return  true;
	}

	if (hasService(i_service_name))
		if (false == isServiceDisabled(i_service_name))
			return true;

	return false;
}
