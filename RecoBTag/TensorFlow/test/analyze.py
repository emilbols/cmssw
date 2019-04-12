import ROOT
import pprint
from DataFormats.FWLite import Events, Handle
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
import numpy as np
from pdb import set_trace

events = Events([
                'file:/afs/cern.ch/user/e/ebols/JoshCheck/CMSSW_10_2_11/src/RecoBTag/TensorFlow/test/test_deep_flavour_MINIAODSIM.root'
])
handle = Handle('vector<pat::Jet>')
inspected = 0
nans = []

for event in events:
        event.getByLabel('selectedUpdatedPatJetscheck', handle)
        jets = handle.product()
        for idx, jet in enumerate(jets):
                inspected += 1
                prob_c  = jet.bDiscriminator('pfDeepFlavourJetTags:probc')
                print prob_c, jet.pt()
		if inspected > 1:
			break

