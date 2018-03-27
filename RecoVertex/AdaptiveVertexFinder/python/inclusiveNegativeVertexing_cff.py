import FWCore.ParameterSet.Config as cms

from RecoVertex.AdaptiveVertexFinder.inclusiveCandidateVertexFinder_cfi import *
from RecoVertex.AdaptiveVertexFinder.candidateVertexMerger_cfi import *
from RecoVertex.AdaptiveVertexFinder.candidateVertexArbitrator_cfi import *


inclusiveCandidateNegativeVertexFinder = inclusiveCandidateVertexFinder.clone(
   vertexMinAngleCosine = cms.double(-0.95) 
)
inclusiveCandidateNegativeVertexFinder.clusterizer.clusterMinAngleCosine = cms.double(-0.5)

candidateNegativeVertexMerger = candidateVertexMerger.clone()
candidateNegativeVertexMerger.secondaryVertices = cms.InputTag("inclusiveCandidateNegativeVertexFinder")

candidateNegativeVertexArbitrator = candidateVertexArbitrator.clone()
candidateNegativeVertexArbitrator.secondaryVertices = cms.InputTag("candidateNegativeVertexMerger")
candidateNegativeVertexArbitrator.dRCut = cms.double(-0.4)

inclusiveCandidateNegativeSecondaryVertices = candidateVertexMerger.clone()
inclusiveCandidateNegativeSecondaryVertices.secondaryVertices = cms.InputTag("candidateNegativeVertexArbitrator")
inclusiveCandidateNegativeSecondaryVertices.maxFraction = 0.2
inclusiveCandidateNegativeSecondaryVertices.minSignificance = 10.



inclusiveCandidateNegativeVertexingTask = cms.Task(inclusiveCandidateNegativeVertexFinder,
                                           candidateNegativeVertexMerger,
                                           candidateNegativeVertexArbitrator,
                                           inclusiveCandidateNegativeSecondaryVertices)

inclusiveCandidateNegativeVertexing = cms.Sequence(inclusiveCandidateNegativeVertexingTask)
