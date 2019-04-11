
import FWCore.ParameterSet.Config as cms
from PhysicsTools.PatAlgos.tools.helpers import getPatAlgosToolsTask

process = cms.Process("PATtest")

## MessageLogger
process.load("FWCore.MessageLogger.MessageLogger_cfi")

## Options and Output Report
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )

## Source
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring()
)
## Maximal Number of Events
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(100) )

## Geometry and Detector Conditions (needed for a few patTuple production steps)
process.load("Configuration.Geometry.GeometryRecoDB_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag.globaltag = '94X_dataRun2_v10'
process.load("Configuration.StandardSequences.MagneticField_cff")

trkProbaCalibTag = 'JPcalib_Data94X_2016_v1'
process.GlobalTag.toGet = cms.VPSet(
    cms.PSet(record = cms.string("BTagTrackProbability3DRcd"),
      tag = cms.string(trkProbaCalibTag),
      connect = cms.string("frontier://FrontierProd/CMS_CONDITIONS")
    )
)


from CondCore.DBCommon.CondDBSetup_cfi import *
import os
dbfile='Summer16_07Aug2017All_V11_DATA'
print "\nUsing private SQLite file", dbfile, "\n"
process.jec = cms.ESSource("PoolDBESSource",CondDBSetup,
                           connect = cms.string( "sqlite_fip:RecoBTag/TensorFlow/test/"+dbfile+'.db'),
                           toGet =  cms.VPSet(
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK4PF"),
            label= cms.untracked.string("AK4PF")
            ),
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK4PFchs"),
            label= cms.untracked.string("AK4PFchs")
            ),
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK4PFPuppi"),
            label= cms.untracked.string("AK4PFPuppi")
            ),
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK8PF"),
            label= cms.untracked.string("AK8PF")
            ),
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK8PFchs"),
            label= cms.untracked.string("AK8PFchs")
            ),
        cms.PSet(
            record = cms.string("JetCorrectionsRecord"),
            tag = cms.string("JetCorrectorParametersCollection_"+dbfile+"_AK8PFPuppi"),
            label= cms.untracked.string("AK8PFPuppi")
            ),
        )
                           )

process.es_prefer_jec = cms.ESPrefer("PoolDBESSource",'jec')



## Output Module Configuration (expects a path 'p')
from PhysicsTools.PatAlgos.patEventContent_cff import patEventContentNoCleaning
process.out = cms.OutputModule("PoolOutputModule",
                               fileName = cms.untracked.string('patTuple.root'),
                               ## save only events passing the full path
                               #SelectEvents = cms.untracked.PSet( SelectEvents = cms.vstring('p') ),
                               ## save PAT output; you need a '*' to unpack the list of commands
                               ## 'patEventContent'
                               outputCommands = cms.untracked.vstring('drop *', *patEventContentNoCleaning )
                               )

patAlgosToolsTask = getPatAlgosToolsTask(process)
process.outpath = cms.EndPath(process.out, patAlgosToolsTask)

## and add them to the event content
from PhysicsTools.PatAlgos.tools.jetTools import updateJetCollection

updateJetCollection(
   process,
   jetSource = cms.InputTag('slimmedJets'),
   pvSource = cms.InputTag('offlineSlimmedPrimaryVertices'),
   svSource = cms.InputTag('slimmedSecondaryVertices'),
   jetCorrections = ('AK4PFchs', cms.vstring(['L1FastJet', 'L2Relative', 'L3Absolute']), 'None'),
   btagDiscriminators = [
      'pfCombinedSecondaryVertexV2BJetTags',
      'pfDeepCSVJetTags:probudsg', 
      'pfDeepCSVJetTags:probb', 
      'pfDeepCSVJetTags:probc', 
      'pfDeepCSVJetTags:probbb', 
      'pfDeepFlavourJetTags:probb',
      'pfDeepFlavourJetTags:probbb',
      'pfDeepFlavourJetTags:problepb',
      'pfDeepFlavourJetTags:probc',
      'pfDeepFlavourJetTags:probuds',
      'pfDeepFlavourJetTags:probg',
      'pfNegativeDeepFlavourJetTags:probb',
      'pfNegativeDeepFlavourJetTags:probbb',
      'pfNegativeDeepFlavourJetTags:problepb',
      'pfNegativeDeepFlavourJetTags:probc',
      'pfNegativeDeepFlavourJetTags:probuds',
      'pfNegativeDeepFlavourJetTags:probg',
      ]
   )
process.source.fileNames = cms.untracked.vstring('/store/data/Run2016B/JetHT/MINIAOD/17Jul2018_ver2-v2/100000/00323A27-15B8-E811-88B5-E0071B6C9DB0.root')

process.maxEvents.input = 10

from Configuration.EventContent.EventContent_cff import MINIAODSIMEventContent
process.out.outputCommands = MINIAODSIMEventContent.outputCommands
process.out.outputCommands.append('keep *_selectedUpdatedPatJets*_*_*')
process.out.outputCommands.append('keep *_pfDeepCSVTagInfos*_*_*')
process.out.outputCommands.append('keep *_pfDeepFlavourTagInfos*_*_*')
process.out.outputCommands.append('keep *_pfDeepFlavourJetTags*_*_*')
process.out.outputCommands.append('keep *_updatedPatJets*_*_*')

process.out.fileName = 'test_deep_flavour_MINIAODSIM.root'

#                                         ##
#   process.options.wantSummary = False   ##  (to suppress the long output at the end of the job)
# process.add_(cms.Service("InitRootHandlers", DebugLevel =cms.untracked.int32(3)))
