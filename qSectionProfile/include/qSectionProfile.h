#pragma once

#include <ccHObject.h>
#include <ccStdPluginInterface.h>

#include <QAction>
#include <QObject>

class ccHObject;

class qSectionProfile : public QObject, public ccStdPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(ccPluginInterface ccStdPluginInterface)
	Q_PLUGIN_METADATA(IID "cccorp.cloudcompare.plugin.qSectionProfile" FILE "../info.json")

public:
	explicit qSectionProfile(QObject* parent = nullptr);
	~qSectionProfile() override = default;

	QList<QAction*> getActions() override;
	void onNewSelection(const ccHObject::Container& selectedEntities) override;

private slots:
	void doAction();

private:
	bool findSelection() const;

	QAction* m_action = nullptr;
	ccHObject::Container m_selectedEntities;
};
