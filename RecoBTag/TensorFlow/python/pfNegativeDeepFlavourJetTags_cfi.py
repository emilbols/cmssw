import FWCore.ParameterSet.Config as cms

from RecoBTag.TensorFlow.pfDeepFlavourJetTags_cfi import pfDeepFlavourJetTags
from RecoBTag.TensorFlow.pfDeepFlavourPrunedJetTags_cfi import pfDeepFlavourPrunedJetTags

pfNegativeDeepFlavourJetTags = pfDeepFlavourJetTags.clone(
        src = cms.InputTag('pfNegativeDeepFlavourTagInfos')
        )

pfNegativeDeepFlavourPrunedJetTags = pfDeepFlavourPrunedJetTags.clone(
        src = cms.InputTag('pfNegativeDeepFlavourTagInfos')
        )
