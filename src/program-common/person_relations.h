#ifndef PERSON_RELATIONS_H

#define PERSON_RELATIONS_H

#include "personbase.h"
#include <assert.h>
#include <vector>
#include <set>

class Person;
class ConfigSettings;
class ConfigWriter;
class GslRandomNumberGenerator;
class ProbabilityDistribution;
class ProbabilityDistribution2D;

class Person_Relations
{
public:
	Person_Relations(const Person *pSelf);
	~Person_Relations();

	// This also resets the iterator for getNextRelationshipPartner
	int getNumberOfRelationships() const														{ return m_relationshipsSet.size(); }
	void startRelationshipIteration();
	Person *getNextRelationshipPartner(double &formationTime);
	int getNumberOfDiagnosedPartners();

	bool hasRelationshipWith(Person *pPerson) const;

	// WARNING: do not use these during relationship iteration
	void addRelationship(Person *pPerson, double t);
	void removeRelationship(Person *pPerson, double t, bool deathBased);
	
	// result is negative if no relations formed yet
	double getLastRelationshipChangeTime() const												{ return m_lastRelationChangeTime; }

	void setSexuallyActive(double t)															{ m_sexuallyActive = true; assert(t >= 0); m_debutTime = t; }
	bool isSexuallyActive()	const																{ return m_sexuallyActive;}
	double getDebutTime() const																	{ return m_debutTime; }

	double getFormationEagernessParameter() const												{ return m_formationEagernessHetero; }
	double getPreferredAgeDifference() const													{ assert(m_preferredAgeDiffHetero < 200.0 && m_preferredAgeDiffHetero > -200.0); return m_preferredAgeDiffHetero; }

	double getFormationEagernessParameterMSM() const											{ return m_formationEagernessHomo; }
	double getPreferredAgeDifferenceMSM() const													{ assert(m_preferredAgeDiffHomo < 200.0 && m_preferredAgeDiffHomo > -200.0); return m_preferredAgeDiffHomo; }

	// NOTE: this ignores the call if already in the list
	void addPersonOfInterest(Person *pPerson);
	void removePersonOfInterest(Person *pPerson);
	void clearPersonsOfInterest()																{ m_personsOfInterest.clear(); }
	int getNumberOfPersonsOfInterest() const													{ return (int)m_personsOfInterest.size(); }
	Person *getPersonOfInterest(int idx) const													{ assert(idx >= 0 && idx < (int)m_personsOfInterest.size()); Person *pPerson = m_personsOfInterest[idx]; assert(pPerson); return pPerson; }

	static void writeToRelationLog(const Person *pMan, const Person *pWoman, double formationTime, double dissolutionTime);
	static void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen);
	static void obtainConfig(ConfigWriter &config);
private:
	class Relationship
	{
	public:
		// A negative time is used when not relevant, e.g. when
		// searching for a relationship with a person
		Relationship(Person *pPerson, double formationTime)				{ assert(pPerson != 0); assert(formationTime >= 0); m_pPerson = pPerson; m_formationTime = formationTime; }
		Relationship(Person *pPerson)							{ assert(pPerson != 0); m_pPerson = pPerson; m_formationTime = -1; }

		Person *getPartner() const							{ return m_pPerson; }
		double getFormationTime() const							{ return m_formationTime; }

		bool operator<(const Relationship &rel) const
		{
			PersonBase *p1 = reinterpret_cast<PersonBase *>(m_pPerson);
			PersonBase *p2 = reinterpret_cast<PersonBase *>(rel.m_pPerson);
			if (p1->getPersonID() < p2->getPersonID()) 
				return true; 
			return false; 
		}
	private:
		Person *m_pPerson;
		double m_formationTime;
	};

#ifndef NDEBUG
	bool m_relIterationBusy;
#endif
	const Person *m_pSelf;

	std::set<Relationship> m_relationshipsSet;
	std::set<Relationship>::const_iterator m_relationshipsIterator;
	double m_lastRelationChangeTime;
	bool m_sexuallyActive;
	double m_debutTime;

	double m_formationEagernessHetero;
	double m_preferredAgeDiffHetero;

	double m_formationEagernessHomo;
	double m_preferredAgeDiffHomo;

	std::vector<Person *> m_personsOfInterest;

	struct EagernessAndAgegap
	{
		EagernessAndAgegap();
		~EagernessAndAgegap();

		bool m_independentEagerness;
		ProbabilityDistribution *m_pEagHetero;
		ProbabilityDistribution *m_pEagHomo;
		ProbabilityDistribution2D *m_pEagJoint;

		ProbabilityDistribution *m_pGapHetero;
		ProbabilityDistribution *m_pGapHomo;

		void processConfig(ConfigSettings &config, GslRandomNumberGenerator *pRndGen,
		                            const std::string &prefixEag, const std::string &prefixGap,
									const std::string &homSuff);
		void obtainConfig(ConfigWriter &config, const std::string &prefixEag, 
		                           const std::string &prefixGap, const std::string &homSuff);
	};

	void pickEagernessAndGap(const EagernessAndAgegap &e);

	static EagernessAndAgegap m_eagAgeMan;
	static EagernessAndAgegap m_eagAgeWoman;
};

inline bool Person_Relations::hasRelationshipWith(Person *pPerson) const
{
	return m_relationshipsSet.find(Relationship(pPerson)) != m_relationshipsSet.end();
}

#endif // PERSON_RELATIONS_H
