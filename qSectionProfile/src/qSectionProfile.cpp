#include "qSectionProfile.h"

#include "SectionProfileDialog.h"

#include <ccHObjectCaster.h>
#include <ccMainAppInterface.h>
#include <ccPointCloud.h>
#include <ccPolyline.h>

#include <QIcon>

qSectionProfile::qSectionProfile(QObject* parent)
	: QObject(parent)
	, ccStdPluginInterface(":/CC/plugin/qSectionProfile/info.json")
{
}

QList<QAction*> qSectionProfile::getActions()
{
	if (!m_action)
	{
		m_action = new QAction(QIcon(":/CC/plugin/qSectionProfile/images/section_profile.svg"),
		                       tr("Section profile from polyline"),
		                       this);
		m_action->setToolTip(tr("Build a section profile from one selected polyline and one selected cloud"));
		connect(m_action, &QAction::triggered, this, &qSectionProfile::doAction);
	}

	m_action->setEnabled(findSelection());
	return { m_action };
}

void qSectionProfile::onNewSelection(const ccHObject::Container& selectedEntities)
{
	m_selectedEntities = selectedEntities;
	if (m_action)
	{
		m_action->setEnabled(findSelection());
	}
}

bool qSectionProfile::findSelection() const
{
	unsigned polylineCount = 0;
	unsigned cloudCount = 0;

	for (ccHObject* entity : m_selectedEntities)
	{
		if (ccHObjectCaster::ToPolyline(entity))
		{
			++polylineCount;
		}
		if (ccHObjectCaster::ToPointCloud(entity))
		{
			++cloudCount;
		}
	}

	return polylineCount == 1 && cloudCount == 1;
}

void qSectionProfile::doAction()
{
	if (!m_app)
	{
		return;
	}

	const ccPolyline* polyline = nullptr;
	const ccPointCloud* cloud = nullptr;

	for (ccHObject* entity : m_selectedEntities)
	{
		if (!polyline)
		{
			polyline = ccHObjectCaster::ToPolyline(entity);
		}
		if (!cloud)
		{
			cloud = ccHObjectCaster::ToPointCloud(entity);
		}
	}

	if (!polyline || !cloud)
	{
		m_app->dispToConsole(tr("Select exactly one polyline and one point cloud."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	if (polyline->size() < 2 || cloud->size() == 0)
	{
		m_app->dispToConsole(tr("The selected polyline needs at least two vertices and the cloud must not be empty."), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	SectionProfileDialog dialog(*polyline, *cloud, m_app->getMainWindow());
	dialog.exec();
}
