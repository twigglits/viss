set(SOURCES_SIMPACT
	personimpl.cpp
	start.cpp
	../program-common/coarsemap.cpp
	../program-common/person.cpp
	../program-common/person_relations.cpp
	../program-common/person_hiv.cpp
	../program-common/person_hsv2.cpp
	../program-common/logsystem.cpp
	../program-common/simpactpopulation.cpp
	../program-common/eventmortalitybase.cpp
	../program-common/eventmortality.cpp
	../program-common/eventaidsmortality.cpp
	../program-common/eventformation.cpp
	../program-common/eventdissolution.cpp
	../program-common/eventdebut.cpp
	../program-common/eventhivtransmission.cpp
	../program-common/eventhsv2transmission.cpp
	../program-common/eventchronicstage.cpp
	../program-common/eventseedbase.cpp
	../program-common/eventhivseed.cpp
	../program-common/eventhsv2seed.cpp
	../program-common/eventintervention.cpp
	../program-common/eventaidsstage.cpp
	../program-common/eventconception.cpp
	../program-common/eventbirth.cpp
	../program-common/eventdiagnosis.cpp
	../program-common/eventmonitoring.cpp
	../program-common/eventdropout.cpp
	../program-common/eventperiodiclogging.cpp
	../program-common/eventsyncpopstats.cpp
	../program-common/eventsyncrefyear.cpp
	../program-common/eventcheckstopalgorithm.cpp
	../program-common/eventrelocation.cpp
	../program-common/vspmodellogweibullwithnoise.cpp
	../program-common/vspmodellogdist.cpp
	../program-common/simpactevent.cpp
	../program-common/evthazardformationsimple.cpp
	../program-common/evthazardformationagegap.cpp
	../program-common/evthazardformationagegaprefyear.cpp
	../program-common/evthazarddissolution.cpp
	../program-common/hazardfunctionformationsimple.cpp
	../program-common/hazardfunctionformationagegap.cpp
	../program-common/hazardfunctionformationagegaprefyear.cpp
	../program-common/main_hazardtest.cpp
	../program-common/main.cpp
	../program-common/signalhandlers.cpp
	../program-common/configutil.cpp
	../program-common/aidstodutil.cpp
	../program-common/configsettingslog.cpp
	../program-common/eventcircum.cpp
	)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/../program-common/")
add_simpact_executable(viss ${SOURCES_SIMPACT})
install_simpact_executable(viss)
