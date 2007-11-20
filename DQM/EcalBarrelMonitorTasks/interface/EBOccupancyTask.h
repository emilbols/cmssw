#ifndef EBOccupancyTask_H
#define EBOccupancyTask_H

/*
 * \file EBOccupancyTask.h
 *
 * $Date: 2007/11/10 15:01:25 $
 * $Revision: 1.8 $
 * \author G. Della Ricca
 *
*/

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

class MonitorElement;
class DaqMonitorBEInterface;

class EBOccupancyTask: public edm::EDAnalyzer{

public:

/// Constructor
EBOccupancyTask(const edm::ParameterSet& ps);

/// Destructor
virtual ~EBOccupancyTask();

protected:

/// Analyze
void analyze(const edm::Event& e, const edm::EventSetup& c);

/// BeginJob
void beginJob(const edm::EventSetup& c);

/// EndJob
void endJob(void);

/// Setup
void setup(void);

/// Cleanup
void cleanup(void);

private:

int ievt_;

DaqMonitorBEInterface* dbe_;

bool enableCleanup_;

edm::InputTag EBDigiCollection_;
edm::InputTag EcalPnDiodeDigiCollection_;

MonitorElement* meEvent_[36];
MonitorElement* meOccupancy_[36];
MonitorElement* meOccupancyMem_[36];

bool init_;

};

#endif
